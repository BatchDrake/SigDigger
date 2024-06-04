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
#include <sigutils/util/compat-statvfs.h>
#include "ui_AudioWidget.h"
#include <QDir>
#include <AddBookmarkDialog.h>
#include <QFileDialog>
#include <QMessageBox>
#include <UIMediator.h>
#include "AudioProcessor.h"
#include <SuWidgetsHelpers.h>
#include <MainSpectrum.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  define DirectoryOnly Directory
#endif

using namespace SigDigger;

static const unsigned int supportedRates[] = {
  8000,
  16000,
  32000,
  44100,
  48000,
  192000};

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), field)
#define LOAD(field) field = conf.get(STRINGFY(field), field)

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
  LOAD(lockToFreq);
  LOAD(agc);
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
AudioWidgetConfig::serialize()
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("AudioWidgetConfig");

  STORE(enabled);
  STORE(collapsed);
  STORE(lockToFreq);
  STORE(agc);
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

  return persist(obj);
}

//////////////////////////////////// Audio Widget //////////////////////////////
AudioWidget::AudioWidget(
    AudioWidgetFactory *factory,
    UIMediator *mediator,
    QWidget *parent) :
  ToolWidget(factory, mediator, parent),
  m_ui(new Ui::AudioPanel)

{
  m_ui->setupUi(this);

  m_processor = new AudioProcessor(mediator, this);
  m_spectrum  = mediator->getMainSpectrum();

  setRecordSavePath(QDir::currentPath().toStdString());

  m_fcDialog = new FrequencyCorrectionDialog(
      this,
      m_demodFreq,
      colorConfig);

  assertConfig();
  populateRates();
  connectAll();

  setProperty("collapsed", m_panelConfig->collapsed);
}

AudioWidget::~AudioWidget()
{
  delete m_ui;
}

// Private methods
void
AudioWidget::populateRates()
{
  m_ui->sampleRateCombo->clear();

  for (unsigned i = 0; i < sizeof(supportedRates) / sizeof(supportedRates[0]); ++i) {
    if (m_bandwidth > supportedRates[i]) {
      m_ui->sampleRateCombo->addItem(
          QString::number(supportedRates[i]),
          QVariant(supportedRates[i]));
      if (supportedRates[i] == m_panelConfig->rate)
        m_ui->sampleRateCombo->setCurrentIndex(static_cast<int>(i));
    }
  }
}

void
AudioWidget::connectAll()
{
  connect(
      m_ui->audioPreviewCheck,
      SIGNAL(stateChanged(int)),
      this,
      SLOT(onEnabledChanged()));

  connect(
      m_ui->lockToFrequencyCheck,
      SIGNAL(stateChanged(int)),
      this,
      SLOT(onLockToFreqChanged()));

  connect(
      m_ui->sampleRateCombo,
      SIGNAL(activated(int)),
      this,
      SLOT(onSampleRateChanged()));

  connect(
      m_ui->demodCombo,
      SIGNAL(activated(int)),
      this,
      SLOT(onDemodChanged()));

  connect(
      m_ui->cutoffSlider,
      SIGNAL(valueChanged(int)),
      this,
      SLOT(onFilterChanged()));

  connect(
      m_ui->volumeSlider,
      SIGNAL(valueChanged(int)),
      this,
      SLOT(onVolumeChanged()));

  connect(
      m_ui->muteButton,
      SIGNAL(toggled(bool)),
      this,
      SLOT(onMuteToggled(bool)));

  connect(
        m_ui->saveButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onChangeSavePath()));

  connect(
        m_ui->recordStartStopButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onRecordStartStop()));

  connect(
        m_ui->sqlButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleSquelch()));

  connect(
        m_ui->sqlLevelSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onSquelchLevelChanged()));

  connect(
        m_ui->dopplerSettingsButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onOpenDopplerSettings()));

  connect(
        m_ui->agcCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onAGCChanged()));

  connect(
        m_fcDialog,
        SIGNAL(accepted()),
        this,
        SLOT(onAcceptCorrectionSetting()));

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

  connect(
        m_processor,
        SIGNAL(audioOpened()),
        this,
        SLOT(onAudioOpened()));

  connect(
        m_processor,
        SIGNAL(audioClosed()),
        this,
        SLOT(onAudioClosed()));

  connect(
        m_processor,
        SIGNAL(recStopped()),
        this,
        SLOT(onAudioSaveError()));

  connect(
        m_processor,
        SIGNAL(recSwamped()),
        this,
        SLOT(onAudioSaveSwamped()));

  connect(
        m_processor,
        SIGNAL(recSaveRate(qreal)),
        this,
        SLOT(onAudioSaveRate(qreal)));

  connect(
        m_processor,
        SIGNAL(recCommit()),
        this,
        SLOT(onAudioCommit()));

  connect(
        m_processor,
        SIGNAL(audioError(QString)),
        this,
        SLOT(onAudioError(QString)));

  connect(
        m_processor,
        SIGNAL(setTLE(Suscan::InspectorMessage const &)),
        this,
        SLOT(onSetTLE(Suscan::InspectorMessage const &)));

  connect(
        m_processor,
        SIGNAL(orbitReport(Suscan::InspectorMessage const &)),
        this,
        SLOT(onOrbitReport(Suscan::InspectorMessage const &)));
}

