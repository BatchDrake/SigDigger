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

#include <QFileDialog>
#include <QMessageBox>
#include <Suscan/Library.h>
#include <DefaultGradient.h>
#include <QMessageBox>

#include <iomanip>
#include <fcntl.h>

using namespace SigDigger;

InspectorUI::InspectorUI(
    QWidget *owner,
    Suscan::Config *config)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  unsigned int ndx;

  this->ui = new Ui::Inspector();
  this->config = config;
  this->owner  = owner;

  this->ui->setupUi(owner);

  if (config->hasPrefix("ask"))
    this->decider.setDecisionMode(Decider::MODULUS);

  this->ui->wfSpectrum->setFreqUnits(1);

  // TODO: put shared UI objects in a singleton
  this->palettes.push_back(Palette("Suscan", wf_gradient));
  for (auto i = sus->getFirstPalette();
       i != sus->getLastPalette();
       i++) {
    ndx = static_cast<unsigned int>(i - sus->getFirstPalette());

    this->palettes.push_back(Palette(*i));

    this->ui->paletteCombo->insertItem(
          static_cast<int>(ndx),
          QIcon(QPixmap::fromImage(this->palettes[ndx].getThumbnail())),
          QString::fromStdString(this->palettes[ndx].getName()),
          QVariant::fromValue(ndx));

    this->palettes[ndx].getThumbnail();
  }

  this->setPalette("Suscan");

  connect(
        this->ui->symView,
        SIGNAL(offsetChanged(unsigned int)),
        this,
        SLOT(onOffsetChanged(unsigned int)));

  connect(
        this->ui->symView,
        SIGNAL(strideChanged(unsigned int)),
        this,
        SLOT(onStrideChanged(unsigned int)));

  connect(
        this->ui->symViewScrollBar,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onScrollBarChanged(int)));

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
        this->ui->recordButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->autoScrollButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->autoFitButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->widthSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->offsetSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onSymViewControlsChanged()));

  connect(
        this->ui->saveButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveSymView()));

  connect(
        this->ui->clearButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onClearSymView()));

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

  this->populate();

  // Configure throttleable widgets
  this->throttle.setCpuBurn(false);
  this->ui->constellation->setThrottleControl(&this->throttle);
  this->ui->symView->setThrottleControl(&this->throttle);
  this->ui->transition->setThrottleControl(&this->throttle);
  this->ui->histogram->setThrottleControl(&this->throttle);
  this->ui->histogram->setDecider(&this->decider);
  this->ui->wfSpectrum->setCenterFreq(0);
  this->ui->wfSpectrum->resetHorizontalZoom();
  this->ui->wfSpectrum->setFftPlotColor(QColor(255, 255, 0));

  // Refresh Bps
  this->setBps(1);

  // Refresh UI
  this->refreshUi();
}

InspectorUI::~InspectorUI()
{
  delete this->ui;
}

void
InspectorUI::setBasebandRate(unsigned int rate)
{
  this->basebandSampleRate = rate;
  this->ui->loLcd->setMin(-static_cast<int>(rate) / 2);
  this->ui->loLcd->setMax(static_cast<int>(rate) / 2);
}

void
InspectorUI::setSampleRate(float rate)
{
  this->sampleRate = rate;
  this->ui->sampleRateLabel->setText(
        "Sample rate: "
        + QString::number(static_cast<qreal>(rate))
        + " sps");
  this->ui->bwLcd->setMin(0);
  this->ui->bwLcd->setMax(static_cast<qint64>(rate));
}

void
InspectorUI::setBandwidth(unsigned int bandwidth)
{
  // More COBOL
  this->ui->bwLcd->setValue(
        static_cast<int>(bandwidth *
              static_cast<unsigned int>(this->ui->bwLcd->getMax())
                         / this->sampleRate));
}

void
InspectorUI::setLo(int lo)
{
  this->ui->loLcd->setValue(lo);
}

