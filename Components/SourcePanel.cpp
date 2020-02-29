//
//    SourcePanel.cpp: source control UI
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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


#include "SourcePanel.h"
#include "ui_SourcePanel.h"
#include <SuWidgetsHelpers.h>

#include <QFileDialog>

#include <Suscan/Library.h>
#include <ConfigDialog.h>

using namespace SigDigger;

/////////////////////////////// Source panel cnfig ////////////////////////////
#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

void
SourcePanelConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(throttle);
  LOAD(throttleRate);
  LOAD(dcRemove);
  LOAD(iqRev);
  LOAD(agcEnabled);

  try {
    Suscan::Object field = conf.getField("DataSaverConfig");
    this->dataSaverConfig->deserialize(field);
  } catch (Suscan::Exception const &) {

  }
}

Suscan::Object &&
SourcePanelConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);
  Suscan::Object dataSaverConfig;

  obj.setClass("SourcePanelConfig");

  STORE(throttle);
  STORE(throttleRate);
  STORE(dcRemove);
  STORE(iqRev);
  STORE(agcEnabled);

  dataSaverConfig = this->dataSaverConfig->serialize();

  obj.setField("DataSaverConfig", dataSaverConfig);

  return this->persist(obj);
}

////////////////////////////// Source panel object ////////////////////////////
SourcePanel::SourcePanel(QWidget *parent) :
  PersistentWidget(parent),
  ui(new Ui::SourcePanel)
{
  ui->setupUi(this);

  this->saverUI = new DataSaverUI(this);
  this->ui->dataSaverGrid->addWidget(this->saverUI);
  this->assertConfig();
  this->connectAll();
}

void
SourcePanel::connectAll(void)
{
  connect(
        this->ui->throttleCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onThrottleChanged(void)));

  connect(
        this->ui->throttleSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onThrottleChanged(void)));

  connect(
        this->saverUI,
        SIGNAL(recordStateChanged(bool)),
        this,
        SLOT(onRecordStartStop()));

  connect(
        this->ui->autoGainCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onSelectAutoGain(void)));

  connect(
        this->ui->autoGainSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onChangeAutoGain(void)));

  connect(
        this->ui->gainPresetCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onToggleAutoGain(void)));

  connect(
        this->ui->dcRemoveCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onToggleDCRemove(void)));

  connect(
        this->ui->swapIQCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onToggleIQReverse(void)));

  connect(
        this->ui->agcEnabledCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onToggleAGCEnabled(void)));

  connect(
        this->ui->antennaCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onAntennaChanged(int)));

  connect(
        this->ui->bwSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onBandwidthChanged(void)));
}

QString
SourcePanel::formatSampleRate(unsigned int rate)
{
  return SuWidgetsHelpers::formatQuantityNearest(rate, 3, "sps");
}

void
SourcePanel::refreshUi()
{
  if (this->profile != nullptr)
    this->setThrottleable(this->profile->getType() != SUSCAN_SOURCE_TYPE_SDR);

  this->ui->antennaCombo->setEnabled(
        this->profile->getType() == SUSCAN_SOURCE_TYPE_SDR);
}

void
SourcePanel::selectAntenna(std::string const &name)
{
  int count;

  count = this->ui->antennaCombo->count();

  for (auto i = 0; i < count; ++i)
    if (this->ui->antennaCombo->itemText(i).toStdString() == name) {
      this->ui->antennaCombo->setCurrentIndex(i);
      return;
    }

  this->ui->antennaCombo->addItem(QString::fromStdString(name));
  this->ui->antennaCombo->setCurrentIndex(count);
}

void
SourcePanel::setSampleRate(unsigned int rate)
{
  if (this->rate != rate) {
    float step;
    this->rate = rate;
    if (rate == 0) {
      this->setProcessRate(0);
      this->ui->sampleRateLabel->setText("N/A");
    } else {
      this->ui->sampleRateLabel->setText(formatSampleRate(rate));
    }

    this->ui->bwSpin->setMaximum(this->rate);

    step = SU_POW(10., SU_FLOOR(SU_LOG(this->rate)));

    if (step >= 10.f)
      step /= 10.f;

    this->ui->bwSpin->setSingleStep(static_cast<int>(step));
  }
}

void
SourcePanel::setProcessRate(unsigned int rate)
{
  if (rate != this->processRate) {
    SUFLOAT percentUsage = 1;
    this->processRate = rate;

    if (this->rate == 0 || this->processRate == 0) {
      this->ui->processingRateLabel->setText("N/A");
      this->ui->deliveryProgress->setEnabled(false);
    } else {
      this->ui->deliveryProgress->setEnabled(true);
      this->ui->processingRateLabel->setText(
            formatSampleRate(this->processRate));
      percentUsage =
          static_cast<SUFLOAT>(this->processRate) /
          static_cast<SUFLOAT>(this->getEffectiveRate());
    }

    this->ui->deliveryProgress->setValue(static_cast<int>(percentUsage * 100));

    if (percentUsage >= SU_ADDSFX(.95))
      this->ui->deliveryLabel->setPixmap(
          QPixmap(QString::fromUtf8(":/icons/transparent.png")));
    else if (percentUsage >= SU_ADDSFX(.85))
      this->ui->deliveryLabel->setPixmap(
          QPixmap(QString::fromUtf8(":/icons/warning.png")));
    else
      this->ui->deliveryLabel->setPixmap(
          QPixmap(QString::fromUtf8(":/icons/critical.png")));
  }
}

