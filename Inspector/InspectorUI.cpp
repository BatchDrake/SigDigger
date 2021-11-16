//
//    InspectorUI.h: Dynamic inspector UI
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

#include "InspectorUI.h"
#include "ui_Inspector.h"

#include "AskControl.h"
#include "GainControl.h"
#include "AfcControl.h"
#include "ToneControl.h"
#include "MfControl.h"
#include "EqualizerControl.h"
#include "ClockRecovery.h"

#include "AppConfig.h"

#include <QFileDialog>
#include <QMessageBox>
#include <Suscan/Library.h>
#include <QMessageBox>
#include <SuWidgetsHelpers.h>
#include <SigDiggerHelpers.h>
#include <FrequencyCorrectionDialog.h>
#include <QMessageBox>
#include <suscan.h>
#include <iomanip>
#include <fcntl.h>

using namespace SigDigger;

InspectorUI::InspectorUI(
    QWidget *owner,
    Suscan::Config *config)
{
  struct timeval tv;

  this->ui = new Ui::Inspector();
  this->config = config;
  this->owner  = owner;

  this->ui->setupUi(owner);

  this->haveQth = suscan_get_qth(&this->qth);

  this->symViewTab = new SymViewTab(this->ui->toolTab);
  this->ui->toolTab->addTab(this->symViewTab, "Symbol stream");

  this->wfTab = new WaveformTab(this->ui->toolTab);
  this->ui->toolTab->addTab(this->wfTab, "Waveform");

  this->tvTab = new TVProcessorTab(this->ui->toolTab, this->getBaudRateFloat());
  this->ui->toolTab->addTab(this->tvTab, "Analog TV");

  this->fcDialog = new FrequencyCorrectionDialog(
        owner,
        0,
        ColorConfig());

  gettimeofday(&tv, NULL);
  this->fcDialog->resetTimestamp(tv);
  if (this->haveQth)
    this->fcDialog->setQth(this->qth);

  if (this->config->hasPrefix("ask")) {
    this->decider.setDecisionMode(Decider::MODULUS);
    this->tvTab->setDecisionMode(Decider::MODULUS);
    this->decider.setMinimum(0);
    this->decider.setMaximum(1);

    this->ui->histogram->overrideDisplayRange(1);
    this->ui->histogram->overrideUnits("");
    this->ui->histogram->overrideDataRange(1);
  } else if (this->config->hasPrefix("afc")) {
    this->decider.setDecisionMode(Decider::ARGUMENT);
    this->tvTab->setDecisionMode(Decider::ARGUMENT);
    this->decider.setMinimum(-PI);
    this->decider.setMaximum(PI);

    this->ui->histogram->overrideDataRange(2 * M_PI);
    this->ui->histogram->overrideDisplayRange(360);
    this->ui->histogram->overrideUnits("º");
  } else if (this->config->hasPrefix("fsk")) {
    this->decider.setDecisionMode(Decider::ARGUMENT);
    this->tvTab->setDecisionMode(Decider::ARGUMENT);
    this->decider.setMinimum(-PI);
    this->decider.setMaximum(PI);

    this->ui->histogram->overrideDataRange(2 * M_PI);
    this->ui->histogram->overrideUnits("Hz");
  }

  this->initUi();
  this->connectAll();

  // Refresh UI
  this->refreshUi();

  // Force refresh of waterfall
  this->onRangeChanged();
  this->onAspectSliderChanged(this->ui->aspectSlider->value());
}

InspectorUI::~InspectorUI()
{
  delete this->ui;

  if (this->dataSaver != nullptr)
    delete this->dataSaver;

  if (this->socketForwarder != nullptr)
    delete this->socketForwarder;
}

void
InspectorUI::initUi(void)
{
  this->ui->wfSpectrum->setFreqUnits(1);

  SigDiggerHelpers::instance()->populatePaletteCombo(this->ui->paletteCombo);

  this->setPalette("Suscan");

  this->populateUnits();
  this->populate();

  // Configure throttleable widgets
  this->throttle.setCpuBurn(false);
  this->ui->constellation->setThrottleControl(&this->throttle);
  this->symViewTab->setThrottleControl(&this->throttle);
  this->wfTab->setThrottleControl(&this->throttle);
  this->ui->transition->setThrottleControl(&this->throttle);
  this->ui->histogram->setThrottleControl(&this->throttle);
  this->ui->histogram->setDecider(&this->decider);
  this->ui->histogram->reset();
  this->ui->wfSpectrum->setCenterFreq(0);
  this->ui->wfSpectrum->resetHorizontalZoom();
  this->ui->wfSpectrum->setFftPlotColor(QColor(255, 255, 0));

  this->ui->centerLabel->setFixedWidth(
        SuWidgetsHelpers::getWidgetTextWidth(
          this->ui->centerLabel,
          "XXX.XXXXXXXXX XHz"));

  this->ui->bwLabel->setFixedWidth(
        SuWidgetsHelpers::getWidgetTextWidth(
          this->ui->bwLabel,
          "XXX.XXXXXXXXX XHz"));

  // Refresh Bps
  this->setBps(1);
}

