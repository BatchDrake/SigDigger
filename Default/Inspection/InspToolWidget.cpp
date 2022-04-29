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
InspToolWidgetConfig::serialize(void)
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
InspToolWidget::allocConfig(void)
{
  if (this->timeWindow == nullptr)
    this->timeWindow = new TimeWindow(this);

  return this->panelConfig = new InspToolWidgetConfig();
}

void
InspToolWidget::refreshInspectorCombo()
{
  std::string factory = this->panelConfig->inspFactory;
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  int index = -1;
  int i = 0;

  this->ui->inspectorCombo->clear();

  for (auto p = sus->getFirstInspectionWidgetFactory();
       p != sus->getLastInspectionWidgetFactory();
       ++p, ++i) {
    this->ui->inspectorCombo->addItem(
          QString((*p)->description()),
          QVariant::fromValue<QString>((*p)->name()));

    if ((*p)->name() == factory)
      index = i;
  }

  this->ui->inspectorCombo->setEnabled(index != -1);

  if (index != -1)
    this->ui->inspectorCombo->setCurrentIndex(index);

  this->refreshUi();
}

void
InspToolWidget::applyConfig(void)
{
  this->refreshInspectorCombo();
  this->setInspectorClass(this->panelConfig->inspectorClass);
  this->setPrecise(this->panelConfig->precise);
  this->timeWindow->setPalette(this->panelConfig->palette);
  this->timeWindow->setPaletteOffset(this->panelConfig->paletteOffset);
  this->timeWindow->setPaletteContrast(this->panelConfig->paletteContrast);
  this->ui->frequencySpinBox->setEditable(false);
  this->ui->frequencySpinBox->setMinimum(-18e9);
  this->ui->triggerSpin->setValue(
        static_cast<qreal>(this->panelConfig->autoSquelchTriggerSNR));

  this->setProperty("collapsed", this->panelConfig->collapsed);

  // Track changes now
  connect(
        this->timeWindow,
        SIGNAL(configChanged(void)),
        this,
        SLOT(onTimeWindowConfigChanged(void)));
}

bool
InspToolWidget::event(QEvent *event)
{
  if (event->type() == QEvent::DynamicPropertyChange) {
    QDynamicPropertyChangeEvent *const propEvent =
        static_cast<QDynamicPropertyChangeEvent*>(event);
    QString propName = propEvent->propertyName();
    if (propName == "collapsed")
      this->panelConfig->collapsed = this->property("collapsed").value<bool>();
  }

  return ToolWidget::event(event);
}

void
InspToolWidget::connectAll(void)
{
  connect(
        this->ui->bandwidthSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onBandwidthChanged(double)));

  connect(
        this->ui->openInspectorButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onOpenInspector(void)));

  connect(
        this->ui->preciseCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onPreciseChanged(void)));

  connect(
        this->ui->captureButton,
        SIGNAL(pressed(void)),
        this,
        SLOT(onPressHold(void)));

  connect(
        this->ui->captureButton,
        SIGNAL(released(void)),
        this,
        SLOT(onReleaseHold(void)));

  connect(
        this->ui->autoSquelchButton,
        SIGNAL(pressed(void)),
        this,
        SLOT(onPressAutoSquelch(void)));

  connect(
        this->ui->autoSquelchButton,
        SIGNAL(released(void)),
        this,
        SLOT(onReleaseAutoSquelch(void)));

  connect(
        this->ui->autoSquelchButton,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleAutoSquelch(void)));

  connect(
        this->ui->triggerSpin,
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
  this->sourceInfo = info;
  this->refreshUi();
}

