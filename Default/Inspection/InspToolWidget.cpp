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

using namespace SigDigger;

///////////////////////////// Inspector panel cnfig ////////////////////////////
#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

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

  return this->persist(obj);
}

////////////////////////// Inspector panel widget //////////////////////////////
Suscan::Serializable *
InspToolWidget::allocConfig()
{
  if (this->m_timeWindow == nullptr)
    this->m_timeWindow = new TimeWindow(this);

  return this->m_panelConfig = new InspToolWidgetConfig();
}

void
InspToolWidget::refreshInspectorCombo()
{
  std::string factory = this->m_panelConfig->inspFactory;
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  auto currClass = QString::fromStdString(getInspectorClass());

  int index = -1;
  int i = 0;

  this->m_ui->inspectorCombo->clear();

  for (auto p = sus->getFirstInspectionWidgetFactory();
       p != sus->getLastInspectionWidgetFactory();
       ++p) {
    if ((*p)->worksWith(currClass)) {
      this->m_ui->inspectorCombo->addItem(
            QString((*p)->description()),
            QVariant::fromValue<QString>((*p)->name()));
      if ((*p)->name() == factory)
        index = i;
      ++i;
    }
  }

  this->m_ui->inspectorCombo->setEnabled(i > 0);

  if (index != -1)
    this->m_ui->inspectorCombo->setCurrentIndex(index);
  else if (i > 0)
    this->m_ui->inspectorCombo->setCurrentIndex(0);

  this->refreshUi();
}

void
InspToolWidget::applyConfig()
{
  this->refreshInspectorCombo();
  this->setInspectorClass(this->m_panelConfig->inspectorClass);
  this->setPrecise(this->m_panelConfig->precise);
  this->m_timeWindow->postLoadInit();
  this->m_timeWindow->setPalette(this->m_panelConfig->palette);
  this->m_timeWindow->setPaletteOffset(this->m_panelConfig->paletteOffset);
  this->m_timeWindow->setPaletteContrast(this->m_panelConfig->paletteContrast);
  this->m_ui->frequencySpinBox->setEditable(false);
  this->m_ui->frequencySpinBox->setMinimum(-18e9);
  this->m_ui->triggerSpin->setValue(
        static_cast<qreal>(this->m_panelConfig->autoSquelchTriggerSNR));

  this->setProperty("collapsed", this->m_panelConfig->collapsed);

  // Track changes now
  connect(
        this->m_timeWindow,
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
      this->m_panelConfig->collapsed = this->property("collapsed").value<bool>();
  }

  return ToolWidget::event(event);
}

void
InspToolWidget::connectAll()
{
  connect(
        this->m_ui->askRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onInspClassChanged()));
  connect(
        this->m_ui->powerRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onInspClassChanged()));
  connect(
        this->m_ui->pskRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onInspClassChanged()));
  connect(
        this->m_ui->fskRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onInspClassChanged()));
  connect(
        this->m_ui->audioRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onInspClassChanged()));
  connect(
        this->m_ui->rawRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onInspClassChanged()));
  connect(
        this->m_ui->bandwidthSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onBandwidthChanged(double)));

  connect(
        this->m_ui->openInspectorButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onOpenInspector()));

  connect(
        this->m_ui->preciseCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onPreciseChanged()));

  connect(
        this->m_ui->captureButton,
        SIGNAL(pressed()),
        this,
        SLOT(onPressHold()));

  connect(
        this->m_ui->captureButton,
        SIGNAL(released()),
        this,
        SLOT(onReleaseHold()));

  connect(
        this->m_ui->autoSquelchButton,
        SIGNAL(pressed()),
        this,
        SLOT(onPressAutoSquelch()));

  connect(
        this->m_ui->autoSquelchButton,
        SIGNAL(released()),
        this,
        SLOT(onReleaseAutoSquelch()));

  connect(
        this->m_ui->autoSquelchButton,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleAutoSquelch()));

  connect(
        this->m_ui->triggerSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onTriggerSNRChanged(double)));

  connect(
        this->mediator()->getMainSpectrum(),
        SIGNAL(bandwidthChanged()),
        this,
        SLOT(onSpectrumBandwidthChanged()));

  connect(
        this->mediator()->getMainSpectrum(),
        SIGNAL(frequencyChanged(qint64)),
        this,
        SLOT(onSpectrumFrequencyChanged()));

  connect(
        this->mediator()->getMainSpectrum(),
        SIGNAL(loChanged(qint64)),
        this,
        SLOT(onSpectrumFrequencyChanged()));

  connect(
        this->m_tracker,
        SIGNAL(opened(Suscan::AnalyzerRequest const &)),
        this,
        SLOT(onOpened(Suscan::AnalyzerRequest const &)));

  connect(
        this->m_tracker,
        SIGNAL(cancelled(Suscan::AnalyzerRequest const &)),
        this,
        SLOT(onCancelled(Suscan::AnalyzerRequest const &)));

  connect(
        this->m_tracker,
        SIGNAL(error(Suscan::AnalyzerRequest const &, const std::string &)),
        this,
        SLOT(onError(Suscan::AnalyzerRequest const &, const std::string &)));
}

