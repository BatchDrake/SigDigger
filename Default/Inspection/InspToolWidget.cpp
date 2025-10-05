//
//    InspToolWidget.cpp: Dockable inspector panel
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

#include "InspToolWidgetFactory.h"
#include "InspToolWidget.h"
#include "InspectionWidgetFactory.h"
#include "ui_InspToolWidget.h"
#include <UIMediator.h>
#include <MainSpectrum.h>
#include <SuWidgetsHelpers.h>
#include <QMessageBox>
#include <QAction>
#include <QIcon>

using namespace SigDigger;

///////////////////////////// Inspector panel cnfig ////////////////////////////
#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), field)
#define LOAD(field) field = conf.get(STRINGFY(field), field)

void
InspToolWidgetConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(collapsed);
  LOAD(inspectorClass);
  LOAD(inspFactory);
  LOAD(precise);
  LOAD(palette);
  LOAD(paletteOffset);
  LOAD(paletteContrast);
  LOAD(autoSquelchTriggerSNR);
}

Suscan::Object &&
InspToolWidgetConfig::serialize()
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("InspToolWidgetConfig");

  STORE(collapsed);
  STORE(inspectorClass);
  STORE(inspFactory);
  STORE(precise);
  STORE(palette);
  STORE(paletteOffset);
  STORE(paletteContrast);
  STORE(autoSquelchTriggerSNR);

  return persist(obj);
}

////////////////////////// Inspector panel widget //////////////////////////////
Suscan::Serializable *
InspToolWidget::allocConfig()
{
  if (m_timeWindow == nullptr)
    m_timeWindow = new TimeWindow(this);

  return m_panelConfig = new InspToolWidgetConfig();
}

void
InspToolWidget::refreshInspectorCombo()
{
  std::string factory = m_panelConfig->inspFactory;
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  auto currClass = QString::fromStdString(getInspectorClass());

  int index = -1;
  int i = 0;

  m_ui->inspectorCombo->clear();

  for (auto p = sus->getFirstInspectionWidgetFactory();
       p != sus->getLastInspectionWidgetFactory();
       ++p) {
    if ((*p)->worksWith(currClass)) {
      m_ui->inspectorCombo->addItem(
            QString((*p)->description()),
            QVariant::fromValue<QString>((*p)->name()));
      if ((*p)->name() == factory)
        index = i;
      ++i;
    }
  }

  m_ui->inspectorCombo->setEnabled(i > 0);

  if (index != -1)
    m_ui->inspectorCombo->setCurrentIndex(index);
  else if (i > 0)
    m_ui->inspectorCombo->setCurrentIndex(0);

  refreshUi();
}

void
InspToolWidget::applyConfig()
{
  loadDemodulators();
  refreshInspectorCombo();
  setInspectorClass(m_panelConfig->inspectorClass);
  setPrecise(m_panelConfig->precise);
  m_timeWindow->postLoadInit();
  m_timeWindow->setPalette(m_panelConfig->palette);
  m_timeWindow->setPaletteOffset(m_panelConfig->paletteOffset);
  m_timeWindow->setPaletteContrast(m_panelConfig->paletteContrast);
  m_ui->frequencySpinBox->setEditable(false);
  m_ui->frequencySpinBox->setMinimum(-18e9);
  m_ui->triggerSpin->setValue(
        static_cast<qreal>(m_panelConfig->autoSquelchTriggerSNR));

  setProperty("collapsed", m_panelConfig->collapsed);

  // Track changes now
  connect(
        m_timeWindow,
        SIGNAL(configChanged()),
        this,
        SLOT(onTimeWindowConfigChanged()));
}

bool
InspToolWidget::event(QEvent *event)
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