bool
AudioWidget::shouldOpenAudio() const
{
  bool audioAvailable = m_processor->isAudioAvailable();
  bool validRate = m_bandwidth >= supportedRates[0];
  return m_audioAllowed && audioAvailable && getEnabled() && validRate;
}

void
AudioWidget::refreshUi()
{
  auto bookmarkDialog = m_mediator->getAppUI()->addBookmarkDialog;
  bool openAudio = shouldOpenAudio();
  bool validRate = m_bandwidth >= supportedRates[0];
  MainSpectrum::Skewness skewness = MainSpectrum::SYMMETRIC;
  bool recording = m_processor->isRecording();
  bool audioAvailable = m_processor->isAudioAvailable();

  m_ui->audioPreviewCheck->setEnabled(
        audioAvailable && validRate && m_audioAllowed);
  m_ui->demodCombo->setEnabled(openAudio);
  m_ui->sampleRateCombo->setEnabled(openAudio);
  m_ui->cutoffSlider->setEnabled(openAudio);
  m_ui->recordStartStopButton->setEnabled(openAudio);

  m_ui->sqlButton->setEnabled(openAudio);
  m_ui->sqlLevelSpin->setEnabled(
        openAudio && getDemod() != AudioDemod::FM);

  if (validRate) {
    setCutOff(m_panelConfig->cutOff);
    setVolume(m_panelConfig->volume);
  }

  // TODO: Fix, add an appropriate API, this just looks too much like Java
  bookmarkDialog->setModulationHint(
        QString::fromStdString(
          SigDiggerHelpers::demodToStr(getDemod())));

  switch (getDemod()) {
    case AudioDemod::AM:
      m_ui->sqlLevelSpin->setSuffix(" %");
      m_ui->sqlLevelSpin->setMinimum(0);
      m_ui->sqlLevelSpin->setMaximum(100);
      m_ui->sqlLevelSpin->setValue(
            static_cast<qreal>(m_panelConfig->amSquelch * 100));
      break;

    case AudioDemod::FM:
      m_ui->sqlLevelSpin->setSuffix("");
      m_ui->sqlLevelSpin->setMinimum(0);
      m_ui->sqlLevelSpin->setMaximum(0);
      break;

    case AudioDemod::USB:
      m_ui->sqlLevelSpin->setSuffix(" dB");
      m_ui->sqlLevelSpin->setMinimum(-120);
      m_ui->sqlLevelSpin->setMaximum(10);
      m_ui->sqlLevelSpin->setValue(
            static_cast<qreal>(
              SU_POWER_DB(m_panelConfig->ssbSquelch)));

      if (openAudio)
        skewness = MainSpectrum::UPPER;
      break;

    case AudioDemod::LSB:
      m_ui->sqlLevelSpin->setSuffix(" dB");
      m_ui->sqlLevelSpin->setMinimum(-120);
      m_ui->sqlLevelSpin->setMaximum(10);
      m_ui->sqlLevelSpin->setValue(
            static_cast<qreal>(
              SU_POWER_DB(m_panelConfig->ssbSquelch)));

      if (openAudio)
        skewness = MainSpectrum::LOWER;
      break;

    case AudioDemod::RAW:
      m_ui->sqlLevelSpin->setSuffix(" dB");
      m_ui->sqlLevelSpin->setMinimum(-120);
      m_ui->sqlLevelSpin->setMaximum(10);
      m_ui->sqlLevelSpin->setValue(
            static_cast<qreal>(
              SU_POWER_DB(m_panelConfig->ssbSquelch)));
      break;
  }

  m_spectrum->setFilterSkewness(skewness);

  m_ui->recordStartStopButton->setText(recording ? "Stop" : "Record");

  refreshNamedChannel();
}