void
InspToolWidget::applySourceInfo(Suscan::AnalyzerSourceInfo const &info)
{
  this->m_sourceInfo = info;
  this->refreshUi();
}

void
InspToolWidget::refreshUi()
{
  bool haveInspFactories = this->m_ui->inspectorCombo->count() > 0;
  bool inspAllowed = this->m_sourceInfo.testPermission(
        SUSCAN_ANALYZER_PERM_OPEN_INSPECTOR)
      && haveInspFactories;
  bool rawAllowed = this->m_sourceInfo.testPermission(
        SUSCAN_ANALYZER_PERM_OPEN_RAW);

  this->m_ui->inspectorCombo->setEnabled(haveInspFactories);

  switch (this->m_state) {
    case DETACHED:
      this->m_ui->openInspectorButton->setEnabled(false);
      this->m_ui->bandwidthSpin->setEnabled(false);
      this->m_ui->captureButton->setEnabled(false);
      this->m_ui->autoSquelchButton->setEnabled(false);
      break;

    case ATTACHED:
      this->m_ui->openInspectorButton->setEnabled(inspAllowed);
      this->m_ui->bandwidthSpin->setEnabled(true);
      this->m_ui->captureButton->setEnabled(rawAllowed);
      this->m_ui->autoSquelchButton->setEnabled(rawAllowed);
      break;
  }
}

void
InspToolWidget::setDemodFrequency(qint64 freq)
{
  this->m_ui->frequencySpinBox->setValue(freq);
  this->m_demodFreq = freq;
}

void
InspToolWidget::setColorConfig(ColorConfig const &config)
{
  this->m_timeWindow->setColorConfig(config);
}

void
InspToolWidget::setBandwidthLimits(unsigned int min, unsigned int max)
{
  this->m_ui->bandwidthSpin->setMinimum(static_cast<int>(min));
  this->m_ui->bandwidthSpin->setMaximum(static_cast<int>(max));
}

void
InspToolWidget::setBandwidth(unsigned int freq)
{
  this->m_ui->bandwidthSpin->setValue(static_cast<int>(freq));
}

void
InspToolWidget::setPrecise(bool precise)
{
  this->m_ui->preciseCheck->setChecked(precise);
}

void
InspToolWidget::setState(enum State state)
{
  if (this->m_state != state) {
    this->m_sourceInfo = Suscan::AnalyzerSourceInfo();
    this->m_state = state;
    this->refreshUi();
  }
}

enum InspToolWidget::State
InspToolWidget::getState() const
{
  return this->m_state;
}

bool
InspToolWidget::getPrecise() const
{
  return this->m_ui->preciseCheck->isChecked();
}

