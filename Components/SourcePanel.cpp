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
#include <ProfileConfigTab.h>

using namespace SigDigger;

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

///////////////////////////////// Autogain config //////////////////////////////
void
AutoGainSetting::deserialize(Suscan::Object const &conf)
{
  LOAD(driver);
  LOAD(name);
  LOAD(value);
}

Suscan::Object &&
AutoGainSetting::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);
  Suscan::Object dataSaverConfig;

  obj.setClass("AutoGainSetting");

  STORE(driver);
  STORE(name);
  STORE(value);

  return this->persist(obj);
}
/////////////////////////////// Source panel config ////////////////////////////

void
SourcePanelConfig::deserialize(Suscan::Object const &conf)
{
  this->agcSettings.clear();

  LOAD(collapsed);
  LOAD(throttle);
  LOAD(throttleRate);
  LOAD(dcRemove);
  LOAD(iqRev);
  LOAD(agcEnabled);
  LOAD(gainPresetEnabled);

  try {
    Suscan::Object field = conf.getField("dataSaverConfig");
    this->dataSaverConfig->deserialize(field);
  } catch (Suscan::Exception const &) {

  }

  try {
    Suscan::Object list = conf.getField("SavedAutoGains");
    if (list.getType() == SUSCAN_OBJECT_TYPE_SET) {
      for (unsigned int i = 0; i < list.length(); ++i) {
        Suscan::Object field = list[i];
        if (field.getType() == SUSCAN_OBJECT_TYPE_OBJECT
            && field.getClass() == "AutoGainSetting") {
          try {
            AutoGainSetting agcSetting;

            agcSetting.deserialize(field);
            this->agcSettings[agcSetting.driver] = agcSetting;
          } catch (Suscan::Exception const &) { }
        }
      }

    }
  } catch (Suscan::Exception const &) { }
}

Suscan::Object &&
SourcePanelConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);
  Suscan::Object dataSaverConfig;
  Suscan::Object list(SUSCAN_OBJECT_TYPE_SET);

  obj.setClass("SourcePanelConfig");

  STORE(collapsed);
  STORE(throttle);
  STORE(throttleRate);
  STORE(dcRemove);
  STORE(iqRev);
  STORE(agcEnabled);
  STORE(gainPresetEnabled);

  dataSaverConfig = this->dataSaverConfig->serialize();

  obj.setField("dataSaverConfig", dataSaverConfig);

  for (auto p : this->agcSettings) {
    Suscan::Object serialized = p.second.serialize();
    list.append(serialized);
  }

  obj.setField("SavedAutoGains", list);

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
  this->ui->throttleSpin->setUnits("sps");
  this->ui->throttleSpin->setMinimum(0);

  this->assertConfig();
  this->connectAll();

  this->setProperty("collapsed", this->panelConfig->collapsed);
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
        SIGNAL(valueChanged(qreal)),
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

  connect(
        this->ui->ppmSpinBox,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onPPMChanged(void)));
}

void
SourcePanel::populateAntennaCombo(Suscan::AnalyzerSourceInfo const &info)
{
  int index = 0;
  int i = 0;
  QComboBox *combo = this->ui->antennaCombo;
  std::vector<std::string> antennaList;

  combo->clear();

  info.getAntennaList(antennaList);

  if (antennaList.empty()) {
    this->ui->antennaCombo->hide();
    this->ui->antennaLabel->hide();
  } else {
    this->ui->antennaCombo->show();
    this->ui->antennaLabel->show();
    for (auto p : antennaList) {
      combo->addItem(QString::fromStdString(p));

      if (info.getAntenna() == p)
        index = i;

      ++i;
    }

    combo->setCurrentIndex(index);
  }
}

QString
SourcePanel::formatSampleRate(unsigned int rate)
{
  return SuWidgetsHelpers::formatQuantity(rate, 4, "sp/s");
}

