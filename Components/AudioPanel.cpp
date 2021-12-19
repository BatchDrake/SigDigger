//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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

#include "AudioPanel.h"
#include "ui_AudioPanel.h"
#include <QDir>
#include <QFileDialog>
#include <FrequencyCorrectionDialog.h>
#include <SuWidgetsHelpers.h>
#include <suscan.h>
#include <QMessageBox>
#include <QDynamicPropertyChangeEvent>

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

void
AudioPanelConfig::deserialize(Suscan::Object const &conf)
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
AudioPanelConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("AudioPanelConfig");

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

///////////////////////////////// Audio Panel //////////////////////////////////
AudioDemod
AudioPanel::strToDemod(std::string const &str)
{
  if (str == "AM")
    return AudioDemod::AM;
  else if (str == "FM")
    return AudioDemod::FM;
  else if (str == "USB")
    return AudioDemod::USB;
  else if (str == "LSB")
    return AudioDemod::LSB;

  return AudioDemod::AM;
}

std::string
AudioPanel::demodToStr(AudioDemod demod)
{
  switch (demod) {
    case AM:
      return "AM";

    case FM:
      return "FM";

    case USB:
      return "USB";

    case LSB:
      return "LSB";
  }

  return "AM"; // Default
}

void
AudioPanel::connectAll(void)
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
}

void
AudioPanel::populateRates(void)
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
AudioPanel::refreshUi(void)
{
  bool enabled = this->getEnabled();
  bool validRate = this->bandwidth >= supportedRates[0];

  this->ui->audioPreviewCheck->setEnabled(validRate);
  this->ui->demodCombo->setEnabled(enabled && validRate);
  this->ui->sampleRateCombo->setEnabled(enabled && validRate);
  this->ui->cutoffSlider->setEnabled(enabled && validRate);
  this->ui->recordStartStopButton->setEnabled(enabled && validRate);

  this->ui->sqlButton->setEnabled(enabled && validRate);
  this->ui->sqlLevelSpin->setEnabled(
        enabled && validRate && this->getDemod() != AudioDemod::FM);

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
    case AudioDemod::LSB:
      this->ui->sqlLevelSpin->setSuffix(" dB");
      this->ui->sqlLevelSpin->setMinimum(-120);
      this->ui->sqlLevelSpin->setMaximum(10);
      this->ui->sqlLevelSpin->setValue(
            static_cast<qreal>(
              SU_POWER_DB(this->panelConfig->ssbSquelch)));
      break;
  }
}

AudioPanel::AudioPanel(QWidget *parent) :
  GenericDataSaverUI(parent),
  ui(new Ui::AudioPanel)
{
  ui->setupUi(this);

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

AudioPanel::~AudioPanel()
{
  delete ui;
}

// Setters
void
AudioPanel::setBandwidth(SUFLOAT bw)
{
  this->bandwidth = bw;
  this->populateRates();
  this->refreshUi();
}

void
AudioPanel::setEnabled(bool enabled)
{
  this->panelConfig->enabled = enabled;
  this->ui->audioPreviewCheck->setChecked(enabled);
  this->refreshUi();
}

void
AudioPanel::setDemodFreq(qint64 freq)
{
  this->demodFreq = static_cast<SUFREQ>(freq);
  this->fcDialog->setFrequency(this->demodFreq);
}

void
AudioPanel::setRealTime(bool realTime)
{
  this->isRealTime = realTime;
  this->fcDialog->setRealTime(this->isRealTime);
}

void
AudioPanel::setTimeStamp(struct timeval const &tv)
{
  this->timeStamp = tv;
  this->fcDialog->setTimestamp(tv);
}

void
AudioPanel::setTimeLimits(
    struct timeval const &start,
    struct timeval const &end)
{
  this->fcDialog->setTimeLimits(start, end);
}

void
AudioPanel::resetTimeStamp(struct timeval const &tv)
{
  this->timeStamp = tv;
  this->fcDialog->resetTimestamp(tv);
}

void
AudioPanel::setDemod(enum AudioDemod demod)
{
  this->panelConfig->demod = AudioPanel::demodToStr(demod);
  this->ui->demodCombo->setCurrentIndex(static_cast<int>(demod));
  this->refreshUi();
}

void
AudioPanel::setSampleRate(unsigned int rate)
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
  }
}