void
AudioWidget::setDiskUsage(qreal usage)
{
  if (std::isnan(usage)) {
    m_ui->diskUsageProgress->setEnabled(false);
    m_ui->diskUsageProgress->setValue(100);
  } else {
    m_ui->diskUsageProgress->setEnabled(true);
    m_ui->diskUsageProgress->setValue(static_cast<int>(usage * 100));
  }
}


// Getters
SUFLOAT
AudioWidget::getBandwidth() const
{
  return m_bandwidth;
}

bool
AudioWidget::getEnabled() const
{
  return m_ui->audioPreviewCheck->isChecked();
}

enum AudioDemod
AudioWidget::getDemod() const
{
  return static_cast<enum AudioDemod>(m_ui->demodCombo->currentIndex());
}

bool
AudioWidget::getLockToFreq() const
{
  return m_ui->lockToFrequencyCheck->isChecked();
}

unsigned int
AudioWidget::getSampleRate() const
{
  if (m_ui->sampleRateCombo->count() > 0)
    return m_ui->sampleRateCombo->currentData().value<unsigned int>();

  return 0;
}

SUFLOAT
AudioWidget::getCutOff() const
{
  return m_ui->cutoffSlider->value();
}

SUFLOAT
AudioWidget::getVolume() const
{
  return m_ui->volumeSlider->value();
}

SUFLOAT
AudioWidget::getMuteableVolume() const
{
  return isMuted() ? -120 : getVolume();
}

bool
AudioWidget::isMuted() const
{
  return m_ui->muteButton->isChecked();
}

std::string
AudioWidget::getAGCConfig() const
{
  switch (m_ui->agcCombo->currentIndex()) {
    case 0:
      return "disabled";

    case 1:
      return "slow";

    case 2:
      return "normal";

    case 3:
      return "fast";
  }

  return "fast";
}

bool
AudioWidget::isCorrectionEnabled() const
{
  return m_panelConfig->tleCorrection;
}

bool
AudioWidget::getSquelchEnabled() const
{
  return m_ui->sqlButton->isChecked();
}

SUFLOAT
AudioWidget::getSquelchLevel() const
{
  switch (getDemod()) {
    case AudioDemod::AM:
      return SU_ASFLOAT(m_ui->sqlLevelSpin->value() * 1e-2);

    case AudioDemod::USB:
    case AudioDemod::LSB:
      return SU_POWER_MAG(SU_ASFLOAT(m_ui->sqlLevelSpin->value()));

    default:
      break;
  }

  return 0;
}

Suscan::Orbit
AudioWidget::getOrbit() const
{
  return m_fcDialog->getOrbit();
}

bool
AudioWidget::getRecordState() const
{
  return m_ui->recordStartStopButton->isChecked();
}

std::string
AudioWidget::getRecordSavePath() const
{
  return m_ui->savePath->text().toStdString();
}

// Setters
void
AudioWidget::setSampleRate(unsigned int rate)
{
  if (rate < m_bandwidth) {
    int i;
    m_panelConfig->rate = rate;
    bool add = true;
    for (i = 0; i < m_ui->sampleRateCombo->count(); ++i) {
      if (m_ui->sampleRateCombo->itemData(i).value<unsigned int>()
          == rate) {
        m_ui->sampleRateCombo->setCurrentIndex(i);
        add = false;
      }
    }

    if (add) {
      m_ui->sampleRateCombo->addItem(QString::number(rate), QVariant(rate));
      m_ui->sampleRateCombo->setCurrentIndex(i);
    }

    // Stay below maximum frequency (fs / 2)
    m_ui->cutoffSlider->setMaximum(rate / 2);

    m_processor->setSampleRate(rate);
    refreshNamedChannel();
  }
}

