//
//    SourceWidget.cpp: Source control widget
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

#include "SourceWidgetFactory.h"
#include "SourceWidget.h"
#include "SuWidgetsHelpers.h"
#include "ui_SourceWidget.h"
#include <QMessageBox>
#include <FileDataSaver.h>
#include <fcntl.h>
#include <UIMediator.h>
#include <SigDiggerHelpers.h>

using namespace SigDigger;

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), field)
#define LOAD(field) field = conf.get(STRINGFY(field), field)

///////////////////////////////// Autogain config //////////////////////////////
void
GainPresetSetting::deserialize(Suscan::Object const &conf)
{
  LOAD(driver);
  LOAD(name);
  LOAD(value);
}

Suscan::Object &&
GainPresetSetting::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);
  Suscan::Object dataSaverConfig;

  obj.setClass("GainPresetSetting");

  STORE(driver);
  STORE(name);
  STORE(value);

  return persist(obj);
}
/////////////////////////////// Source widget config ////////////////////////////

void
SourceWidgetConfig::deserialize(Suscan::Object const &conf)
{
  agcSettings.clear();

  LOAD(collapsed);
  LOAD(throttle);
  LOAD(throttleRate);
  LOAD(dcRemove);
  LOAD(iqRev);
  LOAD(agcEnabled);
  LOAD(gainPresetEnabled);
  LOAD(allocHistory);
  LOAD(replayAllocationMiB);

  try {
    Suscan::Object field = conf.getField("dataSaverConfig");
    dataSaverConfig->deserialize(field);
  } catch (Suscan::Exception const &) {

  }

  try {
    Suscan::Object list = conf.getField("savedPresets");
    if (list.getType() == SUSCAN_OBJECT_TYPE_SET) {
      for (unsigned int i = 0; i < list.length(); ++i) {
        Suscan::Object field = list[i];
        if (field.getType() == SUSCAN_OBJECT_TYPE_OBJECT
            && field.getClass() == "GainPresetSetting") {
          try {
            GainPresetSetting agcSetting;

            agcSetting.deserialize(field);
            agcSettings[agcSetting.driver] = agcSetting;
          } catch (Suscan::Exception const &) { }
        }
      }

    }
  } catch (Suscan::Exception const &) { }
}

Suscan::Object &&
SourceWidgetConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);
  Suscan::Object dataSaverConfig;
  Suscan::Object list(SUSCAN_OBJECT_TYPE_SET);

  obj.setClass("SourceWidgetConfig");

  STORE(collapsed);
  STORE(throttle);
  STORE(throttleRate);
  STORE(dcRemove);
  STORE(iqRev);
  STORE(agcEnabled);
  STORE(gainPresetEnabled);
  STORE(allocHistory);
  STORE(replayAllocationMiB);

  dataSaverConfig = this->dataSaverConfig->serialize();

  obj.setField("dataSaverConfig", dataSaverConfig);

  for (auto p : agcSettings) {
    Suscan::Object serialized = p.second.serialize();
    list.append(serialized);
  }

  obj.setField("savedPresets", list);

  return persist(obj);
}


SourceWidgetConfig::~SourceWidgetConfig()
{
  if (dataSaverConfig != nullptr)
    delete dataSaverConfig;
}

SourceWidget::SourceWidget(
    SourceWidgetFactory *factory,
    UIMediator *mediator,
    QWidget *parent) :
  ToolWidget(factory, mediator, parent),
  m_ui(new Ui::SourcePanel)
{
  m_ui->setupUi(this);

  m_saverUI = new DataSaverUI(this);
  m_ui->dataSaverGrid->addWidget(m_saverUI);
  m_ui->throttleSpin->setUnits("sps");
  m_ui->throttleSpin->setMinimum(0);

  assertConfig();
  connectAll();

  setProperty("collapsed", m_panelConfig->collapsed);
}

SourceWidget::~SourceWidget()
{
  delete m_ui;
}

// Private methods
void
SourceWidget::connectAll(void)
{
  connect(
        m_ui->throttleCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onThrottleChanged(void)));

  connect(
        m_ui->throttleSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onThrottleChanged(void)));

  connect(
        m_saverUI,
        SIGNAL(recordStateChanged(bool)),
        this,
        SLOT(onRecordStartStop()));

  connect(
        m_ui->autoGainCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onSelectAutoGain(void)));

  connect(
        m_ui->autoGainSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onChangeAutoGain(void)));

  connect(
        m_ui->gainPresetCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onToggleAutoGain(void)));

  connect(
        m_ui->dcRemoveCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onToggleDCRemove(void)));

  connect(
        m_ui->swapIQCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onToggleIQReverse(void)));

  connect(
        m_ui->agcEnabledCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onToggleAGCEnabled(void)));

  connect(
        m_ui->antennaCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onAntennaChanged(int)));

  connect(
        m_ui->bwSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onBandwidthChanged(void)));

  connect(
        m_ui->ppmSpinBox,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onPPMChanged(void)));

  connect(
        m_ui->allocHistoryCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onAllocHistoryToggled()));

  connect(
        m_ui->allocSizeSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onAllocHistorySizeChanged()));

  connect(
        m_ui->replayButton,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleReplay()));
}