void
AudioPanel::setCutOff(SUFLOAT cutoff)
{
  this->panelConfig->cutOff = cutoff;
  this->ui->cutoffSlider->setValue(static_cast<int>(cutoff));
  this->ui->cutoffLabel->setText(
        QString::number(this->ui->cutoffSlider->value()) + " Hz");
}

void
AudioPanel::setVolume(SUFLOAT volume)
{
  this->panelConfig->volume = volume;
  this->ui->volumeSlider->setValue(static_cast<int>(volume));
  this->ui->volumeLabel->setText(
        QString::number(this->ui->volumeSlider->value()) + " dB");
}

void
AudioPanel::setQth(xyz_t const &qth)
{
  this->fcDialog->setQth(qth);
}

void
AudioPanel::setMuted(bool muted)
{
  this->ui->muteButton->setChecked(muted);
}

void
AudioPanel::setColorConfig(ColorConfig const &colors)
{
  this->colorConfig = colors;
  this->fcDialog->setColorConfig(colors);
}

void
AudioPanel::setSquelchEnabled(bool enabled)
{
  this->panelConfig->squelch = enabled;
  this->ui->sqlButton->setChecked(enabled);
  this->refreshUi();
}

void
AudioPanel::setSquelchLevel(SUFLOAT val)
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

  this->refreshUi();
}

void
AudioPanel::notifyOrbitReport(Suscan::OrbitReport const &report)
{
  this->ui->correctionLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          static_cast<qreal>(report.getFrequencyCorrection()),
          4,
          "Hz",
          true));
}

void
AudioPanel::notifyDisableCorrection(void)
{
  this->ui->correctionLabel->setText("None");
}

// Overriden setters
void
AudioPanel::setRecordSavePath(std::string const &path)
{
  this->ui->savePath->setText(QString::fromStdString(path));
  this->refreshDiskUsage();
}

void
AudioPanel::setSaveEnabled(bool enabled)
{
  if (!enabled)
    this->ui->saveButton->setChecked(false);

  this->ui->saveButton->setEnabled(enabled);
}

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
AudioPanel::setCaptureSize(quint64 size)
{
  this->ui->captureSizeLabel->setText(formatCaptureSize(size));
}

void
AudioPanel::setDiskUsage(qreal usage)
{
  if (std::isnan(usage)) {
    this->ui->diskUsageProgress->setEnabled(false);
    this->ui->diskUsageProgress->setValue(100);
  } else {
    this->ui->diskUsageProgress->setEnabled(true);
    this->ui->diskUsageProgress->setValue(static_cast<int>(usage * 100));
  }
}

void
AudioPanel::setIORate(qreal)
{
  // Do nothing. This is in general slow.
  this->refreshDiskUsage();
}

void
AudioPanel::setRecordState(bool state)
{
  this->ui->recordStartStopButton->setChecked(state);
  this->ui->recordStartStopButton->setText(state ? "Stop" : "Record");
}

// Getters
SUFLOAT
AudioPanel::getBandwidth(void) const
{
  return this->bandwidth;
}

bool
AudioPanel::getEnabled(void) const
{
  return this->ui->audioPreviewCheck->isChecked();
}

enum AudioDemod
AudioPanel::getDemod(void) const
{
  return static_cast<enum AudioDemod>(this->ui->demodCombo->currentIndex());
}

unsigned int
AudioPanel::getSampleRate(void) const
{
  if (this->ui->sampleRateCombo->count() > 0)
    return this->ui->sampleRateCombo->currentData().value<unsigned int>();

  return 0;
}

SUFLOAT
AudioPanel::getCutOff(void) const
{
  return this->ui->cutoffSlider->value();
}

SUFLOAT
AudioPanel::getVolume(void) const
{
  return this->ui->volumeSlider->value();
}

SUFLOAT
AudioPanel::getMuteableVolume(void) const
{
  return this->isMuted() ? -120 : this->getVolume();
}

bool
AudioPanel::isMuted(void) const
{
  return this->ui->muteButton->isChecked();
}