void
InspectorUI::adjustSizes(void)
{
  QList<int> sizes;
  int width = this->ui->scrollAreaWidgetContents->sizeHint().width() - 25;

  // Adjust splitter
  sizes.append(width);
  sizes.append(this->ui->splitter->width() - width);

  this->ui->splitter->setSizes(sizes);
}

float
InspectorUI::getZeroPoint(void) const
{
  return static_cast<float>(this->ui->zeroPointSpin->value());
}

void
InspectorUI::setBasebandRate(unsigned int rate)
{
  this->basebandSampleRate = rate;
  this->ui->loLcd->setMin(-static_cast<int>(rate) / 2);
  this->ui->loLcd->setMax(static_cast<int>(rate) / 2);

  this->ui->scFreqSpin->setMinimum(-static_cast<qreal>(rate) / 2);
  this->ui->scFreqSpin->setMaximum(static_cast<qreal>(rate) / 2);
  this->ui->scBandwidth->setMaximum(static_cast<qreal>(rate));
}

void
InspectorUI::setSampleRate(float rate)
{
  this->sampleRate = rate;
  this->ui->sampleRateLabel->setText(
        "Sample rate: "
        + SuWidgetsHelpers::formatQuantity(
            static_cast<qreal>(rate),
            4,
            "sp/s"));
  this->ui->bwLcd->setMin(0);
  this->ui->bwLcd->setMax(static_cast<qint64>(rate));

  if (this->config->hasPrefix("fsk"))
    this->ui->histogram->overrideDisplayRange(static_cast<qreal>(rate));

  for (auto p : this->controls)
    p->setSampleRate(rate);
}

void
InspectorUI::setBandwidth(unsigned int bandwidth)
{
  // More COBOL
  this->ui->bwLcd->setValue(static_cast<int>(bandwidth));
}

void
InspectorUI::setQth(xyz_t const &site)
{
  this->qth = site;
  this->haveQth = true;
  this->fcDialog->setQth(site);
}

void
InspectorUI::setLo(int lo)
{
  this->ui->loLcd->setValue(lo);
}

void
InspectorUI::refreshInspectorCtls(void)
{
  for (auto p : this->controls)
    p->refreshUi();
}

unsigned int
InspectorUI::getBandwidth(void) const
{
  return static_cast<unsigned int>(this->ui->bwLcd->getValue());
}

int
InspectorUI::getLo(void) const
{
  return static_cast<int>(this->ui->loLcd->getValue());
}

bool
InspectorUI::setPalette(std::string const &str)
{
  int index = SigDiggerHelpers::instance()->getPaletteIndex(str);

  if (index < 0)
    return false;

  this->ui->wfSpectrum->setPalette(
        SigDiggerHelpers::instance()->getPalette(index)->getGradient());
  this->ui->paletteCombo->setCurrentIndex(index);

  return true;
}

void
InspectorUI::addSpectrumSource(Suscan::SpectrumSource const &src)
{
  this->spectsrcs.push_back(src);
  this->ui->spectrumSourceCombo->addItem(QString::fromStdString(src.desc));
}

void
InspectorUI::addEstimator(Suscan::Estimator const &estimator)
{
  int position = static_cast<int>(this->estimators.size());
  EstimatorControl *ctl;
  this->ui->estimatorsGrid->setAlignment(Qt::AlignTop);

  this->estimators.push_back(estimator);

  ctl = new EstimatorControl(this->owner, estimator);
  this->estimatorCtls[estimator.id] = ctl;

  this->ui->estimatorsGrid->addWidget(ctl, position, 0, Qt::AlignTop);

  connect(
        ctl,
        SIGNAL(estimatorChanged(Suscan::EstimatorId, bool)),
        this,
        SLOT(onToggleEstimator(Suscan::EstimatorId, bool)));

  connect(
        ctl,
        SIGNAL(apply(QString, float)),
        this,
        SLOT(onApplyEstimation(QString, float)));
}