DeviceGain *
SourceWidget::lookupGain(std::string const &name)
{
  // Why is this? Use a map instead.
  for (auto p = m_gainControls.begin();
       p != m_gainControls.end();
       ++p) {
    if ((*p)->getName() == name)
      return *p;
  }

  return nullptr;
}

void
SourceWidget::clearGains(void)
{
  int i, len;

  len = static_cast<int>(m_gainControls.size());

  for (i = 0; i < len; ++i) {
    QLayoutItem *item = m_ui->gainGridLayout->takeAt(i);
    delete item;

    // This is what C++ is for.
    m_gainControls[static_cast<unsigned long>(i)]->deleteLater();
  }

  m_gainControls.clear();
}

void
SourceWidget::refreshGains(Suscan::Source::Config &config)
{
  Suscan::Source::Device const &dev = config.getDevice();
  DeviceGain *gain = nullptr;
  bool presetEnabled = m_ui->gainPresetCheck->isChecked();

  clearGains();

  for (auto p = dev.getFirstGain();
       p != dev.getLastGain();
       ++p) {
    gain = new DeviceGain(nullptr, *p);
    m_gainControls.push_back(gain);
    m_ui->gainGridLayout->addWidget(
          gain,
          static_cast<int>(m_gainControls.size() - 1),
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

  if (m_gainControls.size() == 0 || !config.isRealTime())
    m_ui->gainsFrame->hide();
  else
    m_ui->gainsFrame->show();

  if (presetEnabled)
    refreshCurrentAutoGain(dev.getDriver());
  else
    m_ui->gainsFrame->setEnabled(true);
}

bool
SourceWidget::tryApplyGains(
    Suscan::AnalyzerSourceInfo const &info)
{
  std::vector<Suscan::Source::GainDescription> gains;
  DeviceGain *gain;
  unsigned int i;

  info.getGainInfo(gains);

  if (gains.size() != m_gainControls.size())
    return false;

  for (i = 0; i < gains.size(); ++i) {
    if ((gain = lookupGain(gains[i].getName())) == nullptr)
      return false;

    gain->setGain(gains[i].getDefault());
  }

  return true;
}

void
SourceWidget::applySourceInfo(Suscan::AnalyzerSourceInfo const &info)
{
  std::vector<Suscan::Source::GainDescription> gains;
  DeviceGain *gain = nullptr;
  bool oldBlocking;

  oldBlocking = setBlockingSignals(true);

  // There are 5 source settings that are overriden by the UI configuration
  //
  // 1. Gain presets
  // 2. Throttle
  // 3. DC Removal
  // 4. IQ Reversal
  // 5. AGC
  //
  // These settings are set once the first source info is received, and
  // refresh the UI in subsequent receptions.

  setSampleRate(SCAST(unsigned, info.getSampleRate()));

  if (info.getMeasuredSampleRate() > 0)
    setProcessRate(SCAST(unsigned, info.getMeasuredSampleRate()));

  if (!m_haveSourceInfo) {
    m_sourceInfo = Suscan::AnalyzerSourceInfo(info);
    m_haveSourceInfo = true;

    // First source info! Set delayed analyzer options (this ones are
    // not bound to an analyzer)

    setDelayedAnalyzerOptions();
  } else {
    bool throttleEnabled = !sufeq(
          info.getEffectiveSampleRate(),
          info.getSampleRate(),
          0); // Integer quantities

    m_ui->throttleCheck->setChecked(throttleEnabled);

    setDCRemove(info.getDCRemove());
    setIQReverse(info.getIQReverse());
    setAGCEnabled(info.getAGC());
  }

  BLOCKSIG(m_ui->replayButton, setChecked(info.replayMode()));

  setBandwidth(info.getBandwidth());
  setPPM(info.getPPM());

  // Populate antennas
  populateAntennaCombo(info);

  // What if SoapySDR lies? We consider the case in which the antenna is
  // not reported in the antenna list
  selectAntenna(info.getAntenna());

  if (!tryApplyGains(info)) {
    // Recreate gains
    clearGains();

    info.getGainInfo(gains);
    for (auto p: gains) {
      gain = new DeviceGain(nullptr, p);
      m_gainControls.push_back(gain);
      m_ui->gainGridLayout->addWidget(
            gain,
            static_cast<int>(m_gainControls.size() - 1),
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
  }

  if (m_gainControls.size() == 0)
    m_ui->gainsFrame->hide();
  else
    m_ui->gainsFrame->show();

  // Everything is set, time to decide what is enabled and what is not
  refreshUi();

  setBlockingSignals(oldBlocking);
}

void
SourceWidget::applyCurrentAutogain(void)
{
  if (m_currentAutoGain != nullptr
      && m_ui->gainPresetCheck->isChecked()) {
    GainPresetSetting agc;
    agc.driver = m_currentAutoGain->getDriver();
    agc.name   = m_currentAutoGain->getName();
    agc.value  = m_ui->autoGainSlider->value();

    m_panelConfig->agcSettings[agc.driver] = agc;

    std::vector<GainConfig> cfg =
        m_currentAutoGain->translateGain(agc.value);

    for (auto p = cfg.begin();
         p != cfg.end();
         ++p) {
      DeviceGain *gain = lookupGain(p->name);
      if (gain != nullptr) {
        gain->setGain(static_cast<float>(p->value));
        onGainChanged(
              QString::fromStdString(p->name),
              static_cast<float>(p->value));
      }
    }
  }
}

void
SourceWidget::applyCurrentProfileGains()
{
  if (m_profile != nullptr) {
    Suscan::Source::Device const &dev = m_profile->getDevice();
    for (auto p = dev.getFirstGain();
         p != dev.getLastGain();
         ++p) {
      auto name = p->getName();
      auto value = m_profile->getGain(name);

      DeviceGain *gain = lookupGain(name);
      if (gain != nullptr) {
        gain->setGain(static_cast<float>(m_profile->getGain(name)));
        onGainChanged(
              QString::fromStdString(name),
              static_cast<float>(value));
      }
    }
  }
}

void
SourceWidget::refreshCurrentAutoGain(std::string const &driver)
{
  bool enableGains = true;

  if (m_panelConfig->agcSettings.find(driver) !=
      m_panelConfig->agcSettings.end()) {
    GainPresetSetting setting = m_panelConfig->agcSettings[driver];

    if (selectAutoGain(setting.name)) {
      m_ui->autoGainSlider->setValue(setting.value);
      enableGains = false;
    } else {
      selectAutoGain(0);
    }
  } else {
    selectAutoGain(0);
  }

  m_ui->gainsFrame->setEnabled(enableGains);
}

void
SourceWidget::setBandwidth(float bw)
{
  m_ui->bwSpin->setValue(static_cast<qreal>(bw));
}

void
SourceWidget::setPPM(float ppm)
{
  m_ui->ppmSpinBox->setValue(static_cast<qreal>(ppm));
}

void
SourceWidget::refreshUi()
{
  bool gainPresetEnabled = m_panelConfig->gainPresetEnabled;
  bool replayEnabled = m_panelConfig->allocHistory;
  bool canReplay = replayEnabled;
  bool haveAGC = m_currAutoGainSet != nullptr;

  if (m_profile != nullptr) {
    bool isRemote = m_profile->isRemote();

    setThrottleable(!m_profile->isRealTime() || isRemote);
    m_ui->antennaCombo->setEnabled(m_profile->isRealTime());
    m_ui->bwSpin->setEnabled(m_profile->isRealTime());
    m_ui->ppmSpinBox->setEnabled(m_profile->isRealTime() || isRemote);
    m_saverUI->setEnabled(!isRemote);
  }

  // These depend on the source info only
  m_ui->dcRemoveCheck->setEnabled(
        m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_DC_REMOVE));
  m_ui->swapIQCheck->setEnabled(
        m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_IQ_REVERSE));
  m_ui->agcEnabledCheck->setEnabled(
        m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_AGC));

  // These depend both the profile and source info
  m_ui->bwSpin->setEnabled(
        m_ui->bwSpin->isEnabled()
        && m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_BW));
  m_ui->ppmSpinBox->setEnabled(
        m_ui->ppmSpinBox->isEnabled()
        && m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_PPM));
  m_ui->throttleCheck->setEnabled(
        m_ui->throttleCheck->isEnabled()
        && m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_THROTTLE));
  m_ui->throttleSpin->setEnabled(
        m_ui->throttleCheck->isChecked()
        && m_ui->throttleCheck->isEnabled());
  m_ui->antennaCombo->setEnabled(
        m_ui->antennaCombo->isEnabled()
        && m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_ANTENNA));
  m_ui->gainsFrame->setEnabled(
        (!gainPresetEnabled || !haveAGC)
        && m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_GAIN));
  m_ui->autoGainFrame->setEnabled(
        m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_GAIN));

  if (m_analyzer == nullptr) {
    canReplay = false;
  } else if (m_haveSourceInfo
             && (m_sourceInfo.isSeekable() || m_sourceInfo.historyLength() == 0)) {
    canReplay = false;
  }


  // History
  m_ui->allocHistoryCheck->setEnabled(
        m_analyzer == nullptr || !m_sourceInfo.isSeekable());
  m_ui->replayTimeProgress->setEnabled(m_haveSourceInfo && m_sourceInfo.replayMode());

  if (!m_sourceInfo.replayMode()) {
    m_ui->replayTimeProgress->setValue(0);
    m_ui->replayTimeProgress->setFormat("Idle");
  }

  m_ui->replayButton->setEnabled(canReplay);
  m_ui->replayButton->setStyleSheet(
        canReplay ? "color: white;\nbackground-color: #16448c;" : "");
  m_ui->autoGainCombo->setEnabled(gainPresetEnabled);
  m_ui->autoGainSlider->setEnabled(gainPresetEnabled);
}

