//
//    Default/Audio/AudioWidget.cpp: description
//    Copyright (C) 2022 Gonzalo José Carracedo Carballal
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this program.  If not, see
//    <http://www.gnu.org/licenses/>
//

#include "AudioWidgetFactory.h"
#include "AudioWidget.h"
#include <QVariant>
#include <FrequencyCorrectionDialog.h>
#include <QDynamicPropertyChangeEvent>
#include <util/compat-statvfs.h>
#include "ui_AudioWidget.h"
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <UIMediator.h>
#include "AudioProcessor.h"
#include <SuWidgetsHelpers.h>
#include <MainSpectrum.h>

using namespace SigDigger;

static const unsigned int supportedRates[] = {
  8000,
  16000,
  32000,
  44100,
  48000,
  192000};

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

static QString
formatCaptureSize(quint64 size)
{
  if (size < (1ull << 10))
    return QString::number(size) + " bytes";
  else if (size < (1ull << 20))
    return QString::number(size >> 10) + " KiB";
  else if (size < (1ull << 30))
    return QString::number(size >> 20) + " MiB";

  return QString::number(size >> 30) + " GiB";
}

void
AudioWidgetConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(enabled);
  LOAD(collapsed);
  LOAD(demod);
  LOAD(rate);
  LOAD(cutOff);
  LOAD(volume);
  LOAD(savePath);
  LOAD(squelch);
  LOAD(amSquelch);
  LOAD(ssbSquelch);
  LOAD(tleCorrection);
  LOAD(isSatellite);
  LOAD(satName);
  LOAD(tleData);
}

Suscan::Object &&
AudioWidgetConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("AudioWidgetConfig");

  STORE(enabled);
  STORE(collapsed);
  STORE(demod);
  STORE(rate);
  STORE(cutOff);
  STORE(volume);
  STORE(savePath);
  STORE(squelch);
  STORE(amSquelch);
  STORE(ssbSquelch);
  STORE(tleCorrection);
  STORE(isSatellite);
  STORE(satName);
  STORE(tleData);

  return this->persist(obj);
}

//////////////////////////////////// Audio Widget //////////////////////////////
AudioWidget::AudioWidget(
    AudioWidgetFactory *factory,
    UIMediator *mediator,
    QWidget *parent) :
  ToolWidget(factory, mediator, parent),
  ui(new Ui::AudioPanel)

{
  ui->setupUi(this);

  m_processor = new AudioProcessor(mediator, this);
  m_spectrum  = mediator->getMainSpectrum();

  this->setRecordSavePath(QDir::currentPath().toStdString());

  this->fcDialog = new FrequencyCorrectionDialog(
      this,
      this->demodFreq,
      this->colorConfig);

  this->assertConfig();
  this->populateRates();
  this->connectAll();

  this->setProperty("collapsed", this->panelConfig->collapsed);
}

AudioWidget::~AudioWidget()
{
  delete ui;
}

// Private methods
void
AudioWidget::populateRates(void)
{
  this->ui->sampleRateCombo->clear();

  for (unsigned i = 0; i < sizeof(supportedRates) / sizeof(supportedRates[0]); ++i) {
    if (this->bandwidth > supportedRates[i]) {
      this->ui->sampleRateCombo->addItem(
          QString::number(supportedRates[i]),
          QVariant(supportedRates[i]));
      if (supportedRates[i] == this->panelConfig->rate)
        this->ui->sampleRateCombo->setCurrentIndex(static_cast<int>(i));
    }
  }
}