void
AudioWidget::setCutOff(SUFLOAT cutoff)
{
  m_panelConfig->cutOff = cutoff;
  m_ui->cutoffSlider->setValue(static_cast<int>(cutoff));
  m_ui->cutoffLabel->setText(
        QString::number(m_ui->cutoffSlider->value()) + " Hz");
  m_processor->setCutOff(cutoff);
}

void
AudioWidget::setVolume(SUFLOAT volume)
{
  m_panelConfig->volume = volume;
  m_ui->volumeSlider->setValue(static_cast<int>(volume));
  m_ui->volumeLabel->setText(
        QString::number(m_ui->volumeSlider->value()) + " dB");

  m_processor->setVolume(getMuteableVolume());
}

void
AudioWidget::setAGCConfig(std::string const &agc)
{
  int index = 2;
  float scales[] = {1, 10, 3.162f, 1};
  m_panelConfig->agc = agc;

  if (agc == "disabled")
    index = 0;
  else if (agc == "slow")
    index = 1;
  else if (agc == "normal")
    index = 2;
  else if (agc == "fast")
    index = 3;

  BLOCKSIG(m_ui->agcCombo, setCurrentIndex(index));

  m_processor->setAGCTimeScale(scales[index]);
  m_processor->setAGCEnabled(index > 0);
}

void
AudioWidget::setMuted(bool muted)
{
  m_ui->muteButton->setChecked(muted);
  m_processor->setVolume(getMuteableVolume());
}

void
AudioWidget::setSquelchEnabled(bool enabled)
{
  m_panelConfig->squelch = enabled;
  m_ui->sqlButton->setChecked(enabled);
  m_processor->setSquelchEnabled(enabled);
  refreshUi();
}

void
AudioWidget::setSquelchLevel(SUFLOAT val)
{
  switch (getDemod()) {
    case AudioDemod::AM:
      m_panelConfig->amSquelch = val;
      break;

    case AudioDemod::USB:
    case AudioDemod::LSB:
      m_panelConfig->ssbSquelch = val;
      break;

    default:
      break;
  }

  m_processor->setSquelchLevel(val);

  refreshUi();
}

void
AudioWidget::refreshNamedChannel()
{
  bool shouldHaveNamChan =
         m_analyzer != nullptr
      && m_processor->isOpened()
      && (isCorrectionEnabled() || getLockToFreq());

  // Check whether we should have a named channel here.
  if (shouldHaveNamChan != m_haveNamChan) { // Inconsistency!
    m_haveNamChan = shouldHaveNamChan;

    // Make sure we have a named channel
    if (m_haveNamChan) {
      auto cfFreq = static_cast<qint64>(m_processor->getTrueChannelFreq());
      auto chBw   = static_cast<qint32>(m_processor->calcTrueBandwidth());

      m_namChan = m_mediator->getMainSpectrum()->addChannel(
            "",
            cfFreq,
            -chBw / 2,
            +chBw / 2,
            QColor("#2f2fff"),
            QColor(Qt::white),
            QColor("#2f2fff"));
    } else {
      // We should NOT have a named channel, remove
      m_spectrum->removeChannel(m_namChan);
      m_spectrum->updateOverlay();
    }
  }

  if (m_haveNamChan) {
    qint64 cfFreq = static_cast<qint64>(
          isCorrectionEnabled()
          ? m_processor->getTrueChannelFreq() - m_lastCorrection
          : m_processor->getTrueChannelFreq());
    qint32 chBw   = static_cast<qint32>(m_processor->calcTrueBandwidth());
    QColor color  = getLockToFreq() ? QColor("#ff2f2f") : QColor("#2f2fff");
    QColor markerColor = getLockToFreq() ? QColor("#ff7f7f") : QColor("#7f7fff");
    QString text;

    if (isCorrectionEnabled()) {
      auto t = SuWidgetsHelpers::formatQuantity(-m_lastCorrection, 4, "Hz", true);
      text = "Frequency correction (" + t + ")";
    } else {
      text = "Audio inspector";
    }

    m_namChan.value()->frequency   = cfFreq;
    m_namChan.value()->lowFreqCut  = -chBw / 2;
    m_namChan.value()->highFreqCut = +chBw / 2;

    m_namChan.value()->boxColor    = color;
    m_namChan.value()->cutOffColor = color;
    m_namChan.value()->markerColor = markerColor;
    m_namChan.value()->name        = text;

    m_spectrum->refreshChannel(m_namChan);
  }
}