void
InspToolWidget::refreshUi(void)
{
  bool haveInspFactories = this->ui->inspectorCombo->count() > 0;
  bool inspAllowed = this->sourceInfo.testPermission(
        SUSCAN_ANALYZER_PERM_OPEN_INSPECTOR)
      && haveInspFactories;
  bool rawAllowed = this->sourceInfo.testPermission(
        SUSCAN_ANALYZER_PERM_OPEN_RAW);

  this->ui->inspectorCombo->setEnabled(haveInspFactories);

  switch (this->state) {
    case DETACHED:
      this->ui->openInspectorButton->setEnabled(false);
      this->ui->bandwidthSpin->setEnabled(false);
      this->ui->captureButton->setEnabled(false);
      this->ui->autoSquelchButton->setEnabled(false);
      break;

    case ATTACHED:
      this->ui->openInspectorButton->setEnabled(inspAllowed);
      this->ui->bandwidthSpin->setEnabled(true);
      this->ui->captureButton->setEnabled(rawAllowed);
      this->ui->autoSquelchButton->setEnabled(rawAllowed);
      break;
  }
}

void
InspToolWidget::setDemodFrequency(qint64 freq)
{
  this->ui->frequencySpinBox->setValue(freq);
  this->demodFreq = freq;
}

void
InspToolWidget::setColorConfig(ColorConfig const &config)
{
  this->timeWindow->setColorConfig(config);
}

void
InspToolWidget::setBandwidthLimits(unsigned int min, unsigned int max)
{
  this->ui->bandwidthSpin->setMinimum(static_cast<int>(min));
  this->ui->bandwidthSpin->setMaximum(static_cast<int>(max));
}

void
InspToolWidget::setBandwidth(unsigned int freq)
{
  this->ui->bandwidthSpin->setValue(static_cast<int>(freq));
}

void
InspToolWidget::setPrecise(bool precise)
{
  this->ui->preciseCheck->setChecked(precise);
}

void
InspToolWidget::setState(enum State state)
{
  if (this->state != state) {
    this->sourceInfo = Suscan::AnalyzerSourceInfo();
    this->state = state;
    this->refreshUi();
  }
}

enum InspToolWidget::State
InspToolWidget::getState(void) const
{
  return this->state;
}

bool
InspToolWidget::getPrecise(void) const
{
  return this->ui->preciseCheck->isChecked();
}

void
InspToolWidget::setInspectorClass(std::string const &cls)
{
  if (cls == "psk")
    this->ui->pskRadio->setChecked(true);
  else if (cls == "fsk")
    this->ui->fskRadio->setChecked(true);
  else if (cls == "ask")
    this->ui->askRadio->setChecked(true);
  else if (cls == "audio")
    this->ui->audioRadio->setChecked(true);
}

std::string
InspToolWidget::getInspectorClass(void) const
{
  if (this->ui->pskRadio->isChecked())
    return "psk";
  else if (this->ui->fskRadio->isChecked())
    return "fsk";
  else if (this->ui->askRadio->isChecked())
    return "ask";
  else if (this->ui->audioRadio->isChecked())
    return "audio";
  return "";
}

unsigned int
InspToolWidget::getBandwidth(void) const
{
  return static_cast<unsigned int>(this->ui->bandwidthSpin->value());
}

void
InspToolWidget::resetRawInspector(qreal fs)
{
  this->timeWindowFs = fs;
  this->uiRefreshSamples =
      std::ceil(
        SIGDIGGER_DEFAULT_UPDATEUI_PERIOD_MS * 1e-3 * this->timeWindowFs);
  this->maxSamples = this->ui->maxMemSpin->value() * (1 << 20) / sizeof(SUCOMPLEX);
  this->ui->hangTimeSpin->setMinimum(std::ceil(1e3 / fs));
  this->data.resize(0);
  this->timeWindow->setData(
        this->data,
        this->timeWindowFs,
        this->ui->bandwidthSpin->value());
  this->ui->sampleRateLabel->setText(
        SuWidgetsHelpers::formatQuantity(fs, "sp/s"));
  this->ui->durationLabel->setText(
        SuWidgetsHelpers::formatQuantity(0, "s"));
  this->ui->memoryLabel->setText(
        SuWidgetsHelpers::formatBinaryQuantity(0));
}