void
InspToolWidget::connectAll()
{
  connect(
        m_ui->demodCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onInspClassChanged()));

  connect(
        m_ui->bandwidthSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onBandwidthChanged(double)));

  connect(
        m_ui->openInspectorButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onOpenInspector()));

  connect(
        m_ui->preciseCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onPreciseChanged()));

  connect(
        m_ui->captureButton,
        SIGNAL(pressed()),
        this,
        SLOT(onPressHold()));

  connect(
        m_ui->captureButton,
        SIGNAL(released()),
        this,
        SLOT(onReleaseHold()));

  connect(
        m_ui->autoSquelchButton,
        SIGNAL(pressed()),
        this,
        SLOT(onPressAutoSquelch()));

  connect(
        m_ui->autoSquelchButton,
        SIGNAL(released()),
        this,
        SLOT(onReleaseAutoSquelch()));

  connect(
        m_ui->autoSquelchButton,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleAutoSquelch()));

  connect(
        m_ui->triggerSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onTriggerSNRChanged(double)));

  connect(
        mediator()->getMainSpectrum(),
        SIGNAL(bandwidthChanged()),
        this,
        SLOT(onSpectrumBandwidthChanged()));

  connect(
        mediator()->getMainSpectrum(),
        SIGNAL(frequencyChanged(qint64)),
        this,
        SLOT(onSpectrumFrequencyChanged()));

  connect(
        mediator()->getMainSpectrum(),
        SIGNAL(loChanged(qint64)),
        this,
        SLOT(onSpectrumFrequencyChanged()));

  connect(
        m_tracker,
        SIGNAL(opened(Suscan::AnalyzerRequest const &)),
        this,
        SLOT(onOpened(Suscan::AnalyzerRequest const &)));

  connect(
        m_tracker,
        SIGNAL(cancelled(Suscan::AnalyzerRequest const &)),
        this,
        SLOT(onCancelled(Suscan::AnalyzerRequest const &)));

  connect(
        m_tracker,
        SIGNAL(error(Suscan::AnalyzerRequest const &, const std::string &)),
        this,
        SLOT(onError(Suscan::AnalyzerRequest const &, const std::string &)));
}

void
InspToolWidget::applySourceInfo(Suscan::AnalyzerSourceInfo const &info)
{
  m_sourceInfo = info;
  refreshUi();
}

void
InspToolWidget::refreshUi()
{
  bool haveInspFactories = m_ui->inspectorCombo->count() > 0;
  bool inspAllowed = m_sourceInfo.testPermission(
        SUSCAN_ANALYZER_PERM_OPEN_INSPECTOR)
      && haveInspFactories;
  bool rawAllowed = m_sourceInfo.testPermission(
        SUSCAN_ANALYZER_PERM_OPEN_RAW);

  m_ui->inspectorCombo->setEnabled(haveInspFactories);

  switch (m_state) {
    case DETACHED:
      m_ui->openInspectorButton->setEnabled(false);
      m_ui->bandwidthSpin->setEnabled(false);
      m_ui->captureButton->setEnabled(false);
      m_ui->autoSquelchButton->setEnabled(false);
      break;

    case ATTACHED:
      m_ui->openInspectorButton->setEnabled(inspAllowed);
      m_ui->bandwidthSpin->setEnabled(true);
      m_ui->captureButton->setEnabled(rawAllowed);
      m_ui->autoSquelchButton->setEnabled(rawAllowed);
      break;
  }
}

void
InspToolWidget::setDemodFrequency(qint64 freq)
{
  m_ui->frequencySpinBox->setValue(freq);
  m_demodFreq = freq;
}

void
InspToolWidget::setColorConfig(ColorConfig const &config)
{
  m_timeWindow->setColorConfig(config);
}

void
InspToolWidget::setBandwidthLimits(unsigned int min, unsigned int max)
{
  m_ui->bandwidthSpin->setMinimum(static_cast<int>(min));
  m_ui->bandwidthSpin->setMaximum(static_cast<int>(max));
}

void
InspToolWidget::setBandwidth(unsigned int freq)
{
  m_ui->bandwidthSpin->setValue(static_cast<int>(freq));
}

void
InspToolWidget::setPrecise(bool precise)
{
  m_ui->preciseCheck->setChecked(precise);
}

void
InspToolWidget::setState(enum State state)
{
  if (m_state != state) {
    m_sourceInfo = Suscan::AnalyzerSourceInfo();
    m_state = state;
    refreshUi();
  }
}

enum InspToolWidget::State
InspToolWidget::getState() const
{
  return m_state;
}

bool
InspToolWidget::getPrecise() const
{
  return m_ui->preciseCheck->isChecked();
}

void
InspToolWidget::setInspectorClass(std::string const &cls)
{
  auto qInspClass = QString::fromStdString(cls);

  auto ndx = m_ui->demodCombo->findData(qInspClass);

  if (ndx >= 0)
    m_ui->demodCombo->setCurrentIndex(ndx);
  else
    m_ui->demodCombo->setCurrentIndex(-1);
}

std::string
InspToolWidget::getInspectorClass() const
{
  if (m_ui->demodCombo->currentIndex() >= 0)
    return m_ui->demodCombo->currentData().value<QString>().toStdString();

  return "";
}