void
SourceWidget::selectAntenna(std::string const &name)
{
  int index;
  QString qNam = QString::fromStdString(name);

  if ((index = m_ui->antennaCombo->findText(qNam)) == -1) {
    index = m_ui->antennaCombo->count();
    m_ui->antennaCombo->addItem(qNam);
  }

  m_ui->antennaCombo->setCurrentIndex(index);
}

void
SourceWidget::setSampleRate(unsigned int rate)
{
  if (m_rate != rate) {
    float step;
    m_rate = rate;
    if (rate == 0) {
      setProcessRate(0);
      m_ui->sampleRateLabel->setText("N/A");
    } else {
      m_ui->sampleRateLabel->setText(
            SuWidgetsHelpers::formatQuantity(rate, 4, "sp/s"));
    }

    m_ui->bwSpin->setMaximum(m_rate);

    step = SU_POW(10., SU_FLOOR(SU_LOG(m_rate)));

    if (step >= 10.f)
      step /= 10.f;
  }
}

void
SourceWidget::populateAntennaCombo(Suscan::AnalyzerSourceInfo const &info)
{
  int index = 0;
  int i = 0;
  QComboBox *combo = m_ui->antennaCombo;
  std::vector<std::string> antennaList;

  combo->clear();

  info.getAntennaList(antennaList);

  if (antennaList.empty()) {
    m_ui->antennaCombo->hide();
    m_ui->antennaLabel->hide();
  } else {
    m_ui->antennaCombo->show();
    m_ui->antennaLabel->show();
    for (auto p : antennaList) {
      combo->addItem(QString::fromStdString(p));

      if (info.getAntenna() == p)
        index = i;

      ++i;
    }

    combo->setCurrentIndex(index);
  }
}