void
AudioWidget::applySpectrumState()
{
  auto bandwidth  = m_spectrum->getBandwidth();
  auto loFreq     = m_spectrum->getLoFreq();
  auto centerFreq = m_spectrum->getCenterFreq();

  m_processor->setBandwidth(SCAST(SUFREQ, bandwidth));
  m_processor->setLoFreq(SCAST(SUFREQ, loFreq));
  m_processor->setTunerFreq(SCAST(SUFREQ, centerFreq));

  m_fcDialog->setFrequency(m_processor->getTrueChannelFreq());

  refreshNamedChannel();
}

void
AudioWidget::setEnabled(bool enabled)
{
  m_panelConfig->enabled = enabled;
  m_ui->audioPreviewCheck->setChecked(enabled);

  m_processor->setEnabled(enabled && shouldOpenAudio());

  refreshNamedChannel();

  refreshUi();
}

void
AudioWidget::setDemod(enum AudioDemod demod)
{
  m_panelConfig->demod = SigDiggerHelpers::demodToStr(demod);
  m_ui->demodCombo->setCurrentIndex(static_cast<int>(demod));

  m_processor->setDemod(demod);

  refreshUi();
}

void
AudioWidget::setLockToFreq(bool lock)
{
  m_panelConfig->lockToFreq = lock;
  m_ui->lockToFrequencyCheck->setChecked(lock);

  if (!m_panelConfig->lockToFreq)
    applySpectrumState();
  else
    refreshNamedChannel();
}

void
AudioWidget::refreshDiskUsage()
{
  std::string path = getRecordSavePath().c_str();
  struct statvfs svfs;

  if (statvfs(path.c_str(), &svfs) != -1)
    setDiskUsage(
          1. - static_cast<qreal>(svfs.f_bavail) /
          static_cast<qreal>(svfs.f_blocks));
  else
    setDiskUsage(std::nan(""));
}


void
AudioWidget::setRecordSavePath(std::string const &path)
{
  m_ui->savePath->setText(QString::fromStdString(path));
  refreshDiskUsage();
}

// Overriden methods
Suscan::Serializable *
AudioWidget::allocConfig()
{
  return m_panelConfig = new AudioWidgetConfig();
}

void
AudioWidget::applyConfig()
{
  setSampleRate(m_panelConfig->rate);
  setCutOff(m_panelConfig->cutOff);
  setVolume(m_panelConfig->volume);
  setAGCConfig(m_panelConfig->agc);
  setDemod(SigDiggerHelpers::strToDemod(m_panelConfig->demod));
  setEnabled(m_panelConfig->enabled);
  setLockToFreq(m_panelConfig->lockToFreq);
  setSquelchEnabled(m_panelConfig->squelch);
  setProperty("collapsed", m_panelConfig->collapsed);

  // Frequency correction dialog
  m_fcDialog->findNewSatellites();
  m_fcDialog->setCorrectionFromSatellite(m_panelConfig->isSatellite);
  m_fcDialog->setCurrentSatellite(
        QString::fromStdString(m_panelConfig->satName));
  m_fcDialog->setCurrentTLE(
        QString::fromStdString(m_panelConfig->tleData));
  m_fcDialog->setCorrectionEnabled(m_panelConfig->tleCorrection);
  onAcceptCorrectionSetting(); // Flow this back to the widget

  // Recorder
  if (m_panelConfig->savePath.size() > 0)
    setRecordSavePath(m_panelConfig->savePath);

  // Update processor parameters
  applySpectrumState();
}