void
AudioWidget::connectAll(void)
{
  connect(
      this->ui->audioPreviewCheck,
      SIGNAL(stateChanged(int)),
      this,
      SLOT(onEnabledChanged(void)));

  connect(
      this->ui->sampleRateCombo,
      SIGNAL(activated(int)),
      this,
      SLOT(onSampleRateChanged(void)));

  connect(
      this->ui->demodCombo,
      SIGNAL(activated(int)),
      this,
      SLOT(onDemodChanged(void)));

  connect(
      this->ui->cutoffSlider,
      SIGNAL(valueChanged(int)),
      this,
      SLOT(onFilterChanged(void)));

  connect(
      this->ui->volumeSlider,
      SIGNAL(valueChanged(int)),
      this,
      SLOT(onVolumeChanged(void)));

  connect(
      this->ui->muteButton,
      SIGNAL(toggled(bool)),
      this,
      SLOT(onMuteToggled(bool)));

  connect(
        this->ui->saveButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onChangeSavePath(void)));

  connect(
        this->ui->recordStartStopButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onRecordStartStop(void)));

  connect(
        this->ui->sqlButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleSquelch(void)));

  connect(
        this->ui->sqlLevelSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onSquelchLevelChanged(void)));

  connect(
        this->ui->dopplerSettingsButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onOpenDopplerSettings(void)));

  connect(
        this->fcDialog,
        SIGNAL(accepted()),
        this,
        SLOT(onAcceptCorrectionSetting(void)));

  connect(
        m_spectrum,
        SIGNAL(bandwidthChanged()),
        this,
        SLOT(onSpectrumBandwidthChanged()));

  connect(
        m_spectrum,
        SIGNAL(loChanged(qint64)),
        this,
        SLOT(onSpectrumLoChanged(qint64)));

  connect(
        m_spectrum,
        SIGNAL(frequencyChanged(qint64)),
        this,
        SLOT(onSpectrumFrequencyChanged(qint64)));

  this->connect(
        m_processor,
        SIGNAL(recStopped()),
        this,
        SLOT(onAudioSaveError()));

  this->connect(
        m_processor,
        SIGNAL(recSwamped()),
        this,
        SLOT(onAudioSaveSwamped()));

  this->connect(
        m_processor,
        SIGNAL(recSaveRate(qreal)),
        this,
        SLOT(onAudioSaveRate(qreal)));

  this->connect(
        m_processor,
        SIGNAL(recCommit()),
        this,
        SLOT(onAudioCommit()));
}

bool
AudioWidget::shouldOpenAudio(void) const
{
  bool audioAvailable = m_processor->isAudioAvailable();
  bool validRate = this->bandwidth >= supportedRates[0];
  return m_audioAllowed && audioAvailable && this->getEnabled() && validRate;
}

void
AudioWidget::refreshUi(void)
{
  bool shouldOpenAudio = this->shouldOpenAudio();
  bool validRate = this->bandwidth >= supportedRates[0];
  MainSpectrum::Skewness skewness = MainSpectrum::SYMMETRIC;
  bool recording = m_processor->isRecording();
  bool audioAvailable = m_processor->isAudioAvailable();

  this->ui->audioPreviewCheck->setEnabled(
        audioAvailable && validRate && m_audioAllowed);
  this->ui->demodCombo->setEnabled(shouldOpenAudio);
  this->ui->sampleRateCombo->setEnabled(shouldOpenAudio);
  this->ui->cutoffSlider->setEnabled(shouldOpenAudio);
  this->ui->recordStartStopButton->setEnabled(shouldOpenAudio);

  this->ui->sqlButton->setEnabled(shouldOpenAudio);
  this->ui->sqlLevelSpin->setEnabled(
        shouldOpenAudio && this->getDemod() != AudioDemod::FM);

  if (validRate) {
    this->setCutOff(this->panelConfig->cutOff);
    this->setVolume(this->panelConfig->volume);
  }

  switch (this->getDemod()) {
    case AudioDemod::AM:
      this->ui->sqlLevelSpin->setSuffix(" %");
      this->ui->sqlLevelSpin->setMinimum(0);
      this->ui->sqlLevelSpin->setMaximum(100);
      this->ui->sqlLevelSpin->setValue(
            static_cast<qreal>(this->panelConfig->amSquelch * 100));
      break;

    case AudioDemod::FM:
      this->ui->sqlLevelSpin->setSuffix("");
      this->ui->sqlLevelSpin->setMinimum(0);
      this->ui->sqlLevelSpin->setMaximum(0);
      break;

    case AudioDemod::USB:
      this->ui->sqlLevelSpin->setSuffix(" dB");
      this->ui->sqlLevelSpin->setMinimum(-120);
      this->ui->sqlLevelSpin->setMaximum(10);
      this->ui->sqlLevelSpin->setValue(
            static_cast<qreal>(
              SU_POWER_DB(this->panelConfig->ssbSquelch)));

      if (shouldOpenAudio)
        skewness = MainSpectrum::UPPER;
      break;

    case AudioDemod::LSB:
      this->ui->sqlLevelSpin->setSuffix(" dB");
      this->ui->sqlLevelSpin->setMinimum(-120);
      this->ui->sqlLevelSpin->setMaximum(10);
      this->ui->sqlLevelSpin->setValue(
            static_cast<qreal>(
              SU_POWER_DB(this->panelConfig->ssbSquelch)));

      if (shouldOpenAudio)
        skewness = MainSpectrum::LOWER;
      break;
  }

  m_spectrum->setFilterSkewness(skewness);

  this->ui->recordStartStopButton->setText(recording ? "Stop" : "Record");
}