void
SourceWidget::setThrottleable(bool val)
{
  val = val && m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_THROTTLE);

  m_throttleable = val;
  m_ui->throttleCheck->setEnabled(val);
  if (!val)
    m_ui->throttleCheck->setChecked(false);

  m_ui->throttleSpin->setEnabled(
        m_ui->throttleCheck->isChecked()
        && m_ui->throttleCheck->isEnabled());

  m_ui->bwSpin->setEnabled(!val);
}

unsigned int
SourceWidget::getEffectiveRate(void) const
{
  return m_throttleable && m_panelConfig->throttle
      ? m_panelConfig->throttleRate
      : m_rate;
}

void
SourceWidget::setProcessRate(unsigned int rate)
{
  if (rate != m_processRate) {
    SUFLOAT percentUsage = 1;
    m_processRate = rate;

    if (m_rate == 0 || m_processRate == 0) {
      m_ui->processingRateLabel->setText("N/A");
      m_ui->deliveryProgress->setEnabled(false);
    } else {
      m_ui->deliveryProgress->setEnabled(true);
      m_ui->processingRateLabel->setText(
            SuWidgetsHelpers::formatQuantity(m_processRate, 4, "sp/s"));
      percentUsage =
          static_cast<SUFLOAT>(m_processRate) /
          static_cast<SUFLOAT>(getEffectiveRate());
    }

    if (percentUsage <= 1) {
      m_ui->deliveryProgress->setValue(static_cast<int>(percentUsage * 100));
    } else {
      m_ui->deliveryProgress->setValue(100);
    }

    if (percentUsage >= SU_ADDSFX(.95))
      m_ui->deliveryLabel->setPixmap(
          QPixmap(QString::fromUtf8(":/icons/transparent.png")));
    else if (percentUsage >= SU_ADDSFX(.85))
      m_ui->deliveryLabel->setPixmap(
          QPixmap(QString::fromUtf8(":/icons/warning.png")));
    else
      m_ui->deliveryLabel->setPixmap(
          QPixmap(QString::fromUtf8(":/icons/critical.png")));
  }
}

void
SourceWidget::refreshAutoGains(Suscan::Source::Config &config)
{
  std::string driver = config.getDevice().getDriver();
  bool showFrame = false;

  m_ui->autoGainCombo->clear();

  if (m_autoGains.find(driver) != m_autoGains.end()) {
    m_currAutoGainSet = &m_autoGains[driver];
    m_currentAutoGain = nullptr;

    if (m_currAutoGainSet->size() > 0 && config.isRealTime()) {
      for (auto p = m_currAutoGainSet->begin();
           p != m_currAutoGainSet->end(); ++p)
        m_ui->autoGainCombo->addItem(
              QString::fromStdString(p->getName()));

      if (m_ui->gainPresetCheck->isEnabled())
        refreshCurrentAutoGain(driver);

      showFrame = true;
    }
  } else {
    m_currAutoGainSet = nullptr;
    m_currentAutoGain = nullptr;
  }

  m_ui->autoGainFrame->setVisible(showFrame);
}