void
SourcePanel::setBandwidth(float bw)
{
  this->ui->bwSpin->setValue(static_cast<qreal>(bw));
}

void
SourcePanel::setProfile(Suscan::Source::Config *config)
{
  SUFLOAT bw;

  this->profile = config;
  this->refreshGains(*config);
  this->refreshAutoGains(*config);

  // TODO: Move this somewhere else!!!!
  ConfigDialog::populateAntennaCombo(
        *config,
        this->ui->antennaCombo);

  this->selectAntenna(config->getAntenna());
  this->setSampleRate(config->getDecimatedSampleRate());
  this->setDCRemove(config->getDCRemove());

  bw = this->profile->getBandwidth();
  if (SU_ABS(bw) < 1e-6f)
    bw = config->getDecimatedSampleRate();

  this->setBandwidth(bw);

  this->refreshUi();
}

void
SourcePanel::setThrottleable(bool val)
{
  this->throttleable = val;
  this->ui->throttleCheck->setEnabled(val);
  if (!val)
    this->ui->throttleCheck->setChecked(false);

  this->ui->throttleSpin->setEnabled(this->ui->throttleCheck->isChecked());
  this->ui->bwSpin->setEnabled(!val);
  this->saverUI->setEnabled(!val);
}

DeviceGain *
SourcePanel::lookupGain(std::string const &name)
{
  // Why is this? Use a map instead.
  for (auto p = this->gainControls.begin();
       p != this->gainControls.end();
       ++p) {
    if ((*p)->getName() == name)
      return *p;
  }

  return nullptr;
}

void
SourcePanel::clearGains(void)
{
  int i, len;

  len = static_cast<int>(this->gainControls.size());

  for (i = 0; i < len; ++i) {
    QLayoutItem *item = this->ui->gainGridLayout->takeAt(i);
    delete item;

    // This is what C++ is for.
    this->gainControls[static_cast<unsigned long>(i)]->deleteLater();
  }

  this->gainControls.clear();
}

void
SourcePanel::deserializeAutoGains(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  for (auto i = sus->getFirstAutoGain();
       i != sus->getLastAutoGain();
       i++) {
    AutoGain ag(*i);

    this->autoGains[ag.getDriver()].push_back(ag);
  }
}

void
SourcePanel::selectAutoGain(unsigned int gain)
{
  if (this->currAutoGainSet != nullptr
      && gain < this->currAutoGainSet->size()) {
    this->currentAutoGain = &(*this->currAutoGainSet)[gain];
    this->ui->autoGainSlider->setMinimum(this->currentAutoGain->getMin());
    this->ui->autoGainSlider->setMaximum(this->currentAutoGain->getMax());
  }
}

void
SourcePanel::refreshAutoGains(Suscan::Source::Config &config)
{
  this->currAutoGainSet =
      &this->autoGains[config.getDevice().getDriver()];

  this->currentAutoGain = nullptr;

  this->ui->autoGainCombo->clear();

  if (this->currAutoGainSet->size() == 0
      || config.getType() != SUSCAN_SOURCE_TYPE_SDR) {
    this->ui->autoGainFrame->hide();
  } else {
    for (auto p = this->currAutoGainSet->begin();
         p != this->currAutoGainSet->end(); ++p)
      this->ui->autoGainCombo->addItem(
            QString::fromStdString(p->getName()));

    this->ui->autoGainFrame->show();
    this->selectAutoGain(0);
  }
}

void
SourcePanel::refreshGains(Suscan::Source::Config &config)
{
  Suscan::Source::Device const &dev = config.getDevice();
  DeviceGain *gain = nullptr;

  this->clearGains();

  for (auto p = dev.getFirstGain();
       p != dev.getLastGain();
       ++p) {
    gain = new DeviceGain(nullptr, *p);
    this->gainControls.push_back(gain);
    this->ui->gainGridLayout->addWidget(
          gain,
          static_cast<int>(this->gainControls.size() - 1),
          0,
          1,
          1);

    connect(
          gain,
          SIGNAL(gainChanged(QString, float)),
          this,
          SLOT(onGainChanged(QString, float)));
    gain->setGain(config.getGain(p->getName()));
  }

  if (this->gainControls.size() == 0
      || config.getType() != SUSCAN_SOURCE_TYPE_SDR)
    this->ui->gainsFrame->hide();
  else
    this->ui->gainsFrame->show();
}