bool
AudioPanel::isCorrectionEnabled(void) const
{
  return this->fcDialog->isCorrectionEnabled();
}

bool
AudioPanel::getSquelchEnabled(void) const
{
  return this->ui->sqlButton->isChecked();
}

SUFLOAT
AudioPanel::getSquelchLevel(void) const
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
AudioPanel::getOrbit(void) const
{
  return this->fcDialog->getOrbit();
}

// Overriden getters
bool
AudioPanel::getRecordState(void) const
{
  return this->ui->recordStartStopButton->isChecked();
}

std::string
AudioPanel::getRecordSavePath(void) const
{
  return this->ui->savePath->text().toStdString();
}

// Overriden methods
Suscan::Serializable *
AudioPanel::allocConfig(void)
{
  return this->panelConfig = new AudioPanelConfig();
}

void
AudioPanel::applyConfig(void)
{
  this->setSampleRate(this->panelConfig->rate);
  this->setCutOff(this->panelConfig->cutOff);
  this->setVolume(this->panelConfig->volume);
  this->setDemod(strToDemod(this->panelConfig->demod));
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
}

bool
AudioPanel::event(QEvent *event)
{
  if (event->type() == QEvent::DynamicPropertyChange) {
    QDynamicPropertyChangeEvent *const propEvent =
        static_cast<QDynamicPropertyChangeEvent*>(event);
    QString propName = propEvent->propertyName();
    if (propName == "collapsed")
      this->panelConfig->collapsed = this->property("collapsed").value<bool>();
  }

  return GenericDataSaverUI::event(event);
}
//////////////////////////////// Slots ////////////////////////////////////////
void
AudioPanel::onDemodChanged(void)
{
  this->setDemod(this->getDemod());

  emit changed();
}

void
AudioPanel::onSampleRateChanged(void)
{
  this->setSampleRate(this->getSampleRate());

  emit changed();
}

void
AudioPanel::onFilterChanged(void)
{
  this->setCutOff(this->getCutOff());

  emit changed();
}

void
AudioPanel::onVolumeChanged(void)
{
  this->setVolume(this->getVolume());

  emit volumeChanged(this->getMuteableVolume());
}

void
AudioPanel::onMuteToggled(bool)
{
  this->ui->volumeSlider->setEnabled(!this->isMuted());
  this->ui->volumeLabel->setEnabled(!this->isMuted());

  this->ui->muteButton->setIcon(
        QIcon(
          this->isMuted()
          ? ":/icons/audio-volume-muted-panel.png"
          : ":/icons/audio-volume-medium-panel.png"));

  emit volumeChanged(this->getMuteableVolume());
}

void
AudioPanel::onEnabledChanged(void)
{
  this->setEnabled(this->getEnabled());

  emit changed();
}

void
AudioPanel::onChangeSavePath(void)
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
    emit recordSavePathChanged(path);
  }
}

void
AudioPanel::onRecordStartStop(void)
{
  this->ui->recordStartStopButton->setText(
        this->ui->recordStartStopButton->isChecked()
        ? "Stop"
        : "Record");

  emit recordStateChanged(this->ui->recordStartStopButton->isChecked());
}

void
AudioPanel::onToggleSquelch(void)
{
  this->setSquelchEnabled(this->getSquelchEnabled());

  emit changed();
}

void
AudioPanel::onSquelchLevelChanged(void)
{
  this->setSquelchLevel(this->getSquelchLevel());

  emit changed();
}

void
AudioPanel::onOpenDopplerSettings(void)
{
  xyz_t site;

  if (suscan_get_qth(&site)) {
    this->fcDialog->setQth(site);
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
AudioPanel::onAcceptCorrectionSetting(void)
{
  this->panelConfig->tleCorrection = this->fcDialog->isCorrectionEnabled();
  this->panelConfig->isSatellite   = this->fcDialog->isCorrectionFromSatellite();
  this->panelConfig->satName       = this->fcDialog->getCurrentSatellite().toStdString();
  this->panelConfig->tleData       = this->fcDialog->getCurrentTLE().toStdString();

  if (this->fcDialog->isCorrectionEnabled()) {
    emit setCorrection(this->fcDialog->getOrbit());
  } else {
    emit disableCorrection();
  }
}