void
InspToolWidget::setInspectorClass(std::string const &cls)
{
  if (cls == "psk")
    this->m_ui->pskRadio->setChecked(true);
  else if (cls == "fsk")
    this->m_ui->fskRadio->setChecked(true);
  else if (cls == "ask")
    this->m_ui->askRadio->setChecked(true);
  else if (cls == "audio")
    this->m_ui->audioRadio->setChecked(true);
  else if (cls == "raw")
    this->m_ui->rawRadio->setChecked(true);
  else if (cls == "power")
    this->m_ui->powerRadio->setChecked(true);
}

std::string
InspToolWidget::getInspectorClass() const
{
  if (this->m_ui->pskRadio->isChecked())
    return "psk";
  else if (this->m_ui->fskRadio->isChecked())
    return "fsk";
  else if (this->m_ui->askRadio->isChecked())
    return "ask";
  else if (this->m_ui->audioRadio->isChecked())
    return "audio";
  else if (this->m_ui->rawRadio->isChecked())
    return "raw";
  else if (this->m_ui->powerRadio->isChecked())
    return "power";
  return "";
}

unsigned int
InspToolWidget::getBandwidth() const
{
  return static_cast<unsigned int>(this->m_ui->bandwidthSpin->value());
}

void
InspToolWidget::resetRawInspector(qreal fs)
{
  this->m_timeWindowFs = fs;
  this->m_uiRefreshSamples =
      std::ceil(
        SIGDIGGER_DEFAULT_UPDATEUI_PERIOD_MS * 1e-3 * this->m_timeWindowFs);
  this->m_maxSamples = this->m_ui->maxMemSpin->value() * (1 << 20) / sizeof(SUCOMPLEX);
  this->m_ui->hangTimeSpin->setMinimum(std::ceil(1e3 / fs));

  this->m_ui->sampleRateLabel->setText(
        SuWidgetsHelpers::formatQuantity(fs, "sp/s"));
  this->m_ui->durationLabel->setText(
        SuWidgetsHelpers::formatQuantity(0, "s"));
  this->m_ui->memoryLabel->setText(
        SuWidgetsHelpers::formatBinaryQuantity(0));
}

void
InspToolWidget::refreshCaptureInfo()
{
  this->m_ui->durationLabel->setText(
        SuWidgetsHelpers::formatQuantityFromDelta(
          this->m_data.size() / this->m_timeWindowFs,
          1 / this->m_timeWindowFs,
          "s"));
  this->m_ui->memoryLabel->setText(
        SuWidgetsHelpers::formatBinaryQuantity(
          static_cast<qint64>(this->m_data.size() * sizeof(SUCOMPLEX))));
}

void
InspToolWidget::transferHistory()
{
  // Insert older samples
  this->m_data.insert(
        this->m_data.end(),
        this->m_history.begin() + m_historyPtr,
        this->m_history.end());

  // Insert newer samples
  this->m_data.insert(
        this->m_data.end(),
        this->m_history.begin(),
        this->m_history.begin() + m_historyPtr);
}