bool
AudioWidget::event(QEvent *event)
{
  if (event->type() == QEvent::DynamicPropertyChange) {
    QDynamicPropertyChangeEvent *const propEvent =
        static_cast<QDynamicPropertyChangeEvent*>(event);
    QString propName = propEvent->propertyName();
    if (propName == "collapsed")
      m_panelConfig->collapsed = property("collapsed").value<bool>();
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

    refreshUi();
  }

  m_analyzer = analyzer;

  if (state != m_state)
    m_state = state;

  if (analyzer == nullptr)
    m_processor->setAnalyzer(analyzer);

  applySpectrumState();
}

void
AudioWidget::setQth(Suscan::Location const &qth)
{
  m_fcDialog->setQth(qth.getQth());
}

void
AudioWidget::setColorConfig(ColorConfig const &colors)
{
  colorConfig = colors;
  m_fcDialog->setColorConfig(colors);
}

void
AudioWidget::setTimeStamp(struct timeval const &timeStamp)
{
  m_timeStamp = timeStamp;
  m_fcDialog->setTimestamp(timeStamp);
}

void
AudioWidget::setProfile(Suscan::Source::Config &profile)
{
  struct timeval tv, start, end;
  bool isRealTime = profile.isRealTime();

  if (!profile.isRemote()) {
    if (!isRealTime)
      setTimeStamp(profile.getStartTime());
  } else {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    setTimeStamp(profile.getStartTime());
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

  m_isRealTime = isRealTime;
  m_fcDialog->setRealTime(m_isRealTime);
  m_fcDialog->setTimeLimits(start, end);

  m_processor->setTunerFreq(profile.getFreq());

  refreshNamedChannel();
}

//////////////////////////////// Slots ////////////////////////////////////////
void
AudioWidget::onSpectrumBandwidthChanged()
{
  if (!m_panelConfig->lockToFreq)
    applySpectrumState();
}

void
AudioWidget::onSpectrumLoChanged(qint64)
{
  if (!m_panelConfig->lockToFreq)
    applySpectrumState();
}

void
AudioWidget::onSpectrumFrequencyChanged(qint64)
{
  if (!m_panelConfig->lockToFreq)
    applySpectrumState();
}

void
AudioWidget::onDemodChanged()
{
  setDemod(getDemod());
  refreshNamedChannel();
}

void
AudioWidget::onSampleRateChanged()
{
  setSampleRate(getSampleRate());
  refreshNamedChannel();
}

void
AudioWidget::onFilterChanged()
{
  setCutOff(getCutOff());
  refreshNamedChannel();
}

void
AudioWidget::onVolumeChanged()
{
  setVolume(getVolume());
}

void
AudioWidget::onAGCChanged()
{
  setAGCConfig(getAGCConfig());
}

void
AudioWidget::onMuteToggled(bool)
{
  m_ui->volumeSlider->setEnabled(!isMuted());
  m_ui->volumeLabel->setEnabled(!isMuted());

  m_ui->muteButton->setIcon(
        QIcon(
          isMuted()
          ? ":/icons/audio-volume-muted-panel.png"
          : ":/icons/audio-volume-medium-panel.png"));

  setVolume(getVolume());
}

void
AudioWidget::onEnabledChanged()
{
  if (!m_processor->isAudioAvailable()) {
    QMessageBox::warning(
          this,
          "Audio preview error",
          "Audio playback was disabled due to errors. Reason: "
          + m_processor->getAudioError());
    setEnabled(false);
  } else {
    setEnabled(getEnabled());
  }
}

void
AudioWidget::onChangeSavePath()
{
  QFileDialog dialog(m_ui->saveButton);

  dialog.setFileMode(QFileDialog::DirectoryOnly);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setWindowTitle(QString("Select current save directory"));

  if (dialog.exec()) {
    QString path = dialog.selectedFiles().first();
    m_ui->savePath->setText(path);
    m_panelConfig->savePath = path.toStdString();
    refreshDiskUsage();

    if (m_ui->recordStartStopButton->isChecked()) {
      bool recording;
      m_processor->stopRecording();
      recording = m_processor->startRecording(path);

      m_ui->recordStartStopButton->setChecked(recording);
    }
  }
}

void
AudioWidget::onRecordStartStop()
{
  bool recording = m_ui->recordStartStopButton->isChecked();
  bool nowRec = false;

  if (recording)
    nowRec = m_processor->startRecording(
          QString::fromStdString(m_panelConfig->savePath));
  else
    m_processor->stopRecording();

  if (nowRec != recording)
    m_ui->recordStartStopButton->setChecked(nowRec);

  refreshUi();
}

void
AudioWidget::onToggleSquelch()
{
  setSquelchEnabled(getSquelchEnabled());
}

void
AudioWidget::onSquelchLevelChanged()
{
  setSquelchLevel(getSquelchLevel());
}

void
AudioWidget::onOpenDopplerSettings()
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  if (s->haveQth()) {
    m_fcDialog->show();
  } else {
    QMessageBox::warning(
          this,
          "Doppler settings",
          "Doppler settings require RX location to be properly initialized. "
          "Plase set a receiver location in the settings dialog.");

  }
}