void
SourceWidget::setCaptureSize(quint64 size)
{
  m_saverUI->setCaptureSize(size);
}

void
SourceWidget::setIORate(qreal rate)
{
  m_saverUI->setIORate(rate);
}

void
SourceWidget::setRecordState(bool state)
{
  m_saverUI->setRecordState(state);
}

void
SourceWidget::setDCRemove(bool remove)
{
  m_ui->dcRemoveCheck->setChecked(remove);
  m_panelConfig->dcRemove = remove;
}

void
SourceWidget::setAGCEnabled(bool enabled)
{
  m_ui->agcEnabledCheck->setChecked(enabled);
  m_panelConfig->agcEnabled = enabled;
}

void
SourceWidget::setIQReverse(bool rev)
{
  m_ui->swapIQCheck->setChecked(rev);
  m_panelConfig->iqRev = rev;
}

void
SourceWidget::setSavePath(std::string const &path)
{
  m_saverUI->setRecordSavePath(path);
}

void
SourceWidget::selectAutoGain(unsigned int gain)
{
  if (m_currAutoGainSet != nullptr
      && gain < m_currAutoGainSet->size()) {
    m_currentAutoGain = &(*m_currAutoGainSet)[gain];
    m_ui->autoGainSlider->setMinimum(m_currentAutoGain->getMin());
    m_ui->autoGainSlider->setMaximum(m_currentAutoGain->getMax());
  }
}

bool
SourceWidget::selectAutoGain(std::string const &name)
{
  int ndx;

  ndx = m_ui->autoGainCombo->findText(QString::fromStdString(name));

  if (ndx == -1)
    return false;

  m_ui->autoGainCombo->setCurrentIndex(ndx);
  selectAutoGain(static_cast<unsigned int>(ndx));

  return true;
}

void
SourceWidget::deserializeAutoGains(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  for (auto i = sus->getFirstAutoGain();
       i != sus->getLastAutoGain();
       i++) {
    AutoGain ag(*i);
    m_autoGains[ag.getDriver()].push_back(ag);
  }
}

// Configuration methods
Suscan::Serializable *
SourceWidget::allocConfig()
{
  m_panelConfig = new SourceWidgetConfig();
  m_panelConfig->dataSaverConfig = m_saverUI->allocConfig();

  return m_panelConfig;
}

void
SourceWidget::applyConfig()
{
  BLOCKSIG(m_ui->throttleSpin, setValue(static_cast<int>(m_panelConfig->throttleRate)));
  BLOCKSIG(m_ui->throttleCheck, setChecked(m_panelConfig->throttle));

  onThrottleChanged();

  BLOCKSIG(m_ui->dcRemoveCheck, setChecked(m_panelConfig->dcRemove));
  BLOCKSIG(m_ui->swapIQCheck, setChecked(m_panelConfig->iqRev));
  BLOCKSIG(m_ui->agcEnabledCheck, setChecked(m_panelConfig->agcEnabled));
  BLOCKSIG(m_ui->gainPresetCheck, setChecked(m_panelConfig->gainPresetEnabled));
  BLOCKSIG(m_ui->allocHistoryCheck, setChecked(m_panelConfig->allocHistory));
  BLOCKSIG(m_ui->allocSizeSpin, setValue(m_panelConfig->replayAllocationMiB));

  setProperty("collapsed", m_panelConfig->collapsed);

  m_saverUI->applyConfig();

  deserializeAutoGains();
}

bool
SourceWidget::event(QEvent *event)
{
  if (event->type() == QEvent::DynamicPropertyChange) {
    QDynamicPropertyChangeEvent *const propEvent =
        static_cast<QDynamicPropertyChangeEvent*>(event);
    QString propName = propEvent->propertyName();
    if (propName == "collapsed")
      m_panelConfig->collapsed = property("collapsed").value<bool>();
  }

  return ToolWidget::event(event);
}

bool
SourceWidget::setBlockingSignals(bool blocking)
{
  bool oldState;

#define SETBLOCKING(widget) m_ui->widget->blockSignals(blocking)

  oldState = SETBLOCKING(agcEnabledCheck);
  SETBLOCKING(throttleCheck);
  SETBLOCKING(bwSpin);
  SETBLOCKING(ppmSpinBox);
  SETBLOCKING(gainPresetCheck);
  SETBLOCKING(autoGainCombo);
  SETBLOCKING(autoGainSlider);
  SETBLOCKING(dcRemoveCheck);
  SETBLOCKING(swapIQCheck);
  SETBLOCKING(agcEnabledCheck);
  SETBLOCKING(antennaCombo);

  for (auto p : m_gainControls)
    p->blockSignals(blocking);

#undef SETBLOCKING

  return oldState;
}

