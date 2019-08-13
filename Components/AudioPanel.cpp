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

using namespace SigDigger;

static const unsigned int supportedRates[] = {
  8000,
  16000,
  32000,
  44100,
  48000,
  196000};

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

void
AudioPanelConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(enabled);
  LOAD(demod);
  LOAD(rate);
  LOAD(cutOff);
  LOAD(volume);
}

Suscan::Object &&
AudioPanelConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("AudioPanelConfig");

  STORE(enabled);
  STORE(demod);
  STORE(rate);
  STORE(cutOff);
  STORE(volume);

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
  if (this->bandwidth < supportedRates[0]) {
    this->ui->audioPreviewCheck->setChecked(false);
    this->ui->audioPreviewCheck->setEnabled(false);
    this->ui->demodCombo->setEnabled(false);
    this->ui->sampleRateCombo->setEnabled(false);
    this->ui->cutoffSlider->setEnabled(false);
  } else {
    bool enabled = this->getEnabled();
    this->ui->audioPreviewCheck->setEnabled(true);
    this->ui->demodCombo->setEnabled(enabled);
    this->ui->sampleRateCombo->setEnabled(enabled);
    this->ui->cutoffSlider->setEnabled(enabled);

    this->setCutOff(this->panelConfig->cutOff);
    this->setVolume(this->panelConfig->volume);
  }
}

AudioPanel::AudioPanel(QWidget *parent) :
  PersistentWidget(parent),
  ui(new Ui::AudioPanel)
{
  ui->setupUi(this);

  this->assertConfig();
  this->populateRates();
  this->connectAll();
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
AudioPanel::setDemod(enum AudioDemod demod)
{
  this->panelConfig->demod = AudioPanel::demodToStr(demod);
  this->ui->demodCombo->setCurrentIndex(static_cast<int>(demod));
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
        QString::number(this->ui->volumeSlider->value()) + "%");
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

  emit changed();
}

void
AudioPanel::onEnabledChanged(void)
{
  this->setEnabled(this->getEnabled());

  emit changed();
}