void
InspToolWidget::feedRawInspector(const SUCOMPLEX *data, size_t size)
{
  this->m_totalSamples += size;
  bool refreshUi =
      this->m_totalSamples >= m_uiRefreshSamples;

  if (refreshUi)
    this->m_totalSamples %= m_uiRefreshSamples;

  if (this->m_ui->captureButton->isDown()) {
    // Manual capture
    this->m_data.insert(this->m_data.end(), data, data + size);
    if (refreshUi)
      this->refreshCaptureInfo();
  } else if (this->m_autoSquelch) {
    SUFLOAT level;
    SUFLOAT immLevel = 0;

    SUFLOAT sum = 0;
    SUFLOAT y = 0;
    SUFLOAT t;
    SUFLOAT err = this->m_ui->autoSquelchButton->isDown() ? this->m_powerError : 0;

    // Compute Kahan summation of samples. This is an energy measure.
    for (size_t i = 0; i < size; ++i) {
      y = SU_C_REAL(data[i] * SU_C_CONJ(data[i])) - err;
      t = sum + y;
      err = (t - sum) - y;
      sum = t;
    }

    // Power measure.
    if (this->m_ui->autoSquelchButton->isDown()) { // CASE 1: MANUAL
      this->m_powerAccum += sum;
      this->m_powerError = err;
      this->m_powerSamples += size;

      this->m_currEnergy = this->m_timeWindowFs * m_powerAccum;
      level = SU_POWER_DB(this->m_currEnergy / this->m_powerSamples);
    } else { // CASE 2: Measure a small fraction
      SUFLOAT immEnergy = this->m_timeWindowFs * sum;

      for (size_t i = 0; i < size; ++i) {
        this->m_history[m_historyPtr++] = data[i];
        if (this->m_historyPtr == this->m_history.size())
          this->m_historyPtr = 0;
      }

      // Limited energy accumulation
      if (size > this->m_hangLength) {
        // Rare case. Will never happen.
        this->m_currEnergy = (immEnergy * this->m_hangLength) / size;
      } else {
        // We add the measured energy, but remove an alpha percent of
        // the current energy.
        SUFLOAT alpha = static_cast<SUFLOAT>(size) / this->m_hangLength;
        this->m_currEnergy += immEnergy - alpha * this->m_currEnergy;
      }

      // Level is computed based on the hangLength
      level = SU_POWER_DB(this->m_currEnergy / this->m_hangLength);

      // Immediate level is computed based on the current chunk size
      immLevel = SU_POWER_DB(immEnergy / size);
    }

    // NOT TRIGGERED: Sensing the channel
    if (!this->m_autoSquelchTriggered) {
      if (refreshUi)
        this->m_ui->powerLabel->setText(
            QString::number(.1 * SU_FLOOR(10 * level)) + " dB");
      if (this->m_ui->autoSquelchButton->isDown()) {
        // SQUELCH BUTTON DOWN: Measure noise
        this->m_squelch = level
            + static_cast<SUFLOAT>(this->m_ui->triggerSpin->value());
        this->m_hangLevel = level
            + .5f * static_cast<SUFLOAT>(this->m_ui->triggerSpin->value());
        if (refreshUi)
          this->m_ui->squelchLevelLabel->setText(
              QString::number(.1 * SU_FLOOR(10 * this->m_squelch)) + " dB");
      } else {
        // SQUELCH BUTTON UP: Wait for signal
        if (level >= this->m_squelch) {
          this->transferHistory();
          this->m_autoSquelchTriggered = true;

          // Adjust current energy to measure
          this->m_currEnergy =
              (this->m_currEnergy * this->m_hangLength) / m_powerSamples;
          this->m_ui->autoSquelchButton->setText("Triggered!");
        }
      }
    }

    // TRIGGERED: Recording the channel
    if (this->m_autoSquelchTriggered) {
      this->m_data.insert(this->m_data.end(), data, data + size);
      this->refreshCaptureInfo();
      if (this->m_data.size() > this->m_hangLength) {
        if (immLevel >= this->m_hangLevel)
          this->m_hangCounter = 0;
        else
          this->m_hangCounter += size;

        if (this->m_hangCounter >= m_hangLength || this->m_data.size() > m_maxSamples) { // Hang!
          this->cancelAutoSquelch();
          this->openTimeWindow();
        }
      }
    }
  }
}