void
SourcePanel::refreshUi()
{
  bool gainPresetEnabled = this->panelConfig->gainPresetEnabled;
  bool haveAGC = this->currAutoGainSet != nullptr;

  if (this->profile != nullptr) {
    bool isRemote = this->profile->isRemote();

    this->setThrottleable(
          this->profile->getType() != SUSCAN_SOURCE_TYPE_SDR
          || isRemote);

    this->ui->antennaCombo->setEnabled(
          this->profile->getType() == SUSCAN_SOURCE_TYPE_SDR);

    this->ui->bwSpin->setEnabled(
          this->profile->getType() == SUSCAN_SOURCE_TYPE_SDR);

    this->ui->ppmSpinBox->setEnabled(
          this->profile->getType() == SUSCAN_SOURCE_TYPE_SDR
          || isRemote);

    this->saverUI->setEnabled(!isRemote);
  }

  // These depend on the source info only
  this->ui->dcRemoveCheck->setEnabled(
        this->sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_DC_REMOVE));
  this->ui->swapIQCheck->setEnabled(
        this->sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_IQ_REVERSE));
  this->ui->agcEnabledCheck->setEnabled(
        this->sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_AGC));

  // These depend both the profile and source info
  this->ui->bwSpin->setEnabled(
        this->ui->bwSpin->isEnabled()
        && this->sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_BW));
  this->ui->ppmSpinBox->setEnabled(
        this->ui->ppmSpinBox->isEnabled()
        && this->sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_PPM));
  this->ui->throttleCheck->setEnabled(
        this->ui->throttleCheck->isEnabled()
        && this->sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_THROTTLE));
  this->ui->throttleSpin->setEnabled(
        this->ui->throttleCheck->isChecked()
        && this->ui->throttleCheck->isEnabled());
  this->ui->antennaCombo->setEnabled(
        this->ui->antennaCombo->isEnabled()
        && this->sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_ANTENNA));
  this->ui->gainsFrame->setEnabled(
        (!gainPresetEnabled || !haveAGC)
        && this->sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_GAIN));
  this->ui->autoGainFrame->setEnabled(
        this->sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_GAIN));

  this->ui->autoGainCombo->setEnabled(gainPresetEnabled);
  this->ui->autoGainSlider->setEnabled(gainPresetEnabled);
}