void
InspToolWidget::refreshCaptureInfo(void)
{
  this->ui->durationLabel->setText(
        SuWidgetsHelpers::formatQuantityFromDelta(
          this->data.size() / this->timeWindowFs,
          1 / this->timeWindowFs,
          "s"));
  this->ui->memoryLabel->setText(
        SuWidgetsHelpers::formatBinaryQuantity(
          static_cast<qint64>(this->data.size() * sizeof(SUCOMPLEX))));
}

void
InspToolWidget::transferHistory(void)
{
  // Insert older samples
  this->data.insert(
        this->data.end(),
        this->history.begin() + this->historyPtr,
        this->history.end());

  // Insert newer samples
  this->data.insert(
        this->data.end(),
        this->history.begin(),
        this->history.begin() + this->historyPtr);
}

void
InspToolWidget::feedRawInspector(const SUCOMPLEX *data, size_t size)
{
  this->totalSamples += size;
  bool refreshUi =
      this->totalSamples >= this->uiRefreshSamples;

  if (refreshUi)
    this->totalSamples %= this->uiRefreshSamples;

  if (this->ui->captureButton->isDown()) {
    // Manual capture
    this->data.insert(this->data.end(), data, data + size);
    if (refreshUi)
      this->refreshCaptureInfo();
  } else if (this->autoSquelch) {
    SUFLOAT level;
    SUFLOAT immLevel = 0;

    SUFLOAT sum = 0;
    SUFLOAT y = 0;
    SUFLOAT t;
    SUFLOAT err = this->ui->autoSquelchButton->isDown() ? this->powerError : 0;

    // Compute Kahan summation of samples. This is an energy measure.
    for (size_t i = 0; i < size; ++i) {
      y = SU_C_REAL(data[i] * SU_C_CONJ(data[i])) - err;
      t = sum + y;
      err = (t - sum) - y;
      sum = t;
    }

    // Power measure.
    if (this->ui->autoSquelchButton->isDown()) { // CASE 1: MANUAL
      this->powerAccum += sum;
      this->powerError = err;
      this->powerSamples += size;

      this->currEnergy = this->timeWindowFs * this->powerAccum;
      level = SU_POWER_DB(this->currEnergy / this->powerSamples);
    } else { // CASE 2: Measure a small fraction
      SUFLOAT immEnergy = this->timeWindowFs * sum;

      for (size_t i = 0; i < size; ++i) {
        this->history[this->historyPtr++] = data[i];
        if (this->historyPtr == this->history.size())
          this->historyPtr = 0;
      }

      // Limited energy accumulation
      if (size > this->hangLength) {
        // Rare case. Will never happen.
        this->currEnergy = (immEnergy * this->hangLength) / size;
      } else {
        // We add the measured energy, but remove an alpha percent of
        // the current energy.
        SUFLOAT alpha = static_cast<SUFLOAT>(size) / this->hangLength;
        this->currEnergy += immEnergy - alpha * this->currEnergy;
      }

      // Level is computed based on the hangLength
      level = SU_POWER_DB(this->currEnergy / this->hangLength);

      // Immediate level is computed based on the current chunk size
      immLevel = SU_POWER_DB(immEnergy / size);
    }

    // NOT TRIGGERED: Sensing the channel
    if (!this->autoSquelchTriggered) {
      if (refreshUi)
        this->ui->powerLabel->setText(
            QString::number(.1 * SU_FLOOR(10 * level)) + " dB");
      if (this->ui->autoSquelchButton->isDown()) {
        // SQUELCH BUTTON DOWN: Measure noise
        this->squelch = level
            + static_cast<SUFLOAT>(this->ui->triggerSpin->value());
        this->hangLevel = level
            + .5f * static_cast<SUFLOAT>(this->ui->triggerSpin->value());
        if (refreshUi)
          this->ui->squelchLevelLabel->setText(
              QString::number(.1 * SU_FLOOR(10 * this->squelch)) + " dB");
      } else {
        // SQUELCH BUTTON UP: Wait for signal
        if (level >= this->squelch) {
          this->transferHistory();
          this->autoSquelchTriggered = true;

          // Adjust current energy to measure
          this->currEnergy =
              (this->currEnergy * this->hangLength) / this->powerSamples;
          this->ui->autoSquelchButton->setText("Triggered!");
        }
      }
    }

    // TRIGGERED: Recording the channel
    if (this->autoSquelchTriggered) {
      this->data.insert(this->data.end(), data, data + size);
      this->refreshCaptureInfo();
      if (this->data.size() > this->hangLength) {
        if (immLevel >= this->hangLevel)
          this->hangCounter = 0;
        else
          this->hangCounter += size;

        if (this->hangCounter >= this->hangLength || this->data.size() > this->maxSamples) { // Hang!
          this->cancelAutoSquelch();
          this->openTimeWindow();
        }
      }
    }
  }
}