void
InspToolWidget::openTimeWindow()
{
  this->m_timeWindow->setData(
        this->m_data,
        this->m_timeWindowFs,
        this->m_ui->bandwidthSpin->value());
  this->m_timeWindow->refresh();
  this->m_timeWindow->setCenterFreq(this->m_demodFreq);
  this->m_timeWindow->show();
  this->m_timeWindow->raise();
  this->m_timeWindow->activateWindow();
  this->m_timeWindow->setWindowState(Qt::WindowState::WindowActive);
  this->m_timeWindow->onFit();

  this->m_ui->sampleRateLabel->setText(
        SuWidgetsHelpers::formatQuantity(0, "sp/s"));
  this->m_ui->durationLabel->setText(
        SuWidgetsHelpers::formatQuantity(0, "s"));
  this->m_ui->memoryLabel->setText(
        SuWidgetsHelpers::formatBinaryQuantity(0));
}

void
InspToolWidget::enableAutoSquelch()
{
  // Enable autoSquelch
  this->m_autoSquelch = true;
  this->m_powerAccum = m_powerError = m_powerSamples = 0;
  this->m_historyPtr = 0;
  this->m_ui->squelchLevelLabel->setEnabled(true);
  this->m_ui->powerLabel->setEnabled(true);
  this->m_ui->captureButton->setEnabled(false);
  this->m_ui->autoSquelchButton->setChecked(this->m_autoSquelch);
  this->m_ui->hangTimeSpin->setEnabled(false);
  this->m_ui->maxMemSpin->setEnabled(false);
  this->m_ui->triggerSpin->setEnabled(false);

  this->startRawCapture();
}

void
InspToolWidget::cancelAutoSquelch()
{
  // Cancel autoSquelch
  this->m_autoSquelch = false;
  this->m_autoSquelchTriggered = false;
  this->m_ui->squelchLevelLabel->setText("N/A");
  this->m_ui->powerLabel->setText("N/A");
  this->m_ui->squelchLevelLabel->setEnabled(false);
  this->m_ui->powerLabel->setEnabled(false);
  this->m_ui->captureButton->setEnabled(true);
  this->m_ui->autoSquelchButton->setChecked(this->m_autoSquelch);
  this->m_ui->hangTimeSpin->setEnabled(true);
  this->m_ui->maxMemSpin->setEnabled(true);
  this->m_ui->triggerSpin->setEnabled(true);
  this->m_ui->autoSquelchButton->setText("Autosquelch");

  this->stopRawCapture();
}