void
SourcePanel::applyCurrentAutogain(void)
{
  if (this->currentAutoGain != nullptr) {
    std::vector<GainConfig> cfg = this->currentAutoGain->translateGain(
          this->ui->autoGainSlider->value());
    for (auto p = cfg.begin();
         p != cfg.end();
         ++p) {
      DeviceGain *gain = this->lookupGain(p->name);
      if (gain != nullptr) {
        gain->setGain(static_cast<float>(p->value));
        this->onGainChanged(
              QString::fromStdString(p->name),
              static_cast<float>(p->value));
      }
    }
  }
}

Suscan::Serializable *
SourcePanel::allocConfig(void)
{
  this->panelConfig = new SourcePanelConfig();
  this->panelConfig->dataSaverConfig = this->saverUI->allocConfig();

  return this->panelConfig;
}

void
SourcePanel::applyConfig(void)
{
  this->ui->throttleCheck->setChecked(this->panelConfig->throttle);
  this->ui->dcRemoveCheck->setChecked(this->panelConfig->dcRemove);
  this->ui->swapIQCheck->setChecked(this->panelConfig->iqRev);
  this->ui->agcEnabledCheck->setChecked(this->panelConfig->agcEnabled);
  this->ui->throttleSpin->setValue(static_cast<int>(this->panelConfig->throttleRate));

  this->saverUI->applyConfig();
}

void
SourcePanel::setCaptureSize(quint64 size)
{
  this->saverUI->setCaptureSize(size);
}

void
SourcePanel::setIORate(qreal rate)
{
  this->saverUI->setIORate(rate);
}

void
SourcePanel::setRecordState(bool state)
{
  this->saverUI->setRecordState(state);
}

void
SourcePanel::setDCRemove(bool remove)
{
  this->ui->dcRemoveCheck->setChecked(remove);
  this->panelConfig->dcRemove = remove;
}

void
SourcePanel::setAGCEnabled(bool enabled)
{
  this->ui->agcEnabledCheck->setChecked(enabled);
  this->panelConfig->agcEnabled = enabled;
}

void
SourcePanel::setIQReverse(bool rev)
{
  this->ui->swapIQCheck->setChecked(rev);
  this->panelConfig->iqRev = rev;
}

void
SourcePanel::setSavePath(std::string const &path)
{
  this->saverUI->setRecordSavePath(path);
}


SourcePanel::~SourcePanel()
{
  this->clearGains();
  delete ui;
}

////////////////////////////////// Getters ///////////////////////////////////
bool
SourcePanel::getRecordState(void) const
{
  return this->saverUI->getRecordState();
}

std::string
SourcePanel::getAntenna(void) const
{
  return this->ui->antennaCombo->currentText().toStdString();
}

float
SourcePanel::getBandwidth(void) const
{
  return static_cast<float>(this->ui->bwSpin->value());
}

////////////////////////////////// Slots /////////////////////////////////////
void
SourcePanel::onGainChanged(QString name, float val)
{
  // TODO: Config gain in dialog

  this->setAGCEnabled(false);
  emit gainChanged(name, val);
}

void
SourcePanel::onRecordStartStop(void)
{
  emit toggleRecord();
}

void
SourcePanel::onThrottleChanged(void)
{
  bool throttling = this->ui->throttleCheck->isChecked();

  this->panelConfig->throttle = throttling;
  this->panelConfig->throttleRate = static_cast<unsigned>(this->ui->throttleSpin->value());

  this->ui->throttleSpin->setEnabled(throttling);

  emit throttleConfigChanged();
}

void
SourcePanel::onBandwidthChanged(void)
{
  emit bandwidthChanged();
}

void
SourcePanel::onChangeAutoGain(void)
{
  this->applyCurrentAutogain();
}

void
SourcePanel::onToggleAutoGain(void)
{
  if (this->ui->gainPresetCheck->isChecked()) {
    this->applyCurrentAutogain();
    this->ui->gainsFrame->setEnabled(false);
    this->ui->autoGainCombo->setEnabled(true);
    this->ui->autoGainSlider->setEnabled(true);
  } else {
    this->ui->gainsFrame->setEnabled(true);
    this->ui->autoGainCombo->setEnabled(false);
    this->ui->autoGainSlider->setEnabled(false);
  }
}

void
SourcePanel::onSelectAutoGain(void)
{
  this->selectAutoGain(
        static_cast<unsigned>(this->ui->autoGainCombo->currentIndex()));
  this->applyCurrentAutogain();
}

void
SourcePanel::onToggleDCRemove(void)
{
  this->setDCRemove(this->ui->dcRemoveCheck->isChecked());
  emit toggleDCRemove();
}

void
SourcePanel::onToggleIQReverse(void)
{
  this->setIQReverse(this->ui->swapIQCheck->isChecked());
  emit toggleIQReverse();
}

void
SourcePanel::onToggleAGCEnabled(void)
{
  this->setAGCEnabled(this->ui->agcEnabledCheck->isChecked());
  emit toggleAGCEnabled();
}

void
SourcePanel::onAntennaChanged(int i)
{
  emit antennaChanged(this->ui->antennaCombo->itemText(i));
}