void
InspectorUI::connectAll()
{

  connect(
        this->ui->fpsSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onFPSChanged(void)));

  connect(
        this->ui->burnCPUButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onCPUBurnClicked()));

  connect(
        this->ui->resetFpsButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onFPSReset()));


  connect(
        this->ui->paletteCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onSpectrumConfigChanged()));

  connect(
        this->ui->spectrumSourceCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onSpectrumSourceChanged()));

  connect(
        this->ui->rangeSlider,
        SIGNAL(valuesChanged(int, int)),
        this,
        SLOT(onRangeChanged(void)));

  connect(
        this->ui->peakDetectionButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSpectrumConfigChanged()));

  connect(
        this->ui->peakHoldButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSpectrumConfigChanged()));

  connect(
        this->ui->snrButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleSNR()));

  connect(
        this->ui->snrResetButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onResetSNR()));

  connect(
        this->ui->loLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onChangeLo(void)));

  connect(
        this->ui->bwLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onChangeBandwidth(void)));

  connect(
        this->ui->aspectSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onAspectSliderChanged(int)));

  connect(
        this->ui->wfSpectrum,
        SIGNAL(pandapterRangeChanged(float, float)),
        this,
        SLOT(onPandapterRangeChanged(float, float)));

  connect(
        this->ui->unitsCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onUnitChanged(void)));

  connect(
        this->ui->zeroPointSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onZeroPointChanged(void)));

  connect(
        this->ui->gainSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onGainChanged(void)));

  connect(
        this->ui->wfSpectrum,
        SIGNAL(newFilterFreq(int, int)),
        this,
        SLOT(onNewBandwidth(int, int)));

  connect(
        this->ui->wfSpectrum,
        SIGNAL(newDemodFreq(qint64, qint64)),
        this,
        SLOT(onNewOffset()));

  connect(
        this->ui->dopplerSettingsButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onOpenDopplerSettings(void)));

  connect(
        this->fcDialog,
        SIGNAL(accepted(void)),
        this,
        SLOT(onDopplerAccepted(void)));

  connect(
        this->ui->scFreqSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onScFrequencyChanged(void)));

  connect(
        this->ui->scBandwidth,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onScBandwidthChanged(void)));

  connect(
        this->ui->scOpenButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onScOpenInspector(void)));
}

void
InspectorUI::connectDataSaver()
{
  connect(
        this->dataSaver,
        SIGNAL(stopped(void)),
        this,
        SLOT(onSaveError(void)));

  connect(
        this->dataSaver,
        SIGNAL(swamped(void)),
        this,
        SLOT(onSaveSwamped(void)));

  connect(
        this->dataSaver,
        SIGNAL(dataRate(qreal)),
        this,
        SLOT(onSaveRate(qreal)));

  connect(
        this->dataSaver,
        SIGNAL(commit(void)),
        this,
        SLOT(onCommit(void)));
}

void
InspectorUI::connectNetForwarder()
{
  connect(
        this->socketForwarder,
        SIGNAL(stopped(void)),
        this,
        SLOT(onNetError(void)));

  connect(
        this->socketForwarder,
        SIGNAL(swamped(void)),
        this,
        SLOT(onNetSwamped(void)));

  connect(
        this->socketForwarder,
        SIGNAL(dataRate(qreal)),
        this,
        SLOT(onNetRate(qreal)));

  connect(
        this->socketForwarder,
        SIGNAL(commit(void)),
        this,
        SLOT(onNetCommit(void)));

  connect(
        this->socketForwarder,
        SIGNAL(ready(void)),
        this,
        SLOT(onNetReady(void)));
}

std::string
InspectorUI::captureFileName(void) const
{
  unsigned int i = 0;
  std::string path;

  do {
    std::ostringstream os;

    os << "channel-capture-"
       << this->getClassName()
       << "-"
       << std::to_string(this->getBaudRate())
       << "-baud-"
       << std::setw(4)
       << std::setfill('0')
       << ++i
       << ".raw";
    path = this->saverUI->getRecordSavePath() + "/" + os.str();
  } while (access(path.c_str(), F_OK) != -1);

  return path;
}

bool
InspectorUI::installNetForwarder(void)
{
  if (this->socketForwarder == nullptr) {
    this->socketForwarder = new SocketForwarder(
          this->netForwarderUI->getHost(),
          this->netForwarderUI->getPort(),
          this->netForwarderUI->getFrameLen(),
          this->netForwarderUI->getTcp(),
          this);
    this->recordingRate = this->getBaudRate();
    this->socketForwarder->setSampleRate(recordingRate);
    connectNetForwarder();

    return true;
  }

  return false;
}

void
InspectorUI::uninstallNetForwarder(void)
{
  if (this->socketForwarder)
    this->socketForwarder->deleteLater();
  this->socketForwarder = nullptr;
}

void
InspectorUI::setTunerFrequency(SUFREQ freq)
{
  this->fcDialog->setFrequency(freq);
}

void
InspectorUI::setRealTime(bool realTime)
{
  this->fcDialog->setRealTime(realTime);
}

void
InspectorUI::setTimeStamp(struct timeval const &tv)
{
  this->fcDialog->setTimestamp(tv);
}

void
InspectorUI::setTimeLimits(
    struct timeval const &start,
    struct timeval const &end)
{
  this->fcDialog->setTimeLimits(start, end);
}