void
AudioWidget::onAcceptCorrectionSetting()
{
  m_panelConfig->tleCorrection = m_fcDialog->isCorrectionEnabled();
  m_panelConfig->isSatellite   = m_fcDialog->isCorrectionFromSatellite();
  m_panelConfig->satName       = m_fcDialog->getCurrentSatellite().toStdString();
  m_panelConfig->tleData       = m_fcDialog->getCurrentTLE().toStdString();

  if (isCorrectionEnabled()) {
   m_processor->setAudioCorrection(m_fcDialog->getOrbit());
   m_processor->setCorrectionEnabled(true);
  } else {
    m_processor->setCorrectionEnabled(false);
  }

  refreshNamedChannel();
}

//////////////////////////// Notification slots ////////////////////////////////
void
AudioWidget::onSetTLE(Suscan::InspectorMessage const &msg)
{
  if (!msg.isTLEEnabled()) {
    m_ui->correctionLabel->setText("None");
    m_lastCorrection = 0;
  }
}

void
AudioWidget::onOrbitReport(Suscan::InspectorMessage const &msg)
{
  m_lastCorrection = static_cast<qreal>(
        msg.getOrbitReport().getFrequencyCorrection());

  m_ui->correctionLabel->setText(
        SuWidgetsHelpers::formatQuantity(m_lastCorrection, 4, "Hz", true));

  refreshNamedChannel();
}

void
AudioWidget::onAudioError(QString error)
{
  refreshUi();

  QMessageBox::warning(
              this,
              "SigDigger error",
              error,
              QMessageBox::Ok);

  setEnabled(false);
}


////////////////////////// AudioProcessor slots ////////////////////////////////
void
AudioWidget::onAudioOpened()
{
  refreshNamedChannel();
}

void
AudioWidget::onAudioClosed()
{
  refreshNamedChannel();
}

void
AudioWidget::onAudioSaveError()
{
  refreshUi();

  QMessageBox::warning(
              this,
              "SigDigger error",
              "Audio saver stopped unexpectedly. Check disk usage and directory permissions and try again.",
              QMessageBox::Ok);
}

void
AudioWidget::onAudioSaveSwamped()
{
  refreshUi();

  QMessageBox::warning(
          this,
          "SigDigger error",
          "Audiofile thread swamped. Maybe your storage device is too slow",
          QMessageBox::Ok);
}

void
AudioWidget::onAudioSaveRate(qreal)
{
  refreshDiskUsage();
}

void
AudioWidget::onAudioCommit()
{
  auto len = m_processor->getSaveSize() * sizeof(uint16_t) / sizeof(SUCOMPLEX);
  m_ui->captureSizeLabel->setText(formatCaptureSize(len));
}

void
AudioWidget::onSourceInfoMessage(Suscan::SourceInfoMessage const &msg)
{
  if (!m_haveSourceInfo) {
    m_audioAllowed =
        msg.info()->testPermission(SUSCAN_ANALYZER_PERM_OPEN_AUDIO);

    if (m_audioAllowed) {
      // We do not update processor parameters until source info is available
      applySpectrumState();
      m_processor->setAnalyzer(m_analyzer);
    }

    m_haveSourceInfo = true;
    refreshUi();
  }
}

void
AudioWidget::onLockToFreqChanged()
{
  setLockToFreq(getLockToFreq());
}


////////////////// TODO: implement onJumpToBookmark ////////////////////////////