void
AudioWidget::setDiskUsage(qreal usage)
{
  if (std::isnan(usage)) {
    this->ui->diskUsageProgress->setEnabled(false);
    this->ui->diskUsageProgress->setValue(100);
  } else {
    this->ui->diskUsageProgress->setEnabled(true);
    this->ui->diskUsageProgress->setValue(static_cast<int>(usage * 100));
  }
}


// Getters
SUFLOAT
AudioWidget::getBandwidth(void) const
{
  return this->bandwidth;
}

bool
AudioWidget::getEnabled(void) const
{
  return this->ui->audioPreviewCheck->isChecked();
}

enum AudioDemod
AudioWidget::getDemod(void) const
{
  return static_cast<enum AudioDemod>(this->ui->demodCombo->currentIndex());
}

unsigned int
AudioWidget::getSampleRate(void) const
{
  if (this->ui->sampleRateCombo->count() > 0)
    return this->ui->sampleRateCombo->currentData().value<unsigned int>();

  return 0;
}

SUFLOAT
AudioWidget::getCutOff(void) const
{
  return this->ui->cutoffSlider->value();
}

SUFLOAT
AudioWidget::getVolume(void) const
{
  return this->ui->volumeSlider->value();
}

SUFLOAT
AudioWidget::getMuteableVolume(void) const
{
  return this->isMuted() ? -120 : this->getVolume();
}

bool
AudioWidget::isMuted(void) const
{
  return this->ui->muteButton->isChecked();
}

bool
AudioWidget::isCorrectionEnabled(void) const
{
  return this->fcDialog->isCorrectionEnabled();
}

bool
AudioWidget::getSquelchEnabled(void) const
{
  return this->ui->sqlButton->isChecked();
}

SUFLOAT
AudioWidget::getSquelchLevel(void) const
{
  switch (this->getDemod()) {
    case AudioDemod::AM:
      return SU_ASFLOAT(this->ui->sqlLevelSpin->value() * 1e-2);

    case AudioDemod::USB:
    case AudioDemod::LSB:
      return SU_POWER_MAG(SU_ASFLOAT(this->ui->sqlLevelSpin->value()));

    default:
      break;
  }

  return 0;
}

Suscan::Orbit
AudioWidget::getOrbit(void) const
{
  return this->fcDialog->getOrbit();
}

bool
AudioWidget::getRecordState(void) const
{
  return this->ui->recordStartStopButton->isChecked();
}

std::string
AudioWidget::getRecordSavePath(void) const
{
  return this->ui->savePath->text().toStdString();
}

// Setters
void
AudioWidget::setSampleRate(unsigned int rate)
{
  if (rate < this->bandwidth) {
    int i;
    this->panelConfig->rate = rate;
    bool add = true;
    for (i = 0; i < this->ui->sampleRateCombo->count(); ++i) {
      if (this->ui->sampleRateCombo->itemData(i).value<unsigned int>()
          == rate) {
        this->ui->sampleRateCombo->setCurrentIndex(i);
        add = false;
      }
    }

    if (add) {
      this->ui->sampleRateCombo->addItem(QString::number(rate), QVariant(rate));
      this->ui->sampleRateCombo->setCurrentIndex(i);
    }

    // Stay below maximum frequency (fs / 2)
    this->ui->cutoffSlider->setMaximum(rate / 2);

    m_processor->setSampleRate(rate);
  }
}

void
AudioWidget::setCutOff(SUFLOAT cutoff)
{
  this->panelConfig->cutOff = cutoff;
  this->ui->cutoffSlider->setValue(static_cast<int>(cutoff));
  this->ui->cutoffLabel->setText(
        QString::number(this->ui->cutoffSlider->value()) + " Hz");
  m_processor->setCutOff(cutoff);
}