void
SourcePanel::selectAntenna(std::string const &name)
{
  int index;
  QString qNam = QString::fromStdString(name);

  if ((index = this->ui->antennaCombo->findText(qNam)) == -1) {
    index = this->ui->antennaCombo->count();
    this->ui->antennaCombo->addItem(qNam);
  }

  this->ui->antennaCombo->setCurrentIndex(index);
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

    if (percentUsage <= 1) {
      this->ui->deliveryProgress->setValue(static_cast<int>(percentUsage * 100));
    } else {
      this->ui->deliveryProgress->setValue(100);
    }

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
SourcePanel::setPPM(float ppm)
{
  this->ui->ppmSpinBox->setValue(static_cast<qreal>(ppm));
}

void
SourcePanel::setProfile(Suscan::Source::Config *config)
{
  SUFLOAT bw;
  bool presetEnabled = this->ui->gainPresetCheck->isChecked();
  bool oldRefreshing = this->refreshing;
  this->refreshing = true;

  // Setting the profile resets the SourceInfo
  this->sourceInfo = Suscan::AnalyzerSourceInfo();

  this->profile = config;
  this->refreshGains(*config);
  this->refreshAutoGains(*config);

  // TODO: Move this somewhere else!!!!
  ProfileConfigTab::populateAntennaCombo(
        *config,
        this->ui->antennaCombo);

  if (this->ui->antennaCombo->count() == 0
      || config->getType() != SUSCAN_SOURCE_TYPE_SDR
      || config->getInterface() == SUSCAN_SOURCE_REMOTE_INTERFACE) {
    this->ui->antennaCombo->hide();
    this->ui->antennaLabel->hide();
  } else {
    this->ui->antennaCombo->show();
    this->ui->antennaLabel->show();
  }

  this->selectAntenna(config->getAntenna());
  this->setSampleRate(config->getDecimatedSampleRate());
  this->setDCRemove(config->getDCRemove());

  bw = this->profile->getBandwidth();
  if (SU_ABS(bw) < 1e-6f)
    bw = config->getDecimatedSampleRate();

  this->setBandwidth(bw);
  this->setPPM(this->profile->getPPM());

  // Reset the autogain configuration if a new profile is chosen
  if (presetEnabled)
    this->refreshCurrentAutoGain(config->getDevice().getDriver());
  else
    this->ui->gainsFrame->setEnabled(true);

  this->refreshUi();

  this->refreshing = oldRefreshing;
}

void
SourcePanel::setThrottleable(bool val)
{
  val = val && this->sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_THROTTLE);

  this->throttleable = val;
  this->ui->throttleCheck->setEnabled(val);
  if (!val)
    this->ui->throttleCheck->setChecked(false);

  this->ui->throttleSpin->setEnabled(
        this->ui->throttleCheck->isChecked()
        && this->ui->throttleCheck->isEnabled());

  this->ui->bwSpin->setEnabled(!val);
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
SourcePanel::setState(State state)
{
  if (this->state != state) {
    if (state == DETACHED) {
      this->haveSourceInfo = false;
      this->sourceInfo = Suscan::AnalyzerSourceInfo();
      this->refreshUi();
    }

    this->state = state;
  }
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

bool
SourcePanel::selectAutoGain(std::string const &name)
{
  int ndx;

  ndx = this->ui->autoGainCombo->findText(QString::fromStdString(name));

  if (ndx == -1)
    return false;

  this->ui->autoGainCombo->setCurrentIndex(ndx);
  this->selectAutoGain(static_cast<unsigned int>(ndx));

  return true;
}

void
SourcePanel::refreshCurrentAutoGain(std::string const &driver)
{
  bool enableGains = true;

  if (this->panelConfig->agcSettings.find(driver) !=
      this->panelConfig->agcSettings.end()) {
    AutoGainSetting setting = this->panelConfig->agcSettings[driver];

    if (this->selectAutoGain(setting.name)) {
      this->ui->autoGainSlider->setValue(setting.value);
      enableGains = false;
    } else {
      this->selectAutoGain(0);
    }
  } else {
    this->selectAutoGain(0);
  }

  this->ui->gainsFrame->setEnabled(enableGains);
}

void
SourcePanel::refreshAutoGains(Suscan::Source::Config &config)
{
  std::string driver = config.getDevice().getDriver();
  bool showFrame = false;

  this->ui->autoGainCombo->clear();

  if (this->autoGains.find(driver) != this->autoGains.end()) {
    this->currAutoGainSet = &this->autoGains[driver];
    this->currentAutoGain = nullptr;

    if (this->currAutoGainSet->size() > 0
        && config.getType() == SUSCAN_SOURCE_TYPE_SDR) {
      for (auto p = this->currAutoGainSet->begin();
           p != this->currAutoGainSet->end(); ++p)
        this->ui->autoGainCombo->addItem(
              QString::fromStdString(p->getName()));

      if (this->ui->gainPresetCheck->isEnabled())
        this->refreshCurrentAutoGain(driver);

      showFrame = true;
    }
  } else {
    this->currAutoGainSet = nullptr;
    this->currentAutoGain = nullptr;
  }

  this->ui->autoGainFrame->setVisible(showFrame);
}

void
SourcePanel::refreshGains(Suscan::Source::Config &config)
{
  Suscan::Source::Device const &dev = config.getDevice();
  DeviceGain *gain = nullptr;
  bool presetEnabled = this->ui->gainPresetCheck->isChecked();

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

  if (presetEnabled)
    this->refreshCurrentAutoGain(dev.getDriver());
  else
    this->ui->gainsFrame->setEnabled(true);
}

bool
SourcePanel::tryApplyGains(
    Suscan::AnalyzerSourceInfo const &info)
{
  std::vector<Suscan::Source::GainDescription> gains;
  DeviceGain *gain;
  unsigned int i;

  info.getGainInfo(gains);

  if (gains.size() != this->gainControls.size())
    return false;

  for (i = 0; i < gains.size(); ++i) {
    if ((gain = this->lookupGain(gains[i].getName())) == nullptr)
      return false;

    gain->setGain(gains[i].getDefault());
  }

  return true;
}

void
SourcePanel::applySourceInfo(Suscan::AnalyzerSourceInfo const &info)
{
  std::vector<Suscan::Source::GainDescription> gains;
  DeviceGain *gain = nullptr;
  bool presetEnabled = this->ui->gainPresetCheck->isChecked();
  bool oldRefreshing = this->refreshing;
  bool throttleEnabled;
  this->refreshing = true;

  if (!this->haveSourceInfo) {
    this->sourceInfo = Suscan::AnalyzerSourceInfo(info);
    this->haveSourceInfo = true;
  }

  this->setDCRemove(info.getDCRemove());
  this->setIQReverse(info.getIQReverse());
  this->setAGCEnabled(info.getAGC());
  this->setBandwidth(info.getBandwidth());

  throttleEnabled = !sufeq(
        info.getEffectiveSampleRate(),
        info.getSampleRate(),
        0); // Integer quantities

  this->ui->throttleCheck->setChecked(throttleEnabled);

  // Populate antennas
  populateAntennaCombo(info);

  // What if SoapySDR lies? We consider the case in which the antenna is
  // not reported in the antenna list
  this->selectAntenna(info.getAntenna());

  if (!this->tryApplyGains(info)) {
    // Recreate gains
    this->clearGains();

    info.getGainInfo(gains);

    for (auto p: gains) {
      gain = new DeviceGain(nullptr, p);
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
      gain->setGain(p.getDefault());
    }

    if (this->gainControls.size() == 0)
      this->ui->gainsFrame->hide();
    else
      this->ui->gainsFrame->show();
  }

  // AGC Enabled, we override gain settings with the current AGC
  // settings
  if (presetEnabled)
    this->applyCurrentAutogain();

  // Everything is set, time to decide what is enabled and what is not
  this->refreshUi();

  this->refreshing = oldRefreshing;
}

bool
SourcePanel::event(QEvent *event)
{
  if (event->type() == QEvent::DynamicPropertyChange) {
    QDynamicPropertyChangeEvent *const propEvent =
        static_cast<QDynamicPropertyChangeEvent*>(event);
    QString propName = propEvent->propertyName();
    if (propName == "collapsed")
      this->panelConfig->collapsed = this->property("collapsed").value<bool>();
  }

  return PersistentWidget::event(event);
}

void
SourcePanel::applyCurrentAutogain(void)
{
  if (this->currentAutoGain != nullptr
      && this->ui->gainPresetCheck->isChecked()) {
    AutoGainSetting agc;
    agc.driver = this->currentAutoGain->getDriver();
    agc.name   = this->currentAutoGain->getName();
    agc.value  = this->ui->autoGainSlider->value();

    this->panelConfig->agcSettings[agc.driver] = agc;

    std::vector<GainConfig> cfg =
        this->currentAutoGain->translateGain(agc.value);

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
  this->loadingConfig = true;

  this->ui->throttleCheck->setChecked(this->panelConfig->throttle);
  this->ui->dcRemoveCheck->setChecked(this->panelConfig->dcRemove);
  this->ui->swapIQCheck->setChecked(this->panelConfig->iqRev);
  this->ui->agcEnabledCheck->setChecked(this->panelConfig->agcEnabled);
  this->ui->throttleSpin->setValue(static_cast<int>(this->panelConfig->throttleRate));
  this->ui->gainPresetCheck->setChecked(this->panelConfig->gainPresetEnabled);

  this->setProperty("collapsed", this->panelConfig->collapsed);

  this->saverUI->applyConfig();

  this->loadingConfig = false;
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

float
SourcePanel::getPPM(void) const
{
  return static_cast<float>(this->ui->ppmSpinBox->value());
}

////////////////////////////////// Slots /////////////////////////////////////
void
SourcePanel::onGainChanged(QString name, float val)
{
  // TODO: Config gain in dialog

  if (!this->refreshing) {
    this->setAGCEnabled(false);
    emit gainChanged(name, val);
  }
}

void
SourcePanel::onRecordStartStop(void)
{
  emit toggleRecord();
}

void
SourcePanel::onThrottleChanged(void)
{
  if (!this->loadingConfig) {
    bool throttling = this->ui->throttleCheck->isChecked();

    this->panelConfig->throttle = throttling;
    this->panelConfig->throttleRate = static_cast<unsigned>(this->ui->throttleSpin->value());

    this->ui->throttleSpin->setEnabled(
          throttling && this->ui->throttleCheck->isChecked());

    emit throttleConfigChanged();
  }
}

void
SourcePanel::onBandwidthChanged(void)
{
  if (!this->refreshing)
    emit bandwidthChanged();
}

void
SourcePanel::onPPMChanged(void)
{
  if (!this->refreshing)
    emit ppmChanged();
}

void
SourcePanel::onChangeAutoGain(void)
{
  this->applyCurrentAutogain();
}

void
SourcePanel::onToggleAutoGain(void)
{
  this->panelConfig->gainPresetEnabled = this->ui->gainPresetCheck->isChecked();

  if (this->panelConfig->gainPresetEnabled)
    this->applyCurrentAutogain();

  this->refreshUi();
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
  if (!this->refreshing) {
    this->setDCRemove(this->ui->dcRemoveCheck->isChecked());
    emit toggleDCRemove();
  }
}

void
SourcePanel::onToggleIQReverse(void)
{
  if (!this->refreshing) {
    this->setIQReverse(this->ui->swapIQCheck->isChecked());
    emit toggleIQReverse();
  }
}

void
SourcePanel::onToggleAGCEnabled(void)
{
  if (!this->refreshing) {
    this->setAGCEnabled(this->ui->agcEnabledCheck->isChecked());
    emit toggleAGCEnabled();
  }
}

void
SourcePanel::onAntennaChanged(int i)
{
  if (!this->refreshing)
    emit antennaChanged(this->ui->antennaCombo->itemText(i));
}