unsigned int
InspToolWidget::getBandwidth() const
{
  return static_cast<unsigned int>(m_ui->bandwidthSpin->value());
}

void
InspToolWidget::resetRawInspector(qreal fs)
{
  m_timeWindowFs = fs;
  m_uiRefreshSamples =
      std::ceil(
        SIGDIGGER_DEFAULT_UPDATEUI_PERIOD_MS * 1e-3 * m_timeWindowFs);
  m_maxSamples = m_ui->maxMemSpin->value() * (1 << 20) / sizeof(SUCOMPLEX);
  m_ui->hangTimeSpin->setMinimum(std::ceil(1e3 / fs));

  m_ui->sampleRateLabel->setText(
        SuWidgetsHelpers::formatQuantity(fs, "sp/s"));
  m_ui->durationLabel->setText(
        SuWidgetsHelpers::formatQuantity(0, "s"));
  m_ui->memoryLabel->setText(
        SuWidgetsHelpers::formatBinaryQuantity(0));
}

void
InspToolWidget::refreshCaptureInfo()
{
  m_ui->durationLabel->setText(
        SuWidgetsHelpers::formatQuantityFromDelta(
          m_data.size() / m_timeWindowFs,
          1 / m_timeWindowFs,
          "s"));
  m_ui->memoryLabel->setText(
        SuWidgetsHelpers::formatBinaryQuantity(
          static_cast<qint64>(m_data.size() * sizeof(SUCOMPLEX))));
}

void
InspToolWidget::transferHistory()
{
  // Insert older samples
  m_data.insert(
        m_data.end(),
        m_history.begin() + m_historyPtr,
        m_history.end());

  // Insert newer samples
  m_data.insert(
        m_data.end(),
        m_history.begin(),
        m_history.begin() + m_historyPtr);
}

void
InspToolWidget::feedRawInspector(const SUCOMPLEX *data, size_t size)
{
  m_totalSamples += size;
  bool refreshUi =
      m_totalSamples >= m_uiRefreshSamples;

  if (refreshUi)
    m_totalSamples %= m_uiRefreshSamples;

  if (m_ui->captureButton->isDown()) {
    // Manual capture
    m_data.insert(m_data.end(), data, data + size);
    if (refreshUi)
      refreshCaptureInfo();
  } else if (m_autoSquelch) {
    SUFLOAT level;
    SUFLOAT immLevel = 0;

    SUFLOAT sum = 0;
    SUFLOAT y = 0;
    SUFLOAT t;
    SUFLOAT err = m_ui->autoSquelchButton->isDown() ? m_powerError : 0;

    // Compute Kahan summation of samples. This is an energy measure.
    for (size_t i = 0; i < size; ++i) {
      y = SU_C_REAL(data[i] * SU_C_CONJ(data[i])) - err;
      t = sum + y;
      err = (t - sum) - y;
      sum = t;
    }

    // Power measure.
    if (m_ui->autoSquelchButton->isDown()) { // CASE 1: MANUAL
      m_powerAccum += sum;
      m_powerError = err;
      m_powerSamples += size;

      m_currEnergy = m_timeWindowFs * m_powerAccum;
      level = SU_POWER_DB(m_currEnergy / m_powerSamples);
    } else { // CASE 2: Measure a small fraction
      SUFLOAT immEnergy = m_timeWindowFs * sum;

      for (size_t i = 0; i < size; ++i) {
        m_history[m_historyPtr++] = data[i];
        if (m_historyPtr == m_history.size())
          m_historyPtr = 0;
      }

      // Limited energy accumulation
      if (size > m_hangLength) {
        // Rare case. Will never happen.
        m_currEnergy = (immEnergy * m_hangLength) / size;
      } else {
        // We add the measured energy, but remove an alpha percent of
        // the current energy.
        SUFLOAT alpha = static_cast<SUFLOAT>(size) / m_hangLength;
        m_currEnergy += immEnergy - alpha * m_currEnergy;
      }

      // Level is computed based on the hangLength
      level = SU_POWER_DB(m_currEnergy / m_hangLength);

      // Immediate level is computed based on the current chunk size
      immLevel = SU_POWER_DB(immEnergy / size);
    }

    // NOT TRIGGERED: Sensing the channel
    if (!m_autoSquelchTriggered) {
      if (refreshUi)
        m_ui->powerLabel->setText(
            QString::number(.1 * SU_FLOOR(10 * level)) + " dB");
      if (m_ui->autoSquelchButton->isDown()) {
        // SQUELCH BUTTON DOWN: Measure noise
        m_squelch = level
            + static_cast<SUFLOAT>(m_ui->triggerSpin->value());
        m_hangLevel = level
            + .5f * static_cast<SUFLOAT>(m_ui->triggerSpin->value());
        if (refreshUi)
          m_ui->squelchLevelLabel->setText(
              QString::number(.1 * SU_FLOOR(10 * m_squelch)) + " dB");
      } else {
        // SQUELCH BUTTON UP: Wait for signal
        if (level >= m_squelch) {
          transferHistory();
          m_autoSquelchTriggered = true;

          // Adjust current energy to measure
          m_currEnergy =
              (m_currEnergy * m_hangLength) / m_powerSamples;
          m_ui->autoSquelchButton->setText("Triggered!");
        }
      }
    }

    // TRIGGERED: Recording the channel
    if (m_autoSquelchTriggered) {
      m_data.insert(m_data.end(), data, data + size);
      refreshCaptureInfo();
      if (m_data.size() > m_hangLength) {
        if (immLevel >= m_hangLevel)
          m_hangCounter = 0;
        else
          m_hangCounter += size;

        if (m_hangCounter >= m_hangLength || m_data.size() > m_maxSamples) { // Hang!
          cancelAutoSquelch();
          openTimeWindow();
        }
      }
    }
  }
}