void
AudioWidget::setVolume(SUFLOAT volume)
{
  this->panelConfig->volume = volume;
  this->ui->volumeSlider->setValue(static_cast<int>(volume));
  this->ui->volumeLabel->setText(
        QString::number(this->ui->volumeSlider->value()) + " dB");

  m_processor->setVolume(this->getMuteableVolume());
}

void
AudioWidget::setMuted(bool muted)
{
  this->ui->muteButton->setChecked(muted);
  m_processor->setVolume(this->getMuteableVolume());
}

void
AudioWidget::setSquelchEnabled(bool enabled)
{
  this->panelConfig->squelch = enabled;
  this->ui->sqlButton->setChecked(enabled);
  m_processor->setSquelchEnabled(enabled);
  this->refreshUi();
}

void
AudioWidget::setSquelchLevel(SUFLOAT val)
{
  switch (this->getDemod()) {
    case AudioDemod::AM:
      this->panelConfig->amSquelch = val;
      break;

    case AudioDemod::USB:
    case AudioDemod::LSB:
      this->panelConfig->ssbSquelch = val;
      break;

    default:
      break;
  }

  m_processor->setSquelchLevel(val);

  this->refreshUi();
}

void
AudioWidget::setEnabled(bool enabled)
{
  this->panelConfig->enabled = enabled;
  this->ui->audioPreviewCheck->setChecked(enabled);

  m_processor->setEnabled(enabled && this->shouldOpenAudio());

  this->refreshUi();
}

void
AudioWidget::setDemod(enum AudioDemod demod)
{
  this->panelConfig->demod = SigDiggerHelpers::demodToStr(demod);
  this->ui->demodCombo->setCurrentIndex(static_cast<int>(demod));

  m_processor->setDemod(demod);

  this->refreshUi();
}

void
AudioWidget::refreshDiskUsage(void)
{
  std::string path = this->getRecordSavePath().c_str();
  struct statvfs svfs;

  if (statvfs(path.c_str(), &svfs) != -1)
    this->setDiskUsage(
          1. - static_cast<qreal>(svfs.f_bavail) /
          static_cast<qreal>(svfs.f_blocks));
  else
    this->setDiskUsage(std::nan(""));
}


void
AudioWidget::setRecordSavePath(std::string const &path)
{
  this->ui->savePath->setText(QString::fromStdString(path));
  this->refreshDiskUsage();
}

// Overriden methods
Suscan::Serializable *
AudioWidget::allocConfig(void)
{
  return this->panelConfig = new AudioWidgetConfig();
}

void
AudioWidget::applyConfig(void)
{
  this->setSampleRate(this->panelConfig->rate);
  this->setCutOff(this->panelConfig->cutOff);
  this->setVolume(this->panelConfig->volume);
  this->setDemod(SigDiggerHelpers::strToDemod(this->panelConfig->demod));
  this->setEnabled(this->panelConfig->enabled);
  this->setSquelchEnabled(this->panelConfig->squelch);
  this->setProperty("collapsed", this->panelConfig->collapsed);

  // Frequency correction dialog
  this->fcDialog->setCorrectionEnabled(this->panelConfig->tleCorrection);
  this->fcDialog->setCorrectionFromSatellite(this->panelConfig->isSatellite);
  this->fcDialog->setCurrentSatellite(
        QString::fromStdString(this->panelConfig->satName));
  this->fcDialog->setCurrentTLE(
        QString::fromStdString(this->panelConfig->tleData));

  // Recorder
  if (this->panelConfig->savePath.size() > 0)
    this->setRecordSavePath(this->panelConfig->savePath);

  // Update processor parameters
  m_processor->setBandwidth(SCAST(SUFREQ, m_spectrum->getBandwidth()));
  m_processor->setLoFreq(SCAST(SUFREQ, m_spectrum->getLoFreq()));
  m_processor->setTunerFreq(SCAST(SUFREQ, m_spectrum->getCenterFreq()));
}

bool
AudioWidget::event(QEvent *event)
{
  if (event->type() == QEvent::DynamicPropertyChange) {
    QDynamicPropertyChangeEvent *const propEvent =
        static_cast<QDynamicPropertyChangeEvent*>(event);
    QString propName = propEvent->propertyName();
    if (propName == "collapsed")
      this->panelConfig->collapsed = this->property("collapsed").value<bool>();
  }

  return QWidget::event(event);
}