bool
InspectorUI::installDataSaver(void)
{
  if (this->dataSaver == nullptr) {
    std::string path = this->captureFileName();
    this->fd = open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (this->fd == -1) {
      std::string path;

      path = "Failed to open capture file <pre>" +
          path +
          "</pre>: " +
          std::string(strerror(errno));

      (void) QMessageBox::critical(
            this->owner,
            "Save demodulator output",
            QString::fromStdString(path),
            QMessageBox::Close);

      return false;
    }

    this->dataSaver = new FileDataSaver(this->fd, this);
    this->recordingRate = this->getBaudRate();
    this->dataSaver->setSampleRate(recordingRate);
    connectDataSaver();

    return true;
  }

  return false;
}

void
InspectorUI::uninstallDataSaver(void)
{
  if (this->dataSaver != nullptr)
    this->dataSaver->deleteLater();
  this->dataSaver = nullptr;

  if (this->fd != -1) {
    close(this->fd);
    this->fd = -1;
  }
}

void
InspectorUI::onToggleSNR(void)
{
  this->estimating = this->ui->snrButton->isChecked();

  if (this->estimating) {
    this->estimator.setSigma(1.f);
    this->estimator.setAlpha(1.f / (this->decider.getIntervals()));
    gettimeofday(&this->last_estimator_update, nullptr);
  } else {
    std::vector<float> empty;
    this->ui->histogram->setSNRModel(empty);
  }

  this->ui->snrResetButton->setEnabled(this->estimating);
}

void
InspectorUI::onResetSNR(void)
{
  this->estimator.setSigma(1.f);

}


void
InspectorUI::feed(const SUCOMPLEX *data, unsigned int size)
{  
  this->ui->constellation->feed(data, size);
  this->ui->histogram->feed(data, size);

  if (this->estimating) {
    struct timeval tv, res;
    this->estimator.feed(this->ui->histogram->getHistory());
    gettimeofday(&tv, nullptr);

    timersub(&this->last_estimator_update, &tv, &res);

    if (res.tv_sec > 0 || res.tv_usec > 100000) {
      this->ui->histogram->setSNRModel(this->estimator.getModel());
      this->ui->snrLabel->setText(
            QString::number(
              floor(20. * log10(static_cast<qreal>(this->estimator.getSNR()))))
            + " dB");
      this->last_estimator_update = tv;
    }
  }

  // Decision happens here.
  if (this->symViewTab->isRecording()) {
    if (this->decider.getBps() > 0) {
      this->decider.feed(data, size);

      this->symViewTab->feed(this->decider.get());
      this->ui->transition->feed(this->decider.get());
    }
  }

  if (this->tvTab->isEnabled())
    this->tvTab->feed(data, size);

  if (this->wfTab->isRecording())
    this->wfTab->feed(data, size);

  if (this->recording || this->forwarding) {
    const SUCOMPLEX *chunk;

    if (this->ui->dataVarCombo->currentIndex() == 0) {
      // Decision space
      if (this->buffer.size() < size)
        this->buffer.resize(size);

      switch (this->decider.getDecisionMode()) {
        case Decider::MODULUS:
          for (unsigned i = 0; i < size; ++i)
            this->buffer[i] = SU_C_ABS(data[i]);
          break;

        case Decider::ARGUMENT:
          for (unsigned i = 0; i < size; ++i)
            this->buffer[i] = SU_C_ARG(I * data[i]) / PI;
          break;
      }

      chunk = this->buffer.data();
    } else {
      // Raw I/Q data
      chunk = data;
    }

    if (this->recording)
      this->dataSaver->write(chunk, size);

    if (this->forwarding)
      this->socketForwarder->write(chunk, size);
  }
}

void
InspectorUI::feedSpectrum(const SUFLOAT *data, SUSCOUNT len, SUSCOUNT rate)
{
  if (this->lastRate != rate) {
    this->ui->wfSpectrum->setSampleRate(static_cast<float>(rate));
    this->lastRate = rate;
  }

  this->fftData.resize(len);
  this->fftData.assign(data, data + len);

  this->ui->wfSpectrum->setNewFftData(
        static_cast<float *>(this->fftData.data()),
        static_cast<int>(len));

  if (this->lastLen != len) {
    this->ui->wfSpectrum->resetHorizontalZoom();
    this->lastLen = len;
  }
}

void
InspectorUI::updateEstimator(Suscan::EstimatorId id, float val)
{
  // XXX: Things may change in the future. Null value does not imply
  // invalid estimation

  if (fabsf(val) > 1e-6f) {
    this->estimatorCtls[id]->setParameterValue(val);
    this->estimatorCtls[id]->setParameterAvailable(true);
  } else {
    this->estimatorCtls[id]->setParameterAvailable(false);
  }
}

void
InspectorUI::setState(enum State state)
{
  this->state = state;

  switch (state) {
    case ATTACHED:
      break;

    case DETACHED:
      break;
  }

  this->refreshUi();
}

enum InspectorUI::State
InspectorUI::getState(void) const
{
  return this->state;
}

void
InspectorUI::pushControl(InspectorCtl *ctl)
{
  int position = static_cast<int>(this->controls.size());

  this->controls.push_back(ctl);

  this->ui->controlsGrid->addWidget(ctl, position, 0, Qt::AlignTop);

  connect(
        ctl,
        SIGNAL(changed()),
        this,
        SLOT(onInspectorControlChanged(void)));
}