void
InspToolWidget::startRawCapture()
{
  this->m_data.resize(0);
  this->m_timeWindow->setData(
        this->m_data,
        this->m_timeWindowFs,
        this->m_ui->bandwidthSpin->value());

  if (m_analyzer != nullptr && !m_opened) {
    Suscan::Channel ch;
    ch.bw    = this->getBandwidth();
    ch.ft    = 0;
    ch.fc    = SCAST(SUFREQ, this->mediator()->getMainSpectrum()->getLoFreq());
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
  if (!this->m_stateSet) {
    this->setBandwidth(this->mediator()->getMainSpectrum()->getBandwidth());
    this->m_stateSet = true;
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

    this->setState(m_analyzer == nullptr ? DETACHED : ATTACHED);
  }

  this->refreshInspectorCombo();
}

void
InspToolWidget::setProfile(Suscan::Source::Config &config)
{
  this->setBandwidthLimits(
        1,
        config.getDecimatedSampleRate());
}

InspToolWidget::InspToolWidget
(InspToolWidgetFactory *factory, UIMediator *mediator, QWidget *parent) :
  ToolWidget(factory, mediator, parent),
  m_ui(new Ui::InspectorPanel)
{
  m_ui->setupUi(this);

  m_tracker = new Suscan::AnalyzerRequestTracker(this);

  this->assertConfig();
  this->setState(DETACHED);
  this->refreshUi();
  this->connectAll();

  this->setProperty("collapsed", this->m_panelConfig->collapsed);
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
  Suscan::Channel ch;
  ch.bw    = this->getBandwidth();
  ch.ft    = 0;
  ch.fc    = SCAST(SUFREQ, this->mediator()->getMainSpectrum()->getLoFreq());
  ch.fLow  = - .5 * ch.bw;
  ch.fHigh = + .5 * ch.bw;

  if (this->m_ui->inspectorCombo->currentIndex() == -1)
    return;

  this->m_panelConfig->inspFactory =
    this->m_ui->inspectorCombo->currentData().value<QString>().toStdString();

  this->m_panelConfig->inspectorClass = this->getInspectorClass();

  this->mediator()->openInspectorTab(
        this->m_panelConfig->inspFactory.c_str(),
        this->m_panelConfig->inspectorClass.c_str(),
        ch,
        this->m_ui->preciseCheck->isChecked());
}

void
InspToolWidget::onBandwidthChanged(double bw)
{
  // TODO: getBandwidth, setFilterBandwidth??
  this->mediator()->getMainSpectrum()->setFilterBandwidth(SCAST(unsigned, bw));
}

void
InspToolWidget::onPreciseChanged()
{
  this->m_panelConfig->precise = m_ui->preciseCheck->isChecked();
}

void
InspToolWidget::onPressAutoSquelch()
{
  if (!this->m_autoSquelch) {
    this->enableAutoSquelch();
    this->m_ui->autoSquelchButton->setText("Measuring...");
  } else {
    this->cancelAutoSquelch();
    if (this->m_data.size() > 0)
      this->openTimeWindow();
  }
}

void
InspToolWidget::onReleaseAutoSquelch()
{
  if (this->m_autoSquelch) {
    if (this->m_powerSamples == 0) {
      cancelAutoSquelch();
    } else {
      this->m_hangLength =
          1e-3 * this->m_ui->hangTimeSpin->value() * this->m_timeWindowFs;
      this->m_history.resize(2 * this->m_hangLength);
      std::fill(this->m_history.begin(), this->m_history.end(), 0);
      this->m_powerAccum /= m_powerSamples;
      this->m_powerSamples = 1;
      this->m_ui->autoSquelchButton->setText("Waiting...");
    }
  }
}

void
InspToolWidget::onToggleAutoSquelch()
{
  this->m_ui->autoSquelchButton->setChecked(this->m_autoSquelch);
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

  if (this->m_data.size() > 0)
    this->openTimeWindow();
}

void
InspToolWidget::onTimeWindowConfigChanged()
{
  this->m_panelConfig->palette = this->m_timeWindow->getPalette();
  this->m_panelConfig->paletteOffset = this->m_timeWindow->getPaletteOffset();
  this->m_panelConfig->paletteContrast = this->m_timeWindow->getPaletteContrast();
}

void
InspToolWidget::onTriggerSNRChanged(double val)
{
  this->m_panelConfig->autoSquelchTriggerSNR = static_cast<SUFLOAT>(val);
}


// Main UI slots
void
InspToolWidget::onSpectrumBandwidthChanged()
{
  this->setBandwidth(this->mediator()->getMainSpectrum()->getBandwidth());
}

void
InspToolWidget::onSpectrumFrequencyChanged()
{
  this->setDemodFrequency(
        this->mediator()->getMainSpectrum()->getLoFreq()
        + this->mediator()->getMainSpectrum()->getCenterFreq());
}

// Request tracker slots
void
InspToolWidget::onOpened(Suscan::AnalyzerRequest const &request)
{
  m_opened = true;
  m_request = request;

  m_mediator->setUIBusy(false);

  this->resetRawInspector(SCAST(qreal, request.equivRate));
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
  this->setBandwidthLimits(
        1,
        SCAST(unsigned, msg.info()->getSampleRate()));

  setDemodFrequency(
        mediator()->getMainSpectrum()->getLoFreq()
        + mediator()->getMainSpectrum()->getCenterFreq());
  setBandwidth(this->mediator()->getMainSpectrum()->getBandwidth());

  this->applySourceInfo(*msg.info());
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
    this->feedRawInspector(msg.getSamples(), msg.getCount());
}