void
SourceWidget::setDelayedAnalyzerOptions()
{
  if (m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_THROTTLE))
    onThrottleChanged();

  if (m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_DC_REMOVE))
    onToggleDCRemove();

  if (m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_IQ_REVERSE))
    onToggleIQReverse();

  if (m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_AGC))
    onToggleAGCEnabled();
}

// Overriden methods
void
SourceWidget::setState(int state, Suscan::Analyzer *analyzer)
{
  if (m_analyzer != analyzer) {
    // Uninstall any datasaver
    uninstallDataSaver();
    m_filterInstalled = false; // The filter is not installed anymore.

    m_analyzer = analyzer;

    m_haveSourceInfo = false;

    if (m_analyzer == nullptr) {
      m_sourceInfo = Suscan::AnalyzerSourceInfo();
      setProcessRate(0);
    } else {
      // Switched to running! Then, do the following:
      // 1. Connect source_info_message
      // 2. If recording is enabled, go ahead.
      // 3. Upon the reception of the first source info message: apply delayed

      connect(
            analyzer,
            SIGNAL(source_info_message(const Suscan::SourceInfoMessage &)),
            this,
            SLOT(onSourceInfoMessage(const Suscan::SourceInfoMessage &)));
      connect(
            analyzer,
            SIGNAL(psd_message(const Suscan::PSDMessage &)),
            this,
            SLOT(onPSDMessage(const Suscan::PSDMessage &)));

      onRecordStartStop();

      // First presence of analyzer!
      adjustHistoryConfig();
    }

    m_ui->replayButton->setChecked(false);
    refreshUi();
  }

  m_state = state;
}

void
SourceWidget::setProfile(Suscan::Source::Config &profile)
{
  SUFLOAT bw;
  bool presetEnabled = m_ui->gainPresetCheck->isChecked();
  bool oldBlocking;

  oldBlocking = setBlockingSignals(true);

  // Setting the profile resets the SourceInfo
  m_sourceInfo = Suscan::AnalyzerSourceInfo();

  m_profile = &profile;
  refreshGains(profile);
  refreshAutoGains(profile);

  // TODO: Move this somewhere else!!!!
  SigDiggerHelpers::populateAntennaCombo(
        profile,
        m_ui->antennaCombo);

  if (m_ui->antennaCombo->count() == 0
      || !profile.isRealTime()
      || profile.getInterface() == SUSCAN_SOURCE_REMOTE_INTERFACE) {
    m_ui->antennaCombo->hide();
    m_ui->antennaLabel->hide();
  } else {
    m_ui->antennaCombo->show();
    m_ui->antennaLabel->show();
  }

  selectAntenna(profile.getAntenna());
  setSampleRate(profile.getDecimatedSampleRate());
  setDCRemove(profile.getDCRemove());

  bw = m_profile->getBandwidth();
  if (SU_ABS(bw) < 1e-6f)
    bw = profile.getDecimatedSampleRate();

  setBandwidth(bw);
  setPPM(m_profile->getPPM());

  // Reset the autogain configuration if a new profile is chosen
  if (presetEnabled)
    refreshCurrentAutoGain(profile.getDevice().getDriver());
  else
    m_ui->gainsFrame->setEnabled(true);

  refreshUi();

  setBlockingSignals(oldBlocking);
}

void
SourceWidget::adjustHistoryConfig()
{
  if (m_analyzer != nullptr) {
    if (m_panelConfig->allocHistory) {
      m_analyzer->setHistorySize(
            m_panelConfig->replayAllocationMiB * (1 << 20));
    } else {
      m_analyzer->setHistorySize(0);
    }
  }
}

//////////////////////////////// Data saving ///////////////////////////////////
int
SourceWidget::openCaptureFile(void)
{
  int fd = -1;
  char baseName[80];
  char datetime[17];
  time_t unixtime;
  struct tm tm;

  if (m_profile == nullptr)
    return -1;

  unixtime = time(nullptr);
  gmtime_r(&unixtime, &tm);
  strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M%SZ", &tm);

  snprintf(
        baseName,
        sizeof(baseName),
        "sigdigger_%s_%d_%.0lf_float32_iq.raw",
        datetime,
        m_profile->getDecimatedSampleRate(),
        m_mediator->getCurrentCenterFreq());

  std::string fullPath =
      m_saverUI->getRecordSavePath() + "/" + baseName;

  if ((fd = creat(fullPath.c_str(), 0600)) == -1) {
    QMessageBox::warning(
              this,
              "SigDigger error",
              "Failed to open capture file for writing: " +
              QString(strerror(errno)),
              QMessageBox::Ok);
  }

  return fd;
}