void
InspectorUI::populateUnits(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  this->ui->unitsCombo->clear();

  for (auto p: sus->getSpectrumUnitMap())
    this->ui->unitsCombo->addItem(QString::fromStdString(p.name));

  this->ui->unitsCombo->setCurrentIndex(0);
  this->ui->zeroPointSpin->setValue(0.0);
}

void
InspectorUI::addForwarderWidget(QWidget *widget)
{
  int row = this->ui->forwarderGrid->rowCount();

  this->ui->forwarderGrid->addWidget(widget, row, 0, 1, 2, Qt::AlignTop);
}

void
InspectorUI::populate(void)
{
  this->ui->controlsGrid->setAlignment(Qt::AlignTop);
  this->ui->forwarderGrid->setAlignment(Qt::AlignTop);

  if (this->config->hasPrefix("agc"))
    this->pushControl(new GainControl(this->owner, this->config));
  if (this->config->hasPrefix("afc"))
    this->pushControl(new AfcControl(this->owner, this->config));
  if (this->config->hasPrefix("ask"))
    this->pushControl(new AskControl(this->owner, this->config));
  if (this->config->hasPrefix("fsk"))
    this->pushControl(new ToneControl(this->owner, this->config));
  if (this->config->hasPrefix("mf"))
    this->pushControl(new MfControl(this->owner, this->config));
  if (this->config->hasPrefix("equalizer"))
    this->pushControl(new EqualizerControl(this->owner, this->config));
  if (this->config->hasPrefix("clock"))
    this->pushControl(new ClockRecovery(this->owner, this->config));

  // Add data forwarder objects

  this->saverUI = new DataSaverUI(this->owner);

  this->addForwarderWidget(this->saverUI);

  connect(
        this->saverUI,
        SIGNAL(recordStateChanged(bool)),
        this,
        SLOT(onToggleRecord(void)));

  this->netForwarderUI = new NetForwarderUI(this->owner);

  this->addForwarderWidget(this->netForwarderUI);

  connect(
        this->netForwarderUI,
        SIGNAL(forwardStateChanged(bool)),
        this,
        SLOT(onToggleNetForward(void)));

}

void
InspectorUI::redrawMeasures(void)
{
  this->ui->centerLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          static_cast<qreal>(
            this->ui->wfSpectrum->getFilterOffset()),
          6,
          "Hz",
          true));
  this->ui->scFreqSpin->setValue(
        static_cast<qreal>(
          this->ui->wfSpectrum->getFilterOffset()));

  this->ui->bwLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          static_cast<qreal>(this->ui->wfSpectrum->getFilterBw()),
          6,
          "Hz"));
  this->ui->scBandwidth->setValue(
        static_cast<qreal>(
          this->ui->wfSpectrum->getFilterBw()));

}

void
InspectorUI::refreshUi(void)
{
  bool enabled = this->state == ATTACHED;

  for (auto p  = this->controls.begin(); p != this->controls.end(); ++p)
    (*p)->setEnabled(enabled);

  this->ui->spectrumSourceCombo->setEnabled(enabled);
  this->ui->snrButton->setEnabled(enabled);
  this->ui->snrResetButton->setEnabled(enabled);
  this->symViewTab->setEnabled(enabled);
  this->ui->loLcd->setEnabled(enabled);
  this->ui->bwLcd->setEnabled(enabled);
  this->saverUI->setEnabled(enabled && this->recordingRate != 0);
  this->netForwarderUI->setEnabled(enabled && this->recordingRate != 0);
}

void
InspectorUI::setBps(unsigned int bps)
{
  if (this->bps != bps) {
    this->decider.setBps(bps);
    this->estimator.setBps(bps);
    this->symViewTab->setBitsPerSymbol(bps);
    this->ui->constellation->setOrderHint(bps);
    this->ui->transition->setOrderHint(bps);
    this->ui->histogram->setDecider(&this->decider);
    this->bps = bps; // For caching
  }
}

unsigned int
InspectorUI::getBaudRate(void) const
{
  return static_cast<unsigned int>(this->getBaudRateFloat());
}

SUFLOAT
InspectorUI::getBaudRateFloat(void) const
{
  const Suscan::FieldValue *val;
  SUFLOAT baud = 1.;

  // Check baudrate
  if ((val = this->config->get("clock.baud")) != nullptr)
    baud = val->getFloat();

  return baud;
}

std::string
InspectorUI::getClassName(void) const
{
  if (this->config->hasPrefix("ask"))
    return "AM";
  else if (this->config->hasPrefix("psk"))
    return "PM";
  else if (this->config->hasPrefix("fsk"))
    return "FM";

  return "UNKNOWN";
}