void
InspToolWidget::openTimeWindow()
{
  m_timeWindow->setData(
        m_data,
        m_timeWindowFs,
        m_ui->bandwidthSpin->value());
  m_timeWindow->refresh();
  m_timeWindow->setCenterFreq(m_demodFreq);
  m_timeWindow->show();
  m_timeWindow->raise();
  m_timeWindow->activateWindow();
  m_timeWindow->setWindowState(Qt::WindowState::WindowActive);
  m_timeWindow->onFit();

  m_ui->sampleRateLabel->setText(
        SuWidgetsHelpers::formatQuantity(0, "sp/s"));
  m_ui->durationLabel->setText(
        SuWidgetsHelpers::formatQuantity(0, "s"));
  m_ui->memoryLabel->setText(
        SuWidgetsHelpers::formatBinaryQuantity(0));
}

void
InspToolWidget::enableAutoSquelch()
{
  // Enable autoSquelch
  m_autoSquelch = true;
  m_powerAccum = m_powerError = m_powerSamples = 0;
  m_historyPtr = 0;
  m_ui->squelchLevelLabel->setEnabled(true);
  m_ui->powerLabel->setEnabled(true);
  m_ui->captureButton->setEnabled(false);
  m_ui->autoSquelchButton->setChecked(m_autoSquelch);
  m_ui->hangTimeSpin->setEnabled(false);
  m_ui->maxMemSpin->setEnabled(false);
  m_ui->triggerSpin->setEnabled(false);

  startRawCapture();
}

void
InspToolWidget::cancelAutoSquelch()
{
  // Cancel autoSquelch
  m_autoSquelch = false;
  m_autoSquelchTriggered = false;
  m_ui->squelchLevelLabel->setText("N/A");
  m_ui->powerLabel->setText("N/A");
  m_ui->squelchLevelLabel->setEnabled(false);
  m_ui->powerLabel->setEnabled(false);
  m_ui->captureButton->setEnabled(true);
  m_ui->autoSquelchButton->setChecked(m_autoSquelch);
  m_ui->hangTimeSpin->setEnabled(true);
  m_ui->maxMemSpin->setEnabled(true);
  m_ui->triggerSpin->setEnabled(true);
  m_ui->autoSquelchButton->setText("Autosquelch");

  stopRawCapture();
}

void
InspToolWidget::startRawCapture()
{
  m_data.resize(0);
  m_timeWindow->setData(
        m_data,
        m_timeWindowFs,
        m_ui->bandwidthSpin->value());

  if (m_analyzer != nullptr && !m_opened) {
    Suscan::Channel ch;
    ch.bw    = getBandwidth();
    ch.ft    = 0;
    ch.fc    = SCAST(SUFREQ, mediator()->getMainSpectrum()->getLoFreq());
    ch.fLow  = - .5 * ch.bw;
    ch.fHigh = + .5 * ch.bw;

    if (m_tracker->requestOpen("raw", ch, QVariant(), true))
      m_mediator->setUIBusy(true);
  }
}

void
InspToolWidget::stopRawCapture()
{
  m_mediator->setUIBusy(false);

  if (m_analyzer != nullptr) {
    if (m_opened)
      m_analyzer->closeInspector(m_request.handle);
    else
      m_tracker->cancelAll();
  }
}