void
SourceWidget::uninstallDataSaver()
{
  if (m_dataSaver != nullptr)
    delete m_dataSaver;

  m_dataSaver = nullptr;
}

void
SourceWidget::connectDataSaver()
{
  connect(
        m_dataSaver,
        SIGNAL(stopped()),
        this,
        SLOT(onSaveError()));

  connect(
        m_dataSaver,
        SIGNAL(swamped()),
        this,
        SLOT(onSaveSwamped()));

  connect(
        m_dataSaver,
        SIGNAL(dataRate(qreal)),
        this,
        SLOT(onSaveRate(qreal)));

  connect(
        m_dataSaver,
        SIGNAL(commit()),
        this,
        SLOT(onCommit()));
}

// I wish this could be static
SUBOOL
SigDigger::onBaseBandData(
    void *privdata,
    suscan_analyzer_t *,
    SUCOMPLEX *samples,
    SUSCOUNT length,
    SUSCOUNT)
{
  SourceWidget *widget = static_cast<SourceWidget *>(privdata);
  FileDataSaver *saver;

  if ((saver = widget->m_dataSaver) != nullptr)
    saver->write(samples, length);

  return SU_TRUE;
}

void
SourceWidget::installDataSaver(int fd)
{
  if (m_dataSaver == nullptr) {
    if (m_profile != nullptr && m_analyzer != nullptr) {
      m_dataSaver = new FileDataSaver(fd, this);
      m_dataSaver->setSampleRate(m_profile->getDecimatedSampleRate());

      if (!m_filterInstalled) {
        m_analyzer->registerBaseBandFilter(onBaseBandData, this);
        m_filterInstalled = true;
      }

      connectDataSaver();
    }
  }
}


////////////////////////////////////// Slots ///////////////////////////////////
void
SourceWidget::onSourceInfoMessage(Suscan::SourceInfoMessage const &msg)
{
  applySourceInfo(*msg.info());
}

void
SourceWidget::onPSDMessage(Suscan::PSDMessage const &msg)
{
  setSampleRate(msg.getSampleRate());
  setProcessRate(msg.getMeasuredSampleRate());
}

void
SourceWidget::onGainChanged(QString name, float val)
{
  setAGCEnabled(false);

  if (m_profile != nullptr)
    m_profile->setGain(name.toStdString(), val);

  if (m_analyzer != nullptr) {
    try {
      m_analyzer->setGain(name.toStdString(), val);
    } catch (Suscan::Exception &) {
    (void)  QMessageBox::critical(
          this,
          "SigDigger error",
          "Source does not allow adjusting gain settings",
          QMessageBox::Ok);
    }
  }
}

void
SourceWidget::onRecordStartStop(void)
{
  if (m_analyzer != nullptr) {
    bool recordState =  m_saverUI->isEnabled()
        && m_saverUI->getRecordState();

    if (recordState) {
      int fd = openCaptureFile();
      if (fd != -1)
        installDataSaver(fd);
      setRecordState(fd != -1);
    } else {
      uninstallDataSaver();
      setCaptureSize(0);
      setRecordState(false);
    }
  }
}

void
SourceWidget::onThrottleChanged(void)
{
  bool throttling = m_ui->throttleCheck->isChecked();

  m_panelConfig->throttle = throttling;
  m_panelConfig->throttleRate = static_cast<unsigned>(
        m_ui->throttleSpin->value());

  m_ui->throttleSpin->setEnabled(
        throttling && m_ui->throttleCheck->isChecked());

  if (m_analyzer != nullptr) {
    try {
      m_analyzer->setThrottle(throttling ? m_panelConfig->throttleRate : 0);
    } catch (Suscan::Exception &) {
      (void)  QMessageBox::critical(
            this,
            "SigDigger error",
            "Source does not allow changing the throttle config",
            QMessageBox::Ok);
    }
  }
}

void
SourceWidget::onBandwidthChanged(void)
{
  float bandwidth = SCAST(float, m_ui->bwSpin->value());

  if (m_profile != nullptr)
    m_profile->setBandwidth(bandwidth);

  if (m_analyzer != nullptr) {
    try {
      m_analyzer->setBandwidth(bandwidth);
    } catch (Suscan::Exception &) {
      (void)  QMessageBox::critical(
            this,
            "SigDigger error",
            "Source does not allow setting the bandwidth",
            QMessageBox::Ok);
    }
  }
}

void
SourceWidget::onPPMChanged(void)
{
  float ppm = SCAST(float, m_ui->ppmSpinBox->value());

  if (m_profile !=  nullptr)
    m_profile->setPPM(ppm);

  if (m_analyzer != nullptr) {
    try {
      m_analyzer->setPPM(ppm);
    } catch (Suscan::Exception &) {
      (void)  QMessageBox::critical(
            this,
            "SigDigger error",
            "Source does not allow adjusting the receiver's PPM",
            QMessageBox::Ok);
    }
  }
}