unsigned int
InspectorUI::getBps(void) const
{
  const Suscan::FieldValue *val;
  unsigned int bps = 0;

  // Check if bits per symbol have changed
  if ((val = this->config->get("afc.bits-per-symbol")) != nullptr)
    bps = static_cast<unsigned int>(val->getUint64());
  else if ((val = this->config->get("fsk.bits-per-symbol")) != nullptr)
    bps = static_cast<unsigned int>(val->getUint64());
  else if ((val = this->config->get("ask.bits-per-symbol")) != nullptr)
    bps = static_cast<unsigned int>(val->getUint64());

  if (bps == 0)
    bps = 1;

  return bps;
}

//////////////////////////////////// Slots ////////////////////////////////////
void
InspectorUI::onInspectorControlChanged(void)
{
  unsigned int newRate = this->getBaudRate();
  unsigned int oldRate = this->recordingRate;
  // Changing the newRate has a set of implications

  if (newRate != oldRate) {
    this->tvTab->setSampleRate(this->getBaudRateFloat());
    this->wfTab->setSampleRate(this->getBaudRateFloat());

    if (this->recording) {
      this->recording = false;
      if (newRate == 0) {
        this->uninstallDataSaver();
      } else if (newRate != this->recordingRate) {
        this->uninstallDataSaver();
        this->recording = this->installDataSaver();
      }

      this->saverUI->setRecordState(this->recording);
    }

    if (this->forwarding) {
      this->forwarding = false;
      if (newRate == 0) {
        this->uninstallNetForwarder();
      } else if (newRate != this->recordingRate) {
        this->uninstallNetForwarder();
        this->forwarding = this->installNetForwarder();
      }

      this->saverUI->setRecordState(this->recording);
    }
  }

  this->saverUI->setEnabled(newRate != 0);
  this->netForwarderUI->setEnabled(newRate != 0);

  this->setBps(this->getBps());

  this->ui->histogram->reset();

  emit configChanged();
}


void
InspectorUI::onCPUBurnClicked(void)
{
  bool burn = this->ui->burnCPUButton->isChecked();

  this->throttle.setCpuBurn(burn);
  this->ui->fpsSpin->setEnabled(!burn);
}

void
InspectorUI::setAppConfig(AppConfig const &cfg)
{
  ColorConfig const &colors = cfg.colors;
  InspectorPanelConfig panelConfig;
  FftPanelConfig fftConfig;

  fftConfig.deserialize(cfg.fftConfig->serialize());
  panelConfig.deserialize(cfg.inspectorConfig->serialize());

  this->fcDialog->setColorConfig(colors);

  // Set colors according to application config
  this->ui->constellation->setForegroundColor(colors.constellationForeground);
  this->ui->constellation->setBackgroundColor(colors.constellationBackground);
  this->ui->constellation->setAxesColor(colors.constellationAxes);

  this->ui->transition->setForegroundColor(colors.transitionForeground);
  this->ui->transition->setBackgroundColor(colors.transitionBackground);
  this->ui->transition->setAxesColor(colors.transitionAxes);

  this->ui->histogram->setForegroundColor(colors.histogramForeground);
  this->ui->histogram->setBackgroundColor(colors.histogramBackground);
  this->ui->histogram->setAxesColor(colors.histogramAxes);

  // this->ui->histogram->setModelColor(colors.histogramModel);

  this->ui->bwLcd->setForegroundColor(colors.lcdForeground);
  this->ui->bwLcd->setBackgroundColor(colors.lcdBackground);

  this->ui->loLcd->setForegroundColor(colors.lcdForeground);
  this->ui->loLcd->setBackgroundColor(colors.lcdBackground);

  this->ui->wfSpectrum->setFftPlotColor(colors.spectrumForeground);
  this->ui->wfSpectrum->setFftBgColor(colors.spectrumBackground);
  this->ui->wfSpectrum->setFftAxesColor(colors.spectrumAxes);
  this->ui->wfSpectrum->setFftTextColor(colors.spectrumText);
  this->ui->wfSpectrum->setFilterBoxColor(colors.filterBox);

  // Set SymView colors
  this->symViewTab->setColorConfig(colors);

  // Set Waveform colors
  this->wfTab->setColorConfig(colors);
  this->wfTab->setPalette(panelConfig.palette);
  this->wfTab->setPaletteOffset(panelConfig.paletteOffset);
  this->wfTab->setPaletteContrast(panelConfig.paletteContrast);

  // Set palette
  (void) this->setPalette(fftConfig.palette);
}

void
InspectorUI::setOrbitReport(Suscan::OrbitReport const &report)
{
  this->ui->vlosLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          report.getVlosVelocity(),
          3,
          "m/s",
          true));

  this->ui->dopplerLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          static_cast<qreal>(report.getFrequencyCorrection()),
          3,
          "Hz",
          true));


  this->ui->azLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          SU_RAD2DEG(report.getAzel().azimuth),
          0,
          "º",
          false));

  this->ui->elLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          SU_RAD2DEG(report.getAzel().elevation),
          0,
          "º",
          true));

  this->ui->distanceLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          SU_RAD2DEG(report.getAzel().distance),
          3,
          "m",
          true));

  this->ui->airmassLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          1 / cos(.5 * PI - report.getAzel().elevation),
          3,
          "",
          true));

  this->ui->visibleLabel->setText(
        report.getAzel().elevation < 0 ? "No" : "Yes");

  this->fcDialog->setTimestamp(report.getRxTime());
}