void
InspToolWidget::setState(int, Suscan::Analyzer *analyzer)
{
  if (!m_stateSet) {
    setBandwidth(mediator()->getMainSpectrum()->getBandwidth());
    m_stateSet = true;
  }

  if (m_analyzer != analyzer) {
    m_mediator->setUIBusy(false);
    m_analyzer = analyzer;

    m_tracker->setAnalyzer(analyzer);
    m_opened = false;

    if (m_analyzer != nullptr) {
      connect(
            m_analyzer,
            SIGNAL(source_info_message(Suscan::SourceInfoMessage const &)),
            this,
            SLOT(onSourceInfoMessage(Suscan::SourceInfoMessage const &)));

      connect(
            m_analyzer,
            SIGNAL(inspector_message(Suscan::InspectorMessage const &)),
            this,
            SLOT(onInspectorMessage(Suscan::InspectorMessage const &)));

      connect(
            m_analyzer,
            SIGNAL(samples_message(Suscan::SamplesMessage const &)),
            this,
            SLOT(onInspectorSamples(Suscan::SamplesMessage const &)));
    }

    setState(m_analyzer == nullptr ? DETACHED : ATTACHED);
  }

  for (auto &p : UIComponent::actions())
    p->setEnabled(m_analyzer != nullptr);

  refreshInspectorCombo();
}

void
InspToolWidget::setProfile(Suscan::Source::Config &config)
{
  setBandwidthLimits(
        1,
        config.getDecimatedSampleRate());
}

void
InspToolWidget::openInspector(
    const char *inspFactory,
    const char *inspClass,
    bool precise)
{
  Suscan::Channel ch;
  ch.bw    = getBandwidth();
  ch.ft    = 0;
  ch.fc    = SCAST(SUFREQ, mediator()->getMainSpectrum()->getLoFreq());
  ch.fLow  = - .5 * ch.bw;
  ch.fHigh = + .5 * ch.bw;

  mediator()->openInspectorTab(inspFactory, inspClass, ch, precise);
}

void
InspToolWidget::loadDemodulators()
{
  auto demods = Suscan::Singleton::get_instance()->getInspectorDemodulators();

  BLOCKSIG(m_ui->demodCombo, clear());

  for (auto &demod : demods)
    BLOCKSIG(m_ui->demodCombo, addItem(
          QString::fromStdString(demod.second),
          QString::fromStdString(demod.first)));
}

InspToolWidget::InspToolWidget
(InspToolWidgetFactory *factory, UIMediator *mediator, QWidget *parent) :
  ToolWidget(factory, mediator, parent),
  m_ui(new Ui::InspectorPanel)
{
  m_ui->setupUi(this);

  m_tracker = new Suscan::AnalyzerRequestTracker(this);

  assertConfig();
  setState(DETACHED);
  refreshUi();
  loadDemodulators();
  connectAll();

  setProperty("collapsed", m_panelConfig->collapsed);

  auto action = new QAction;

  action->setIcon(QIcon(":/icons/ask-inspector.svg"));
  registerAction(action, SLOT(onOpenASK()));

  action = new QAction;
  action->setIcon(QIcon(":/icons/fsk-inspector.svg"));
  registerAction(action, SLOT(onOpenFSK()));

  action = new QAction;
  action->setIcon(QIcon(":/icons/psk-inspector.svg"));
  registerAction(action, SLOT(onOpenPSK()));

  action = new QAction;
  action->setIcon(QIcon(":/icons/rms-inspector.svg"));
  registerAction(action, SLOT(onOpenRMS()));
}

InspToolWidget::~InspToolWidget()
{
  delete m_ui;
}

/////////////////////////////////// Slots /////////////////////////////////////
void
InspToolWidget::onInspClassChanged()
{
  refreshInspectorCombo();
}

void
InspToolWidget::onOpenInspector()
{
  m_panelConfig->inspFactory =
    m_ui->inspectorCombo->currentData().value<QString>().toStdString();
  m_panelConfig->inspectorClass = getInspectorClass();

  openInspector(
        m_panelConfig->inspFactory.c_str(),
        m_panelConfig->inspectorClass.c_str(),
        m_ui->preciseCheck->isChecked());
}

void
InspToolWidget::onBandwidthChanged(double bw)
{
  // TODO: getBandwidth, setFilterBandwidth??
  mediator()->getMainSpectrum()->setFilterBandwidth(SCAST(unsigned, bw));
}