void
AudioWidget::setState(int state, Suscan::Analyzer *analyzer)
{
  if (m_analyzer == nullptr && analyzer != nullptr) {
    m_haveSourceInfo = false;
    m_audioAllowed = true;

    connect(
          analyzer,
          SIGNAL(source_info_message(const Suscan::SourceInfoMessage &)),
          this,
          SLOT(onSourceInfoMessage(const Suscan::SourceInfoMessage &)));

    this->refreshUi();
  }

  m_analyzer = analyzer;

  if (state != m_state)
    m_state = state;

  m_processor->setBandwidth(SCAST(SUFREQ, m_spectrum->getBandwidth()));
  m_processor->setLoFreq(SCAST(SUFREQ, m_spectrum->getLoFreq()));
  m_processor->setTunerFreq(SCAST(SUFREQ, m_spectrum->getCenterFreq()));
  m_processor->setAnalyzer(analyzer);
}

void
AudioWidget::setQth(Suscan::Location const &qth)
{
  this->fcDialog->setQth(qth.getQth());
}

void
AudioWidget::setColorConfig(ColorConfig const &colors)
{
  this->colorConfig = colors;
  this->fcDialog->setColorConfig(colors);
}

void
AudioWidget::setTimeStamp(struct timeval const &timeStamp)
{
  this->timeStamp = timeStamp;
  this->fcDialog->setTimestamp(timeStamp);
}

void
AudioWidget::setProfile(Suscan::Source::Config &profile)
{
  struct timeval tv, start, end;
  bool isRealTime = false;

  if (!profile.isRemote()) {
    if (profile.getType() == SUSCAN_SOURCE_TYPE_SDR) {
      isRealTime = true;
    } else {
      this->setTimeStamp(profile.getStartTime());
    }
  } else {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    this->setTimeStamp(profile.getStartTime());
  }

  if (isRealTime) {
    gettimeofday(&tv, nullptr);

    start = tv;
    start.tv_sec -= 1;
    start.tv_usec = 0;

    end = tv;
    end.tv_sec += 1;
    end.tv_usec = 0;
  } else {
    if (profile.fileIsValid()) {
      start = profile.getStartTime();
      end   = profile.getEndTime();
    } else {
      start = profile.getStartTime();
      end   = start;
      end.tv_sec += 1;
    }
    tv = start;
  }

  this->isRealTime = isRealTime;
  this->fcDialog->setRealTime(this->isRealTime);
  this->fcDialog->setTimeLimits(start, end);

  m_processor->setTunerFreq(profile.getFreq());
}

//////////////////////////////// Slots ////////////////////////////////////////
void
AudioWidget::onSpectrumBandwidthChanged()
{
  m_processor->setBandwidth(SCAST(SUFREQ, m_spectrum->getBandwidth()));
}

void
AudioWidget::onSpectrumLoChanged(qint64 lo)
{
  m_processor->setLoFreq(SCAST(SUFREQ, lo));
}

void
AudioWidget::onSpectrumFrequencyChanged(qint64 freq)
{
  m_processor->setTunerFreq(SCAST(SUFREQ, freq));
}

void
AudioWidget::onDemodChanged(void)
{
  this->setDemod(this->getDemod());
}

void
AudioWidget::onSampleRateChanged(void)
{
  this->setSampleRate(this->getSampleRate());
}

void
AudioWidget::onFilterChanged(void)
{
  this->setCutOff(this->getCutOff());
}

void
AudioWidget::onVolumeChanged(void)
{
  this->setVolume(this->getVolume());
}

void
AudioWidget::onMuteToggled(bool)
{
  this->ui->volumeSlider->setEnabled(!this->isMuted());
  this->ui->volumeLabel->setEnabled(!this->isMuted());

  this->ui->muteButton->setIcon(
        QIcon(
          this->isMuted()
          ? ":/icons/audio-volume-muted-panel.png"
          : ":/icons/audio-volume-medium-panel.png"));

  this->setVolume(this->getVolume());
}

void
AudioWidget::onEnabledChanged(void)
{
  if (!m_processor->isAudioAvailable()) {
    QMessageBox::warning(
          this,
          "Audio preview error",
          "Audio playback was disabled due to errors. Reason: "
          + m_processor->getAudioError());
    this->setEnabled(false);
  } else {
    this->setEnabled(this->getEnabled());
  }
}