void
InspectorUI::notifyDisableCorrection(void)
{
  this->ui->vlosLabel->setText("N / A");
  this->ui->dopplerLabel->setText("N / A");
  this->ui->azLabel->setText("N / A");
  this->ui->elLabel->setText("N / A");
  this->ui->distanceLabel->setText("N / A");
  this->ui->airmassLabel->setText("N / A");
  this->ui->visibleLabel->setText("N / A");
}

void
InspectorUI::setZeroPoint(float zp)
{
  this->ui->zeroPointSpin->setValue(static_cast<double>(zp));
  this->ui->wfSpectrum->setZeroPoint(this->currentUnit.zeroPoint + zp);
}

void
InspectorUI::onFPSReset(void)
{
  this->ui->fpsSpin->setValue(THROTTLE_CONTROL_DEFAULT_RATE);
  this->ui->burnCPUButton->setChecked(false);
  this->throttle.setCpuBurn(false);
  this->ui->fpsSpin->setEnabled(true);
}

void
InspectorUI::onFPSChanged(void)
{
  this->throttle.setRate(
        static_cast<unsigned int>(this->ui->fpsSpin->value()));
}

void
InspectorUI::onSpectrumConfigChanged(void)
{
  int index = this->ui->paletteCombo->currentIndex();
  this->ui->wfSpectrum->setPalette(
        SigDiggerHelpers::instance()->getPalette(index)->getGradient());
  this->ui->wfSpectrum->setPeakDetection(
        this->ui->peakDetectionButton->isChecked(), 3);

  this->ui->wfSpectrum->setPeakHold(this->ui->peakHoldButton->isChecked());
}

void
InspectorUI::onSpectrumSourceChanged(void)
{
  emit setSpectrumSource(
        static_cast<unsigned>(this->ui->spectrumSourceCombo->currentIndex()));
}

void
InspectorUI::onRangeChanged(void)
{
  if (!this->adjusting) {
    this->ui->wfSpectrum->setPandapterRange(
          this->ui->rangeSlider->minimumValue(),
          this->ui->rangeSlider->maximumValue());

    this->ui->wfSpectrum->setWaterfallRange(
          this->ui->rangeSlider->minimumValue(),
          this->ui->rangeSlider->maximumValue());
  }
}

void
InspectorUI::onChangeLo(void)
{
  emit loChanged();
}

void
InspectorUI::onChangeBandwidth(void)
{
  emit bandwidthChanged();
}

void
InspectorUI::onToggleEstimator(Suscan::EstimatorId id, bool enabled)
{
  emit toggleEstimator(id, enabled);
}

void
InspectorUI::onApplyEstimation(QString name, float value)
{
  emit applyEstimation(name, value);
}

void
InspectorUI::onAspectSliderChanged(int ratio)
{
  this->ui->wfSpectrum->setPercent2DScreen(ratio);
}

void
InspectorUI::onPandapterRangeChanged(float min, float max)
{
  bool adjusting = this->adjusting;
  this->adjusting = true;

  this->ui->rangeSlider->setMinimumPosition(static_cast<int>(min));
  this->ui->rangeSlider->setMaximumPosition(static_cast<int>(max));

  this->ui->wfSpectrum->setWaterfallRange(min, max);

  this->adjusting = adjusting;
}

// Datasaver
void
InspectorUI::onToggleRecord(void)
{
  bool recording = false;

  if (this->saverUI->getRecordState()) {
    recording = this->installDataSaver();
  } else {
    this->uninstallDataSaver();
  }

  this->recording = recording;

  this->saverUI->setRecordState(recording);
}


void
InspectorUI::onSaveError(void)
{
  if (this->dataSaver != nullptr) {
    QString error = this->dataSaver->getLastError();
    this->recording = false;
    this->uninstallDataSaver();

    QMessageBox::warning(
              this->owner,
              "SigDigger error",
              "Capture interrupted due to errors. " + error,
              QMessageBox::Ok);

    this->saverUI->setRecordState(false);
  }
}

void
InspectorUI::onSaveSwamped(void)
{
  if (this->dataSaver != nullptr) {
    this->recording = false;
    this->uninstallDataSaver();
    QMessageBox::warning(
          this->owner,
          "SigDigger error",
          "Capture thread swamped. Maybe your storage device is too slow",
          QMessageBox::Ok);

    this->saverUI->setRecordState(false);
  }
}

void
InspectorUI::onSaveRate(qreal rate)
{
  this->saverUI->setIORate(rate);
}