void
InspectorUI::refreshInspectorCtls(void)
{
  for (auto p = this->controls.begin(); p != this->controls.end(); ++p)
    (*p)->refreshUi();
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
  unsigned int i;

  for (i = 0; i < this->palettes.size(); ++i) {
    if (this->palettes[i].getName().compare(str) == 0) {
      this->ui->wfSpectrum->setPalette(this->palettes[i].getGradient());
      this->ui->paletteCombo->setCurrentIndex(static_cast<int>(i));
      return true;
    }
  }

  return false;
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
InspectorUI::connectDataSaver()
{
  connect(
        this->dataSaver.get(),
        SIGNAL(stopped()),
        this,
        SLOT(onSaveError()));

  connect(
        this->dataSaver.get(),
        SIGNAL(swamped()),
        this,
        SLOT(onSaveSwamped()));

  connect(
        this->dataSaver.get(),
        SIGNAL(dataRate(qreal)),
        this,
        SLOT(onSaveRate(qreal)));

  connect(
        this->dataSaver.get(),
        SIGNAL(commit()),
        this,
        SLOT(onCommit()));
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
InspectorUI::installDataSaver(void)
{
  if (this->dataSaver.get() == nullptr) {
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

    this->dataSaver = std::make_unique<AsyncDataSaver>(this->fd, this);
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

  if (this->demodulating) {
    if (this->decider.getBps() > 0) {
      this->decider.feed(data, size);
      this->ui->symView->feed(this->decider.get());
      this->ui->transition->feed(this->decider.get());
      this->ui->sizeLabel->setText(
            "Size: " +
            QString::number(this->ui->symView->getLength()) +
            " symbols (" +
            QString::number(this->ui->symView->getLength() * this->decider.getBps()) +
            " bits)");

    }
  }

  if (this->recording) {
    if (this->decider.getDecisionMode() == Decider::MODULUS) {
      this->dataSaver->write(data, size);
    } else {
      if (this->buffer.size() < size)
        this->buffer.resize(size);

      for (unsigned i = 0; i < size; ++i)
        this->buffer[i] = SU_C_ARG(I * data[i]) / PI;

      this->dataSaver->write(this->buffer.data(), size);
    }
  }
}

void
InspectorUI::feedSpectrum(const SUFLOAT *data, SUSCOUNT len, SUSCOUNT rate)
{
  if (this->lastRate != rate) {
    this->ui->wfSpectrum->setSampleRate(static_cast<float>(rate));
    this->lastRate = rate;
  }

  this->ui->wfSpectrum->setNewFftData((float *) data, static_cast<int>(len));

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
InspectorUI::populate(void)
{
  int position;

  this->ui->controlsGrid->setAlignment(Qt::AlignTop);

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

  // Add dataSaver UI here. It is not exactly an inspector control, but
  // but it definitely should be placed at the end of the demodulator
  // chain.

  position = static_cast<int>(this->controls.size());
  this->saverUI = new DataSaverUI(this->owner);

  this->ui->controlsGrid->addWidget(this->saverUI, position, 0, Qt::AlignTop);

  connect(
        this->saverUI,
        SIGNAL(recordStateChanged(bool)),
        this,
        SLOT(onToggleRecord(void)));
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
  this->ui->recordButton->setEnabled(enabled);
  this->ui->loLcd->setEnabled(enabled);
  this->ui->bwLcd->setEnabled(enabled);
  this->saverUI->setEnabled(enabled && this->recordingRate != 0);
}

//////////////////////////////////// Slots ////////////////////////////////////
void
InspectorUI::setBps(unsigned int bps)
{
  if (this->bps != bps) {
    this->decider.setBps(bps);
    this->estimator.setBps(bps);
    this->ui->symView->setBitsPerSymbol(bps);
    this->ui->constellation->setOrderHint(bps);
    this->ui->transition->setOrderHint(bps);
    this->ui->histogram->setDecider(&this->decider);
    this->bps = bps; // For caching
  }
}

unsigned int
InspectorUI::getBaudRate(void) const
{
  const Suscan::FieldValue *val;
  unsigned int baud = 1;

  // Check baudrate
  if ((val = this->config->get("clock.baud")) != nullptr)
    baud = static_cast<unsigned int>(val->getFloat());

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

void
InspectorUI::onInspectorControlChanged(void)
{
  unsigned int newRate = this->getBaudRate();

  // Changing the newRate has a set of implications

  if (this->recording) {
    if (newRate == 0) {
      this->uninstallDataSaver();
      this->recording = false;
    } else if (newRate != this->recordingRate) {
      this->uninstallDataSaver();
      this->recording = this->installDataSaver();
    }

    this->saverUI->setRecordState(this->recording);
  }

  this->saverUI->setEnabled(newRate != 0);

  this->setBps(this->getBps());

  this->ui->histogram->reset();

  emit configChanged();
}

void
InspectorUI::onScrollBarChanged(int offset)
{
  this->scrolling = true;
  this->ui->symView->setOffset(
        static_cast<unsigned int>(this->ui->symView->getStride() * offset));
  this->scrolling = false;
}

void
InspectorUI::onOffsetChanged(unsigned int offset)
{
  int max = this->ui->symView->getLines() - this->ui->symView->height();

  if (max <= 0) {
    this->ui->symViewScrollBar->setPageStep(0);
    this->ui->symViewScrollBar->setMaximum(1);
  } else {
    this->ui->symViewScrollBar->setPageStep(this->ui->symView->height());
    this->ui->symViewScrollBar->setMaximum(max);

    if (!this->scrolling)
      this->ui->symViewScrollBar->setValue(static_cast<int>(offset));
  }

  this->ui->offsetSpin->setValue(static_cast<int>(offset));
}

void
InspectorUI::onStrideChanged(unsigned int stride)
{
  this->ui->widthSpin->setValue(static_cast<int>(stride));
}

void
InspectorUI::onCPUBurnClicked(void)
{
  bool burn = this->ui->burnCPUButton->isChecked();

  this->throttle.setCpuBurn(burn);
  this->ui->fpsSpin->setEnabled(!burn);
}

void
InspectorUI::onSymViewControlsChanged(void)
{
  bool autoStride = this->ui->autoFitButton->isChecked();
  bool autoScroll = this->ui->autoScrollButton->isChecked();

  this->demodulating = this->ui->recordButton->isChecked();

  this->ui->symView->setAutoStride(autoStride);
  this->ui->symView->setAutoScroll(autoScroll);
  this->ui->widthSpin->setEnabled(!autoStride);
  this->ui->offsetSpin->setEnabled(!autoScroll);

  if (!autoStride)
    this->ui->symView->setStride(
        static_cast<unsigned int>(this->ui->widthSpin->value()));

  if (!autoScroll)
    this->ui->symView->setOffset(
        static_cast<unsigned int>(this->ui->offsetSpin->value()));
}

void
InspectorUI::setColors(ColorConfig const &colors)
{
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

  this->ui->wfSpectrum->setFftPlotColor(colors.spectrumForeground);
  this->ui->wfSpectrum->setFftBgColor(colors.spectrumBackground);
  this->ui->wfSpectrum->setFftAxesColor(colors.spectrumAxes);
  this->ui->wfSpectrum->setFftTextColor(colors.spectrumText);
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
InspectorUI::onSaveSymView(void)
{
  QFileDialog dialog(this->ui->symView);
  QStringList filters;
  enum SymView::FileFormat fmt = SymView::FILE_FORMAT_TEXT;

  filters << "Text file (*.txt)"
          << "Binary file (*.bin)"
          << "C source file (*.c)";

  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setWindowTitle(QString("Save current symbol capture as..."));
  dialog.setNameFilters(filters);

  if (dialog.exec()) {
    QString filter = dialog.selectedNameFilter();
    if (strstr(filter.toStdString().c_str(), ".txt") != nullptr)
      fmt = SymView::FILE_FORMAT_TEXT;
    else if (strstr(filter.toStdString().c_str(), ".bin") != nullptr)
      fmt = SymView::FILE_FORMAT_RAW;
    else if (strstr(filter.toStdString().c_str(), ".c") != nullptr)
      fmt = SymView::FILE_FORMAT_C_ARRAY;

    try {
      this->ui->symView->save(dialog.selectedFiles().first(), fmt);
    } catch (std::ios_base::failure const &) {
      (void) QMessageBox::critical(
            this->ui->symView,
            "Save symbol file",
            "Failed to save file in the specified location. Please try again.",
            QMessageBox::Close);
    }
  }
}

void
InspectorUI::onClearSymView(void)
{
  this->ui->symView->clear();
}

void
InspectorUI::onSpectrumConfigChanged(void)
{
  int index = this->ui->paletteCombo->currentIndex();
  this->ui->wfSpectrum->setPalette(
        this->palettes[static_cast<unsigned>(index)].getGradient());
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
  this->ui->wfSpectrum->setPandapterRange(
        this->ui->rangeSlider->minimumValue(),
        this->ui->rangeSlider->maximumValue());

  this->ui->wfSpectrum->setWaterfallRange(
        this->ui->rangeSlider->minimumValue(),
        this->ui->rangeSlider->maximumValue());
}

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
  if (this->dataSaver.get() != nullptr) {
    this->uninstallDataSaver();

    QMessageBox::warning(
              this->owner,
              "SigDigger error",
              "Capture file write error. Disk full?",
              QMessageBox::Ok);

    this->saverUI->setRecordState(false);
  }
}

void
InspectorUI::onSaveSwamped(void)
{
  if (this->dataSaver.get() != nullptr) {
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