void
AudioWidget::onChangeSavePath(void)
{
  QFileDialog dialog(this->ui->saveButton);

  dialog.setFileMode(QFileDialog::DirectoryOnly);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setWindowTitle(QString("Select current save directory"));

  if (dialog.exec()) {
    QString path = dialog.selectedFiles().first();
    this->ui->savePath->setText(path);
    this->panelConfig->savePath = path.toStdString();
    this->refreshDiskUsage();

    if (this->ui->recordStartStopButton->isChecked()) {
      bool recording;
      m_processor->stopRecording();
      recording = m_processor->startRecording(path);

      this->ui->recordStartStopButton->setChecked(recording);
    }
  }
}

void
AudioWidget::onRecordStartStop(void)
{
  bool recording = this->ui->recordStartStopButton->isChecked();
  bool nowRec = false;

  if (recording)
    nowRec = m_processor->startRecording(
          QString::fromStdString(this->panelConfig->savePath));
  else
    m_processor->stopRecording();

  if (nowRec != recording)
    this->ui->recordStartStopButton->setChecked(nowRec);

  this->refreshUi();
}

void
AudioWidget::onToggleSquelch(void)
{
  this->setSquelchEnabled(this->getSquelchEnabled());
}

void
AudioWidget::onSquelchLevelChanged(void)
{
  this->setSquelchLevel(this->getSquelchLevel());
}

void
AudioWidget::onOpenDopplerSettings(void)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  if (s->haveQth()) {
    this->fcDialog->show();
  } else {
    QMessageBox::warning(
          this,
          "Doppler settings",
          "Doppler settings require RX location to be properly initialized. "
          "Plase set a receiver location in the settings dialog.");

  }
}

void
AudioWidget::onAcceptCorrectionSetting(void)
{
  this->panelConfig->tleCorrection = this->fcDialog->isCorrectionEnabled();
  this->panelConfig->isSatellite   = this->fcDialog->isCorrectionFromSatellite();
  this->panelConfig->satName       = this->fcDialog->getCurrentSatellite().toStdString();
  this->panelConfig->tleData       = this->fcDialog->getCurrentTLE().toStdString();

  if (this->fcDialog->isCorrectionEnabled()) {
   m_processor->setAudioCorrection(this->fcDialog->getOrbit());
   m_processor->setCorrectionEnabled(true);
  } else {
    m_processor->setCorrectionEnabled(false);
  }
}

//////////////////////////// Notification slots ////////////////////////////////
void
AudioWidget::onSetTLE(Suscan::InspectorMessage const &msg)
{
  if (!msg.isTLEEnabled())
    this->ui->correctionLabel->setText("None");
}

void
AudioWidget::onOrbitReport(Suscan::InspectorMessage const &msg)
{
  this->ui->correctionLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          static_cast<qreal>(msg.getOrbitReport().getFrequencyCorrection()),
          4,
          "Hz",
          true));
}

////////////////////////// AudioFileSaver slots ////////////////////////////////
void
AudioWidget::onAudioSaveError(void)
{
  this->refreshUi();

  QMessageBox::warning(
              this,
              "SigDigger error",
              "Audio saver stopped unexpectedly. Check disk usage and directory permissions and try again.",
              QMessageBox::Ok);



}

void
AudioWidget::onAudioSaveSwamped(void)
{
  this->refreshUi();

  QMessageBox::warning(
          this,
          "SigDigger error",
          "Audiofile thread swamped. Maybe your storage device is too slow",
          QMessageBox::Ok);
}

void
AudioWidget::onAudioSaveRate(qreal)
{
  this->refreshDiskUsage();
}

void
AudioWidget::onAudioCommit(void)
{
  auto len = m_processor->getSaveSize() * sizeof(uint16_t) / sizeof(SUCOMPLEX);
  this->ui->captureSizeLabel->setText(formatCaptureSize(len));
}

void
AudioWidget::onSourceInfoMessage(Suscan::SourceInfoMessage const &msg)
{
  if (!m_haveSourceInfo) {
    m_audioAllowed =
        msg.info()->testPermission(SUSCAN_ANALYZER_PERM_OPEN_AUDIO);
    m_haveSourceInfo = true;
    this->refreshUi();
  }
}

////////////////// TODO: implement onJumpToBookmark ////////////////////////////