void
InspectorUI::onCommit(void)
{
  this->saverUI->setCaptureSize(this->dataSaver->getSize());
}


// Net Forwarder
void
InspectorUI::onToggleNetForward(void)
{
  bool forwarding = false;

  if (this->netForwarderUI->getForwardState()) {
    forwarding = this->installNetForwarder();
  } else {
    this->forwarding = false;
    this->uninstallNetForwarder();
  }

  this->forwarding = forwarding;

  this->netForwarderUI->setPreparing(this->forwarding);
  this->netForwarderUI->setForwardState(this->forwarding);
}

void
InspectorUI::onNetReady(void)
{
  this->netForwarderUI->setPreparing(false);
}

void
InspectorUI::onNetError(void)
{
  if (this->socketForwarder != nullptr) {
    QString error = this->socketForwarder->getLastError();
    this->forwarding = false;
    this->uninstallNetForwarder();
    QMessageBox::warning(
              this->owner,
              "SigDigger error",
              "Network forwarding was interrupted. " + error,
              QMessageBox::Ok);

    this->netForwarderUI->setForwardState(false);
  }
}

void
InspectorUI::onNetSwamped(void)
{
  if (this->socketForwarder != nullptr) {
    this->forwarding = false;
    this->uninstallNetForwarder();

    QMessageBox::warning(
          this->owner,
          "SigDigger error",
          "Capture thread swamped. Maybe your network interface is too slow.",
          QMessageBox::Ok);

    this->netForwarderUI->setForwardState(false);
  }
}

void
InspectorUI::onNetRate(qreal rate)
{
  this->netForwarderUI->setIORate(rate);
}

void
InspectorUI::onNetCommit(void)
{
  this->netForwarderUI->setCaptureSize(this->socketForwarder->getSize());
}

void
InspectorUI::onUnitChanged(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  float currZPdB = this->zeroPointToDb();
  float newZp;
  std::string name = this->ui->unitsCombo->currentText().toStdString();
  auto it = sus->getSpectrumUnitFrom(name);

  this->ui->zeroPointSpin->setSuffix(" " + QString::fromStdString(name));

  if (it != sus->getLastSpectrumUnit()) {
    this->currentUnit = *it;
  } else {
    this->currentUnit.name      = "dBFS";
    this->currentUnit.dBPerUnit = 1.f;
    this->currentUnit.zeroPoint = 0;
  }

  newZp = this->dbToZeroPoint(currZPdB);

  this->ui->wfSpectrum->setUnitName(
        QString::fromStdString(this->currentUnit.name));
  this->ui->wfSpectrum->setdBPerUnit(this->currentUnit.dBPerUnit);
  this->setZeroPoint(newZp);
}

void
InspectorUI::onZeroPointChanged(void)
{
  float currZP = static_cast<float>(this->ui->zeroPointSpin->value());
  this->ui->wfSpectrum->setZeroPoint(this->currentUnit.zeroPoint + currZP);
}

void
InspectorUI::onGainChanged(void)
{
  this->ui->wfSpectrum->setGain(
        static_cast<float>(this->ui->gainSpinBox->value()));
}

void
InspectorUI::onNewOffset(void)
{
  this->redrawMeasures();
}

void
InspectorUI::onNewBandwidth(int, int)
{
  this->redrawMeasures();
}

void
InspectorUI::onOpenDopplerSettings(void)
{
  if (this->haveQth) {
    this->fcDialog->show();
  } else {
    QMessageBox::warning(
          this->owner,
          "Doppler settings",
          "Doppler settings require RX location to be properly initialized. "
          "Plase set a receiver location in the settings dialog.");

  }
}

void
InspectorUI::onDopplerAccepted(void)
{
  if (this->fcDialog->isCorrectionEnabled()) {
    Suscan::Orbit orbit = this->fcDialog->getOrbit();
    emit setCorrection(orbit);
  } else {
    emit disableCorrection();
  }
}

void
InspectorUI::onScFrequencyChanged(void)
{
  this->ui->wfSpectrum->setFilterOffset(
        static_cast<qint64>(
          this->ui->scFreqSpin->value()));

}

void
InspectorUI::onScBandwidthChanged(void)
{
  this->ui->wfSpectrum->setHiLowCutFrequencies(
        -static_cast<qint64>(
          this->ui->scBandwidth->value() / 2),
        +static_cast<qint64>(
          this->ui->scBandwidth->value() / 2));
}

void
InspectorUI::onScOpenInspector(void)
{
  QString inspClass;

  if (this->ui->scASKRadio->isChecked())
    inspClass = "ask";
  else  if (this->ui->scFSKRadio->isChecked())
    inspClass = "fsk";
  else
    inspClass = "psk";

  emit openInspector(
        inspClass,
        static_cast<qint64>(this->ui->scFreqSpin->value()),
        this->ui->scBandwidth->value(),
        this->ui->scPreciseCheck->isChecked());
}