void
InspToolWidget::openTimeWindow(void)
{
  this->timeWindow->setData(
        this->data,
        this->timeWindowFs,
        this->ui->bandwidthSpin->value());
  this->timeWindow->setCenterFreq(this->demodFreq);
  this->timeWindow->show();
  this->timeWindow->raise();
  this->timeWindow->activateWindow();
  this->timeWindow->setWindowState(Qt::WindowState::WindowActive);
  this->timeWindow->onFit();

  this->ui->sampleRateLabel->setText(
        SuWidgetsHelpers::formatQuantity(0, "sp/s"));
  this->ui->durationLabel->setText(
        SuWidgetsHelpers::formatQuantity(0, "s"));
  this->ui->memoryLabel->setText(
        SuWidgetsHelpers::formatBinaryQuantity(0));
}

void
InspToolWidget::enableAutoSquelch(void)
{
  // Enable autoSquelch
  this->autoSquelch = true;
  this->powerAccum = this->powerError = this->powerSamples = 0;
  this->historyPtr = 0;
  this->ui->squelchLevelLabel->setEnabled(true);
  this->ui->powerLabel->setEnabled(true);
  this->ui->captureButton->setEnabled(false);
  this->ui->autoSquelchButton->setChecked(this->autoSquelch);
  this->ui->hangTimeSpin->setEnabled(false);
  this->ui->maxMemSpin->setEnabled(false);
  this->ui->triggerSpin->setEnabled(false);

  this->startRawCapture();
}

void
InspToolWidget::cancelAutoSquelch(void)
{
  // Cancel autoSquelch
  this->autoSquelch = false;
  this->autoSquelchTriggered = false;
  this->ui->squelchLevelLabel->setText("N/A");
  this->ui->powerLabel->setText("N/A");
  this->ui->squelchLevelLabel->setEnabled(false);
  this->ui->powerLabel->setEnabled(false);
  this->ui->captureButton->setEnabled(true);
  this->ui->autoSquelchButton->setChecked(this->autoSquelch);
  this->ui->hangTimeSpin->setEnabled(true);
  this->ui->maxMemSpin->setEnabled(true);
  this->ui->triggerSpin->setEnabled(true);
  this->ui->autoSquelchButton->setText("Autosquelch");

  this->stopRawCapture();
  this->resetRawInspector(1);
}

void
InspToolWidget::startRawCapture()
{
  if (m_analyzer != nullptr && !m_opened) {
    Suscan::Channel ch;
    ch.bw    = this->getBandwidth();
    ch.ft    = 0;
    ch.fc    = SCAST(SUFREQ, this->mediator()->getMainSpectrum()->getLoFreq());
    ch.fLow  = - .5 * ch.bw;
    ch.fHigh = + .5 * ch.bw;

    m_tracker->requestOpen("raw", ch, QVariant(), true);
  }
}