void
InspToolWidget::onPreciseChanged()
{
  m_panelConfig->precise = m_ui->preciseCheck->isChecked();
}

void
InspToolWidget::onPressAutoSquelch()
{
  if (!m_autoSquelch) {
    enableAutoSquelch();
    m_ui->autoSquelchButton->setText("Measuring...");
  } else {
    cancelAutoSquelch();
    if (m_data.size() > 0)
      openTimeWindow();
  }
}

void
InspToolWidget::onReleaseAutoSquelch()
{
  if (m_autoSquelch) {
    if (m_powerSamples == 0) {
      cancelAutoSquelch();
    } else {
      m_hangLength =
          1e-3 * m_ui->hangTimeSpin->value() * m_timeWindowFs;
      m_history.resize(2 * m_hangLength);
      std::fill(m_history.begin(), m_history.end(), 0);
      m_powerAccum /= m_powerSamples;
      m_powerSamples = 1;
      m_ui->autoSquelchButton->setText("Waiting...");
    }
  }
}

void
InspToolWidget::onToggleAutoSquelch()
{
  m_ui->autoSquelchButton->setChecked(m_autoSquelch);
}

void
InspToolWidget::onPressHold()
{
  startRawCapture();
}

void
InspToolWidget::onReleaseHold()
{
  stopRawCapture();

  if (m_data.size() > 0)
    openTimeWindow();
}

void
InspToolWidget::onTimeWindowConfigChanged()
{
  m_panelConfig->palette = m_timeWindow->getPalette();
  m_panelConfig->paletteOffset = m_timeWindow->getPaletteOffset();
  m_panelConfig->paletteContrast = m_timeWindow->getPaletteContrast();
}

void
InspToolWidget::onTriggerSNRChanged(double val)
{
  m_panelConfig->autoSquelchTriggerSNR = static_cast<SUFLOAT>(val);
}


// Main UI slots
void
InspToolWidget::onSpectrumBandwidthChanged()
{
  setBandwidth(mediator()->getMainSpectrum()->getBandwidth());
}

void
InspToolWidget::onSpectrumFrequencyChanged()
{
  setDemodFrequency(
        mediator()->getMainSpectrum()->getLoFreq()
        + mediator()->getMainSpectrum()->getCenterFreq());
}

// Request tracker slots
void
InspToolWidget::onOpened(Suscan::AnalyzerRequest const &request)
{
  m_opened = true;
  m_request = request;

  m_mediator->setUIBusy(false);

  resetRawInspector(SCAST(qreal, request.equivRate));
}

void
InspToolWidget::onCancelled(Suscan::AnalyzerRequest const &)
{
  m_mediator->setUIBusy(false);
}

void
InspToolWidget::onError(Suscan::AnalyzerRequest const &, std::string const &error)
{
  m_mediator->setUIBusy(false);

  QMessageBox::critical(
        this,
        "Failed to open raw inspector",
        "Failed to open raw inspector in the selected channel: "
        + QString::fromStdString(error));
}

// Analyzer slots
void
InspToolWidget::onSourceInfoMessage(Suscan::SourceInfoMessage const &msg)
{
  setBandwidthLimits(
        1,
        SCAST(unsigned, msg.info()->getSampleRate()));

  setDemodFrequency(
        mediator()->getMainSpectrum()->getLoFreq()
        + mediator()->getMainSpectrum()->getCenterFreq());
  setBandwidth(mediator()->getMainSpectrum()->getBandwidth());

  applySourceInfo(*msg.info());
}

void
InspToolWidget::onInspectorMessage(Suscan::InspectorMessage const &msg)
{
  if (
      m_opened
      && msg.getKind() == SUSCAN_ANALYZER_INSPECTOR_MSGKIND_CLOSE
      && msg.getInspectorId() == m_request.inspectorId)
    m_opened = false;
}

void
InspToolWidget::onInspectorSamples(Suscan::SamplesMessage const &msg)
{
  if (m_opened && msg.getInspectorId() == m_request.inspectorId)
    feedRawInspector(msg.getSamples(), msg.getCount());
}

void
InspToolWidget::onOpenASK()
{
  openInspector("GenericInspectorFactory", "ask", true);
}

void
InspToolWidget::onOpenFSK()
{
  openInspector("GenericInspectorFactory", "fsk", true);
}

void
InspToolWidget::onOpenPSK()
{
  openInspector("GenericInspectorFactory", "psk", true);
}

void
InspToolWidget::onOpenRMS()
{
  openInspector("RMSInspectorFactory", "power", true);
}