void
SourceWidget::onChangeAutoGain(void)
{
  // This is just a shortcut to per-gain settings
  applyCurrentAutogain();
}

void
SourceWidget::onToggleAutoGain(void)
{
  m_panelConfig->gainPresetEnabled = m_ui->gainPresetCheck->isChecked();

  if (m_panelConfig->gainPresetEnabled)
    applyCurrentAutogain();

  refreshUi();
}

void
SourceWidget::onSelectAutoGain(void)
{
  selectAutoGain(
        static_cast<unsigned>(m_ui->autoGainCombo->currentIndex()));
  applyCurrentAutogain();
}

void
SourceWidget::onToggleDCRemove(void)
{
  setDCRemove(m_ui->dcRemoveCheck->isChecked());

  if (m_analyzer != nullptr) {
    try {
      m_analyzer->setDCRemove(m_panelConfig->dcRemove);
    } catch (Suscan::Exception &) {
      (void)  QMessageBox::critical(
            this,
            "SigDigger error",
            "Source does not allow toggling the DC setting",
            QMessageBox::Ok);
    }
  }
}

void
SourceWidget::onToggleIQReverse(void)
{
  setIQReverse(m_ui->swapIQCheck->isChecked());

  if (m_analyzer != nullptr) {
    try {
      m_analyzer->setIQReverse(m_panelConfig->iqRev);
    } catch (Suscan::Exception &) {
      (void)  QMessageBox::critical(
            this,
            "SigDigger error",
            "Source does not allow reversing the IQ components",
            QMessageBox::Ok);
    }
  }
}

void
SourceWidget::onToggleAGCEnabled(void)
{
  setAGCEnabled(m_ui->agcEnabledCheck->isChecked());

  if (m_analyzer != nullptr) {
    try {
      m_analyzer->setAGC(m_panelConfig->agcEnabled);
    } catch (Suscan::Exception &) {
      (void)  QMessageBox::critical(
            this,
            "SigDigger error",
            "Source does not allow toggling the AGC setting",
            QMessageBox::Ok);
    }

    if (!m_panelConfig->agcEnabled &&
        m_sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_GAIN)) {
      // restore the manual gain settings
      if (m_panelConfig->gainPresetEnabled)
        applyCurrentAutogain();
      else
        applyCurrentProfileGains();
    }
  }
}

void
SourceWidget::onAntennaChanged(int)
{
  int i = m_ui->antennaCombo->currentIndex();

  if (i >= 0) {
    std::string antenna = m_ui->antennaCombo->itemText(i).toStdString();

    if (m_profile != nullptr)
      m_profile->setAntenna(antenna);

    if (m_analyzer != nullptr) {
      try {
        m_analyzer->setAntenna(antenna);
      } catch (Suscan::Exception &) {
        (void)  QMessageBox::critical(
              this,
              "SigDigger error",
              "Source does not allow changing the current RX antenna",
              QMessageBox::Ok);
      }
    }
  }
}

//////////////////////////// Datasaver slots ///////////////////////////////////
void
SourceWidget::onSaveError(void)
{
  if (m_dataSaver != nullptr) {
    uninstallDataSaver();

    QMessageBox::warning(
              this,
              "SigDigger error",
              "Capture file write error. Disk full?",
              QMessageBox::Ok);

    setRecordState(false);
  }
}

void
SourceWidget::onSaveSwamped(void)
{
  if (m_dataSaver != nullptr) {
    uninstallDataSaver();
    SU_WARNING("Capture thread swamped. Maybe the selected storage device is too slow.\n");
    int fd = openCaptureFile();
    if (fd != -1) {
      SU_WARNING("Capture restarted.\n");
      installDataSaver(fd);
    } else {
      QMessageBox::warning(
            this,
            "SigDigger error",
            "Capture swamped, but failed to reopen the capture file.",
            QMessageBox::Ok);
      setRecordState(false);
    }
  }
}

void
SourceWidget::onSaveRate(qreal rate)
{
  if (m_dataSaver != nullptr)
    setIORate(rate);
}

void
SourceWidget::onCommit(void)
{
  if (m_dataSaver != nullptr)
    setCaptureSize(m_dataSaver->getSize());
}

void
SourceWidget::onAllocHistoryToggled()
{
  m_panelConfig->allocHistory = m_ui->allocHistoryCheck->isChecked();
  adjustHistoryConfig();
  refreshUi();
}

void
SourceWidget::onAllocHistorySizeChanged()
{
  m_panelConfig->replayAllocationMiB = m_ui->allocSizeSpin->value();
  adjustHistoryConfig();
}

void
SourceWidget::onToggleReplay()
{
  if (m_analyzer != nullptr)
    m_analyzer->replay(m_ui->replayButton->isChecked());
}

