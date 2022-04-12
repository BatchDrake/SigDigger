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
#include "ui_AudioPanel.h"

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
    QWidget *parent) : ToolWidget(factory, mediator, parent)
{
  ui->setupUi(this);
}

AudioWidget::~AudioWidget()
{
  delete ui;
}

// Private methods
bool
AudioWidget::shouldOpenAudio(void) const
{
  bool validRate = this->bandwidth >= supportedRates[0];
  return this->audioAllowed && this->getEnabled() && validRate;
}

void
AudioWidget::refreshUi(void)
{
  bool shouldOpenAudio = this->shouldOpenAudio();
  bool validRate = this->bandwidth >= supportedRates[0];

  this->ui->audioPreviewCheck->setEnabled(validRate && this->audioAllowed);
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
  }
}

void
AudioWidget::setCutOff(SUFLOAT cutoff)
{
  this->panelConfig->cutOff = cutoff;
  this->ui->cutoffSlider->setValue(static_cast<int>(cutoff));
  this->ui->cutoffLabel->setText(
        QString::number(this->ui->cutoffSlider->value()) + " Hz");
}

void
AudioWidget::setVolume(SUFLOAT volume)
{
  this->panelConfig->volume = volume;
  this->ui->volumeSlider->setValue(static_cast<int>(volume));
  this->ui->volumeLabel->setText(
        QString::number(this->ui->volumeSlider->value()) + " dB");
}

void
AudioWidget::setQth(xyz_t const &qth)
{
  this->fcDialog->setQth(qth);
}

void
AudioWidget::setMuted(bool muted)
{
  this->ui->muteButton->setChecked(muted);
}

void
AudioWidget::setColorConfig(ColorConfig const &colors)
{
  this->colorConfig = colors;
  this->fcDialog->setColorConfig(colors);
}

void
AudioWidget::setSquelchEnabled(bool enabled)
{
  this->panelConfig->squelch = enabled;
  this->ui->sqlButton->setChecked(enabled);
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

  this->refreshUi();
}

void
AudioWidget::setEnabled(bool enabled)
{
  this->panelConfig->enabled = enabled;
  this->ui->audioPreviewCheck->setChecked(enabled);
  this->refreshUi();
}

void
AudioWidget::setDemod(enum AudioDemod demod)
{
  this->panelConfig->demod = AudioWidget::demodToStr(demod);
  this->ui->demodCombo->setCurrentIndex(static_cast<int>(demod));
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

//////////////////////////// Static members ///////////////////////////////////
AudioDemod
AudioWidget::strToDemod(std::string const &str)
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
AudioWidget::demodToStr(AudioDemod demod)
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