void
InspToolWidget::stopRawCapture()
{
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
  if (m_analyzer != analyzer) {
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
}

void
InspToolWidget::setProfile(Suscan::Source::Config &config)
{
  this->setBandwidthLimits(
        1,
        config.getDecimatedSampleRate());

  this->setBandwidth(this->mediator()->getMainSpectrum()->getBandwidth());
}

InspToolWidget::InspToolWidget
(InspToolWidgetFactory *factory, UIMediator *mediator, QWidget *parent) :
  ToolWidget(factory, mediator, parent),
  ui(new Ui::InspectorPanel)
{
  ui->setupUi(this);

  m_tracker = new Suscan::AnalyzerRequestTracker(this);

  this->assertConfig();
  this->setState(DETACHED);
  this->refreshUi();
  this->connectAll();

  this->setProperty("collapsed", this->panelConfig->collapsed);
}

InspToolWidget::~InspToolWidget()
{
  delete ui;
}

/////////////////////////////////// Slots /////////////////////////////////////
void
InspToolWidget::onOpenInspector(void)
{
  Suscan::Channel ch;
  ch.bw    = this->getBandwidth();
  ch.ft    = 0;
  ch.fc    = SCAST(SUFREQ, this->mediator()->getMainSpectrum()->getLoFreq());
  ch.fLow  = - .5 * ch.bw;
  ch.fHigh = + .5 * ch.bw;

  if (this->ui->inspectorCombo->currentIndex() == -1)
    return;

  this->panelConfig->inspFactory =
    this->ui->inspectorCombo->currentData().value<QString>().toStdString();

  this->panelConfig->inspectorClass = this->getInspectorClass();

  this->mediator()->openInspectorTab(
        this->panelConfig->inspFactory.c_str(),
        this->panelConfig->inspectorClass.c_str(),
        ch,
        this->ui->preciseCheck->isChecked());
}

void
InspToolWidget::onBandwidthChanged(double bw)
{
  // TODO: getBandwidth, setFilterBandwidth??
  this->mediator()->getMainSpectrum()->setFilterBandwidth(SCAST(unsigned, bw));
}

void
InspToolWidget::onPreciseChanged(void)
{
  this->panelConfig->precise = this->ui->preciseCheck->isChecked();
}

void
InspToolWidget::onPressAutoSquelch(void)
{
  if (!this->autoSquelch) {
    this->enableAutoSquelch();
    this->ui->autoSquelchButton->setText("Measuring...");
  } else {
    this->cancelAutoSquelch();
    if (this->data.size() > 0)
      this->openTimeWindow();
  }
}

void
InspToolWidget::onReleaseAutoSquelch(void)
{
  if (this->autoSquelch) {
    if (this->powerSamples == 0) {
      cancelAutoSquelch();
    } else {
      this->hangLength =
          1e-3 * this->ui->hangTimeSpin->value() * this->timeWindowFs;
      this->history.resize(2 * this->hangLength);
      std::fill(this->history.begin(), this->history.end(), 0);
      this->powerAccum /= this->powerSamples;
      this->powerSamples = 1;
      this->ui->autoSquelchButton->setText("Waiting...");
    }
  }
}

void
InspToolWidget::onToggleAutoSquelch(void)
{
  this->ui->autoSquelchButton->setChecked(this->autoSquelch);
}

void
InspToolWidget::onPressHold(void)
{
  startRawCapture();
}

void
InspToolWidget::onReleaseHold(void)
{
  stopRawCapture();

  if (this->data.size() > 0)
    this->openTimeWindow();
}

void
InspToolWidget::onTimeWindowConfigChanged(void)
{
  this->panelConfig->palette = this->timeWindow->getPalette();
  this->panelConfig->paletteOffset = this->timeWindow->getPaletteOffset();
  this->panelConfig->paletteContrast = this->timeWindow->getPaletteContrast();
}

void
InspToolWidget::onTriggerSNRChanged(double val)
{
  this->panelConfig->autoSquelchTriggerSNR = static_cast<SUFLOAT>(val);
}


// Main UI slots
void
InspToolWidget::onSpectrumBandwidthChanged(void)
{
  this->setBandwidth(this->mediator()->getMainSpectrum()->getBandwidth());
}

void
InspToolWidget::onSpectrumFrequencyChanged(void)
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

  this->resetRawInspector(SCAST(qreal, request.equivRate));
}

void
InspToolWidget::onCancelled(Suscan::AnalyzerRequest const &)
{

}

void
InspToolWidget::onError(Suscan::AnalyzerRequest const &, std::string const &error)
{
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
