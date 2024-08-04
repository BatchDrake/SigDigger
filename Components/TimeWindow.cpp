//
//    TimeWindow.cpp: Time Window for time view operations
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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

#include <TimeWindow.h>
#include <QFileDialog>
#include <QMessageBox>
#include <Suscan/Library.h>
#include <sigutils/sampling.h>
#include <fstream>
#include <iomanip>
#include <SuWidgetsHelpers.h>
#include <SigDiggerHelpers.h>
#include <climits>
#include <HistogramFeeder.h>

#include <DopplerCalculator.h>
#include <CarrierDetector.h>
#include <CarrierXlator.h>
#include <CostasRecoveryTask.h>
#include <PLLSyncTask.h>
#include <QuadDemodTask.h>
#include <AGCTask.h>
#include <DelayedConjTask.h>
#include <LPFTask.h>

#include "ui_TimeWindow.h"

using namespace SigDigger;

void
TimeWindow::connectFineTuneSelWidgets()
{
  connect(
        ui->selStartDecDeltaTButton,
        SIGNAL(clicked()),
        this,
        SLOT(onFineTuneSelectionClicked()));

  connect(
        ui->selStartDecSampleButton,
        SIGNAL(clicked()),
        this,
        SLOT(onFineTuneSelectionClicked()));

  connect(
        ui->selStartIncDeltaTButton,
        SIGNAL(clicked()),
        this,
        SLOT(onFineTuneSelectionClicked()));

  connect(
        ui->selStartIncSampleButton,
        SIGNAL(clicked()),
        this,
        SLOT(onFineTuneSelectionClicked()));

  connect(
        ui->selEndDecDeltaTButton,
        SIGNAL(clicked()),
        this,
        SLOT(onFineTuneSelectionClicked()));

  connect(
        ui->selEndDecSampleButton,
        SIGNAL(clicked()),
        this,
        SLOT(onFineTuneSelectionClicked()));

  connect(
        ui->selEndIncDeltaTButton,
        SIGNAL(clicked()),
        this,
        SLOT(onFineTuneSelectionClicked()));

  connect(
        ui->selEndIncSampleButton,
        SIGNAL(clicked()),
        this,
        SLOT(onFineTuneSelectionClicked()));
}

void
TimeWindow::connectTransformWidgets()
{
  connect(
        ui->lpfApplyButton,
        SIGNAL(clicked()),
        this,
        SLOT(onLPF()));

  connect(
        ui->costasSyncButton,
        SIGNAL(clicked()),
        this,
        SLOT(onCostasRecovery()));

  connect(
        ui->pllSyncButton,
        SIGNAL(clicked()),
        this,
        SLOT(onPLLRecovery()));

  connect(
        ui->cycloButton,
        SIGNAL(clicked()),
        this,
        SLOT(onCycloAnalysis()));

  connect(
        ui->agcButton,
        SIGNAL(clicked()),
        this,
        SLOT(onAGC()));

  connect(
        ui->dcmButton,
        SIGNAL(clicked()),
        this,
        SLOT(onDelayedConjugate()));

  connect(
        ui->quadDemodButton,
        SIGNAL(clicked()),
        this,
        SLOT(onQuadDemod()));

  connect(
        ui->agcRateSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onAGCRateChanged()));

  connect(
        ui->dcmRateSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onDelayedConjChanged()));

  connect(
        ui->resetTransformButton,
        SIGNAL(clicked()),
        this,
        SLOT(onResetCarrier()));
}

void
TimeWindow::connectAll()
{
  connectTransformWidgets();

  connect(
        ui->realWaveform,
        SIGNAL(horizontalRangeChanged(qint64, qint64)),
        this,
        SLOT(onHZoom(qint64, qint64)));

  connect(
        ui->realWaveform,
        SIGNAL(horizontalSelectionChanged(qreal, qreal)),
        this,
        SLOT(onHSelection(qreal, qreal)));

  connect(
        ui->imagWaveform,
        SIGNAL(horizontalRangeChanged(qint64, qint64)),
        this,
        SLOT(onHZoom(qint64, qint64)));

  connect(
        ui->imagWaveform,
        SIGNAL(horizontalSelectionChanged(qreal, qreal)),
        this,
        SLOT(onHSelection(qreal, qreal)));

  connect(
        ui->realWaveform,
        SIGNAL(hoverTime(qreal)),
        this,
        SLOT(onHoverTime(qreal)));

  connect(
        ui->imagWaveform,
        SIGNAL(hoverTime(qreal)),
        this,
        SLOT(onHoverTime(qreal)));

  connect(
        ui->actionSave,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onSaveAll()));

  connect(
        ui->actionSave_selection,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onSaveSelection()));

  connect(
        ui->actionAutoFit,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleAutoFit()));

#if 0
  connect(
        ui->actionHorizontal_selection,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onToggleHorizontalSelection()));

  connect(
        ui->actionVertical_selection,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onToggleVerticalSelection()));
#endif

  connect(
        ui->actionZoom_selection,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onZoomToSelection()));

  connect(
        ui->actionResetZoom,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onZoomReset()));

  connect(
        ui->actionShowWaveform,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onShowWaveform()));

  connect(
        ui->actionShowEnvelope,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onShowEnvelope()));

  connect(
        ui->actionShowPhase,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onShowPhase()));

  connect(
        ui->actionPhaseDerivative,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onPhaseDerivative()));

  connect(
        ui->periodicSelectionCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onTogglePeriodicSelection()));

  connect(
        ui->periodicDivisionsSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onPeriodicDivisionsChanged()));

  connect(
        ui->paletteCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onPaletteChanged(int)));

  connect(
        ui->offsetSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onChangePaletteOffset(int)));

  connect(
        ui->contrastSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onChangePaletteContrast(int)));

  connect(
        ui->taskAbortButton,
        SIGNAL(clicked()),
        this,
        SLOT(onAbort()));

  connect(
        &m_taskController,
        SIGNAL(cancelling()),
        this,
        SLOT(onTaskCancelling()));

  connect(
        &m_taskController,
        SIGNAL(progress(qreal, QString)),
        this,
        SLOT(onTaskProgress(qreal, QString)));

  connect(
        &m_taskController,
        SIGNAL(done()),
        this,
        SLOT(onTaskDone()));

  connect(
        &m_taskController,
        SIGNAL(cancelled()),
        this,
        SLOT(onTaskCancelled()));

  connect(
        &m_taskController,
        SIGNAL(error(QString)),
        this,
        SLOT(onTaskError(QString)));

  connect(
        ui->guessCarrierButton,
        SIGNAL(clicked()),
        this,
        SLOT(onGuessCarrier()));

  connect(
        ui->syncButton,
        SIGNAL(clicked()),
        this,
        SLOT(onSyncCarrier()));

  connect(
        ui->resetButton,
        SIGNAL(clicked()),
        this,
        SLOT(onResetCarrier()));


  connect(
        ui->dcNotchSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onCarrierSlidersChanged()));

  connect(
        ui->averagerSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onCarrierSlidersChanged()));

  connect(
        ui->showHistogramButton,
        SIGNAL(clicked()),
        this,
        SLOT(onTriggerHistogram()));

  connect(
        m_histogramDialog,
        SIGNAL(blanked()),
        this,
        SLOT(onHistogramBlanked()));

  connect(
        ui->startSamplinButton,
        SIGNAL(clicked()),
        this,
        SLOT(onTriggerSampler()));

  connect(
        m_samplerDialog,
        SIGNAL(resample()),
        this,
        SLOT(onResample()));

  connect(
        m_samplerDialog,
        SIGNAL(stopTask()),
        this,
        SLOT(onAbort()));

  connect(
        m_histogramDialog,
        SIGNAL(stopTask()),
        this,
        SLOT(onAbort()));

  connect(
        ui->clckSourceBtnGrp,
        SIGNAL(buttonClicked(QAbstractButton *)),
        this,
        SLOT(onClkSourceButtonClicked()));

  connect(
        ui->dopplerButton,
        SIGNAL(clicked()),
        this,
        SLOT(onCalculateDoppler()));

  connect(
        ui->realWaveform,
        SIGNAL(waveViewChanged()),
        this,
        SLOT(onWaveViewChanged()));

  connect(
        ui->zeroPointSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onZeroPointChanged()));

  connect(
        ui->clkComponentCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onZeroCrossingComponentChanged()));

  connect(
        ui->spaceButtonGrp,
        SIGNAL(buttonClicked(QAbstractButton *)),
        this,
        SLOT(onZeroPointChanged()));

  connectFineTuneSelWidgets();
}

int
TimeWindow::getPeriodicDivision() const
{
  return ui->periodicDivisionsSpin->value();
}

const SUCOMPLEX *
TimeWindow::getDisplayData() const
{
  return m_displayDataPtr;
}

size_t
TimeWindow::getDisplayDataLength() const
{
  return m_displayDataLength;
}

void
TimeWindow::showEvent(QShowEvent *)
{
  if (m_firstShow) {
    ui->dockWidget->setMinimumWidth(
          ui->measurementsGrid->sizeHint().width()
            + TIME_WINDOW_EXTRA_WIDTH);
    m_firstShow = false;
  }
}


void
TimeWindow::setPalette(std::string const &name)
{
  int index = SigDiggerHelpers::instance()->getPaletteIndex(name);

  if (index >= 0) {
    ui->paletteCombo->setCurrentIndex(index);
    onPaletteChanged(index);
  }
}

void
TimeWindow::setPaletteOffset(unsigned int offset)
{
  if (offset > 255)
    offset = 255;
  ui->offsetSlider->setValue(static_cast<int>(offset));
  onChangePaletteOffset(static_cast<int>(offset));
}

void
TimeWindow::setPaletteContrast(int contrast)
{
  ui->contrastSlider->setValue(contrast);
  onChangePaletteContrast(contrast);
}

void
TimeWindow::setColorConfig(ColorConfig const &cfg)
{
  ui->constellation->setBackgroundColor(cfg.constellationBackground);
  ui->constellation->setForegroundColor(cfg.constellationForeground);
  ui->constellation->setAxesColor(cfg.constellationAxes);

  ui->realWaveform->setBackgroundColor(cfg.spectrumBackground);
  ui->realWaveform->setForegroundColor(cfg.spectrumForeground);
  ui->realWaveform->setAxesColor(cfg.spectrumAxes);
  ui->realWaveform->setTextColor(cfg.spectrumText);
  ui->realWaveform->setSelectionColor(cfg.selection);

  ui->imagWaveform->setBackgroundColor(cfg.spectrumBackground);
  ui->imagWaveform->setForegroundColor(cfg.spectrumForeground);
  ui->imagWaveform->setAxesColor(cfg.spectrumAxes);
  ui->imagWaveform->setTextColor(cfg.spectrumText);
  ui->imagWaveform->setSelectionColor(cfg.selection);

  m_histogramDialog->setColorConfig(cfg);
  m_samplerDialog->setColorConfig(cfg);
  m_dopplerDialog->setColorConfig(cfg);
}

std::string
TimeWindow::getPalette() const
{
  const Palette *palette = SigDiggerHelpers::instance()->getPalette(
        ui->paletteCombo->currentIndex());

  if (palette == nullptr)
    return "Suscan";

  return palette->getName();
}

unsigned int
TimeWindow::getPaletteOffset() const
{
  return static_cast<unsigned>(ui->offsetSlider->value());
}

int
TimeWindow::getPaletteContrast() const
{
  return ui->contrastSlider->value();
}

void
TimeWindow::fineTuneSelSetEnabled(bool enabled)
{
  ui->selStartButtonsWidget->setEnabled(enabled);
  ui->selEndButtonsWidget->setEnabled(enabled);
  ui->lockButton->setEnabled(enabled);
}

void
TimeWindow::fineTuneSelNotifySelection(bool sel)
{
  fineTuneSelSetEnabled(sel);
}

void
TimeWindow::carrierSyncSetEnabled(bool enabled)
{
  ui->carrierSyncPage->setEnabled(enabled);
}

void
TimeWindow::carrierSyncNotifySelection(bool selection)
{
  ui->guessCarrierButton->setEnabled(selection);
}

void
TimeWindow::samplingSetEnabled(bool enabled)
{
  ui->samplingPage->setEnabled(enabled);
}

const SUCOMPLEX *
TimeWindow::getData() const
{
  return m_data == nullptr ? m_roDataPtr : m_data->data();
}

size_t
TimeWindow::getLength() const
{
  return m_data == nullptr ? m_roDataLength : m_data->size();
}

void
TimeWindow::getTransformRegion(
    const SUCOMPLEX * &origin,
    SUCOMPLEX *&destination,
    SUSCOUNT &length,
    bool selection)
{
  const SUCOMPLEX *data = getDisplayData();
  SUCOMPLEX *dest;
  length = 0;

  m_processedData.resize(getDisplayDataLength());
  dest = m_processedData.data();

  if (data == getData())
    memcpy(dest, data, getDisplayDataLength() * sizeof(SUCOMPLEX));

  if (selection && ui->realWaveform->getHorizontalSelectionPresent()) {
    qint64 selStart = static_cast<qint64>(
          ui->realWaveform->getHorizontalSelectionStart());
    qint64 selEnd = static_cast<qint64>(
          ui->realWaveform->getHorizontalSelectionEnd());

    origin      = data + selStart;
    destination = dest + selStart;
    length      = selEnd - selStart;
  }

  if (length == 0) {
    origin = data;
    destination = dest;
    length = getDisplayDataLength();
  }
}

void
TimeWindow::populateSamplingProperties(SamplingProperties &prop)
{
  bool haveSelection = ui->realWaveform->getHorizontalSelectionPresent();
  bool intSelection =
      haveSelection && ui->intSelectionButton->isChecked();
  qreal seconds;

  prop.fs = m_fs;
  prop.loopGain = 0;

  if (ui->clkGardnerButton->isChecked())
    prop.sync = SamplingClockSync::GARDNER;
  else if (ui->clkZcButton->isChecked())
    prop.sync = SamplingClockSync::ZERO_CROSSING;
  else
    prop.sync = SamplingClockSync::MANUAL;

  if (ui->decAmplitudeButton->isChecked()) {
    prop.space = SamplingSpace::AMPLITUDE;
    prop.zeroCrossingAngle = ui->clkComponentCombo->currentIndex() == 0
        ? 1
        : -SU_I;
  } else if (ui->decPhaseButton->isChecked()) {
    prop.space = SamplingSpace::PHASE;
  } else if (ui->decFrequencyButton->isChecked()) {
    prop.space = SamplingSpace::FREQUENCY;
  }

  if (intSelection) {
    size_t start = static_cast<size_t>(
          ui->realWaveform->getHorizontalSelectionStart());
    prop.data = getDisplayData() + start;
    prop.length =
        static_cast<size_t>(
          ui->realWaveform->getHorizontalSelectionEnd()
          - ui->realWaveform->getHorizontalSelectionStart());
    prop.symbolSync = start;
  } else {
    prop.data = getDisplayData();
    prop.length = getDisplayDataLength();
    prop.symbolSync = 0;
  }

  seconds = prop.length / m_fs;

  if (haveSelection && ui->clkSelectionButton->isChecked()) {
    if (intSelection) {
      // Interval is selection. Select all subdivisions
      prop.symbolCount = ui->periodicDivisionsSpin->value();
      prop.rate        = prop.symbolCount / seconds;
    } else {
      qreal selLength =
            ui->realWaveform->getHorizontalSelectionEnd()
            - ui->realWaveform->getHorizontalSelectionStart();

      // Compute deltaT based on selection and then the number of symbols
      // in the defined interval.
      qreal deltaT     = selLength / ui->periodicDivisionsSpin->value();
      prop.rate        = 1 / deltaT;
      prop.symbolCount = prop.length / deltaT;
    }
  } else if (ui->clkManualButton->isChecked()) {
    prop.rate        = ui->baudSpin->value();
    prop.symbolCount = seconds * prop.rate;
  } else if (ui->clkPartitionButton->isChecked()) {
    prop.symbolCount = ui->numSymSpin->value();
    prop.rate        = prop.symbolCount / seconds;
  } else {
    prop.rate        = ui->baudSpin->value();
    prop.loopGain       =
        static_cast<qreal>(
          SU_MAG_RAW(
            static_cast<SUFLOAT>(ui->clkGardnerLoopGain->value())));
  }
}

void
TimeWindow::samplingNotifySelection(bool selection, bool periodic)
{
  ui->intSelectionButton->setEnabled(selection);
  ui->clkSelectionButton->setEnabled(selection);

  if (!selection) {
    if (ui->intSelectionButton->isChecked())
      ui->intFullButton->setChecked(true);

    if (ui->clkSelectionButton->isChecked())
      ui->clkManualButton->setChecked(true);
  } else {
    ui->intSelectionButton->setChecked(true);

    if (periodic)
      ui->clkSelectionButton->setChecked(true);
  }
}

void
TimeWindow::notifyTaskRunning(bool running)
{
  m_taskRunning = running;

  ui->taskAbortButton->setEnabled(running);
  carrierSyncSetEnabled(!running);
  samplingSetEnabled(!running);

  ui->lpfApplyButton->setEnabled(!running);
  ui->agcButton->setEnabled(!running);
  ui->dcmButton->setEnabled(!running);
  ui->cycloButton->setEnabled(!running);
  ui->quadDemodButton->setEnabled(!running);
  ui->resetTransformButton->setEnabled(!running);
  ui->resetButton->setEnabled(!running);
  ui->costasSyncButton->setEnabled(!running);
  ui->pllSyncButton->setEnabled(!running);
}

void
TimeWindow::refreshUi()
{
  bool haveSelection = ui->realWaveform->getHorizontalSelectionPresent();
  bool baudEditable;

  ui->periodicDivisionsSpin->setEnabled(
        ui->periodicSelectionCheck->isChecked());
  ui->selStartLabel->setEnabled(haveSelection);
  ui->selEndLabel->setEnabled(haveSelection);
  ui->selLengthLabel->setEnabled(haveSelection);
  ui->periodLabel->setEnabled(haveSelection);
  ui->baudLabel->setEnabled(haveSelection);
  ui->actionSave_selection->setEnabled(haveSelection);
  ui->dopplerButton->setEnabled(haveSelection);

  if (haveSelection != m_hadSelectionBefore) {
    carrierSyncNotifySelection(haveSelection);
    fineTuneSelNotifySelection(haveSelection);
    samplingNotifySelection(
          haveSelection,
          ui->periodicSelectionCheck->isChecked());
  }

  ui->sampleRateLabel->setText(
        QString::number(static_cast<int>(
          ui->realWaveform->getSampleRate())) +
        QStringLiteral(" sp/s"));

  baudEditable = ui->clkManualButton->isChecked()
      || ui->clkGardnerButton->isChecked()
      || ui->clkZcButton->isChecked();

  ui->clkRateFrame->setEnabled(baudEditable);
  ui->clkPartitionFrame->setEnabled(
        ui->clkPartitionButton->isChecked());
  ui->clkGardnerFrame->setEnabled(
        ui->clkGardnerButton->isChecked());
  ui->clkZcFrame->setEnabled(
        ui->clkZcButton->isChecked());
  ui->clkComponentCombo->setEnabled(
        ui->decAmplitudeButton->isChecked());

  SamplingProperties sp;
  populateSamplingProperties(sp);
  qreal baud = (sp.symbolCount * m_fs) / sp.length;

  if (ui->clkSelectionButton->isChecked()
      || ui->clkPartitionButton->isChecked())
    ui->baudSpin->setValue(baud);

  onZeroPointChanged();
  onZeroCrossingComponentChanged();

  m_hadSelectionBefore = haveSelection;
}

void
TimeWindow::startSampling()
{
  WaveSampler *ws = m_samplerDialog->makeSampler();

  connect(
        ws,
        SIGNAL(data(SigDigger::WaveSampleSet)),
        this,
        SLOT(onSampleSet(SigDigger::WaveSampleSet)));

  notifyTaskRunning(true);
  m_taskController.process(QStringLiteral("triggerSampler"), ws);
}

void
TimeWindow::refreshMeasures()
{
  qreal selStart = 0;
  qreal selEnd   = 0;
  qreal deltaT = 1. / ui->realWaveform->getSampleRate();
  SUCOMPLEX min, max, mean;
  SUFLOAT rms;
  int length = static_cast<int>(getDisplayDataLength());
  bool selection = false;

  if (ui->realWaveform->getHorizontalSelectionPresent()) {
    selStart = ui->realWaveform->getHorizontalSelectionStart();
    selEnd   = ui->realWaveform->getHorizontalSelectionEnd();

    if (selStart < 0)
      selStart = 0;
    if (selEnd > length)
      selEnd = length;
  }

  if (selEnd - selStart > 0 && ui->realWaveform->isComplete()) {
    WaveLimits limits;

    qreal period =
        (selEnd - selStart) /
        (ui->periodicSelectionCheck->isChecked()
           ? getPeriodicDivision()
           : 1)
        * deltaT;
    qreal baud = 1 / period;

#if 0
    const SUCOMPLEX *data = getDisplayData();

    SuWidgetsHelpers::kahanMeanAndRms(
          &mean,
          &rms,
          data + static_cast<qint64>(selStart),
          SCAST(SUSCOUNT, selEnd - selStart));
    SuWidgetsHelpers::calcLimits(
          &min,
          &max,
          data + static_cast<qint64>(selStart),
          SCAST(SUSCOUNT, selEnd - selStart));
#else
    ui->realWaveform->computeLimits(selStart, selEnd, limits);

    mean = limits.mean;
    min  = limits.min;
    max  = limits.max;
    rms  = limits.envelope;

    selection = true;

#endif
    ui->periodLabel->setText(
          SuWidgetsHelpers::formatQuantityFromDelta(
            period,
            deltaT,
            "s"));
    ui->baudLabel->setText(
      SuWidgetsHelpers::formatQuantity(
        baud,
        4,
        "Hz"));
    ui->selStartLabel->setText(
          SuWidgetsHelpers::formatQuantityFromDelta(
            ui->realWaveform->samp2t(selStart),
            deltaT,
            "s",
            true)
          + " (" + SuWidgetsHelpers::formatReal(selStart) + ")");
    ui->selEndLabel->setText(
          SuWidgetsHelpers::formatQuantityFromDelta(
            ui->realWaveform->samp2t(selEnd),
            deltaT,
            "s",
            true)
          + " (" + SuWidgetsHelpers::formatReal(selEnd) + ")");
    ui->selLengthLabel->setText(
          SuWidgetsHelpers::formatQuantityFromDelta(
            (selEnd - selStart) * deltaT,
            deltaT,
            "s")
          + " (" + SuWidgetsHelpers::formatReal(selEnd - selStart) + ")");
  } else {
    min  = ui->realWaveform->getDataMin();
    max  = ui->realWaveform->getDataMax();
    mean = ui->realWaveform->getDataMean();
    rms  = ui->realWaveform->getDataRMS();

    ui->periodLabel->setText("N/A");
    ui->baudLabel->setText("N/A");
    ui->selStartLabel->setText("N/A");
    ui->selEndLabel->setText("N/A");
    ui->selLengthLabel->setText("N/A");
  }

  ui->lengthLabel->setText(QString::number(length) + " samples");

  ui->durationLabel->setText(
        SuWidgetsHelpers::formatQuantityFromDelta(
          length * deltaT,
          deltaT,
          "s"));

  ui->minILabel->setText(
        SuWidgetsHelpers::formatScientific(SU_C_REAL(min)));

  ui->maxILabel->setText(
        SuWidgetsHelpers::formatScientific(SU_C_REAL(max)));

  ui->meanILabel->setText(
        SuWidgetsHelpers::formatScientific(SU_C_REAL(mean)));

  ui->minQLabel->setText(
        SuWidgetsHelpers::formatScientific(SU_C_IMAG(min)));

  ui->maxQLabel->setText(
        SuWidgetsHelpers::formatScientific(SU_C_IMAG(max)));

  ui->meanQLabel->setText(
        SuWidgetsHelpers::formatScientific(SU_C_IMAG(mean)));

  ui->rmsLabel->setText(
        (selection ? "< " : "") +
        SuWidgetsHelpers::formatReal(rms));
}

void
TimeWindow::setCenterFreq(SUFREQ center)
{
  m_centerFreq = center;
  ui->centerFreqLabel->setText(
        SuWidgetsHelpers::formatIntegerPart(center) + " Hz");

  ui->refFreqSpin->setValue(center);
}

void
TimeWindow::refresh()
{
  ui->realWaveform->setData(m_displayDataPtr, m_displayDataLength, true, true, true);
  ui->imagWaveform->setData(m_displayDataPtr, m_displayDataLength, true, true, true);
}

void
TimeWindow::setDisplayData(
    const SUCOMPLEX *displayData,
    size_t displayLen,
    bool keepView)
{
  QCursor cursor = this->cursor();

  m_displayDataPtr    = displayData;
  m_displayDataLength = displayLen;

  // This is just a workaround. TODO: fix build method in Waveformview
  setCursor(Qt::WaitCursor);

  if (displayLen == 0) {
    ui->realWaveform->setData(nullptr, false);
    ui->imagWaveform->setData(nullptr, false);
  } else {
    qint64 currStart = ui->realWaveform->getSampleStart();
    qint64 currEnd   = ui->realWaveform->getSampleEnd();

    ui->realWaveform->setData(displayData, displayLen, keepView, true);
    ui->imagWaveform->setData(displayData, displayLen, keepView, true);

    if (currStart != currEnd) {
      ui->realWaveform->zoomHorizontal(currStart, currEnd);
      ui->imagWaveform->zoomHorizontal(currStart, currEnd);
    }
  }

  setCursor(Qt::ArrowCursor);

  refreshUi();
  refreshMeasures();
  setCursor(cursor);
}

void
TimeWindow::setData(const SUCOMPLEX *data, size_t size, qreal fs, qreal bw)
{
  if (m_fs != fs) {
    m_fs = fs;
    ui->costasBwSpin->setValue(m_fs / 200);
    ui->pllCutOffSpin->setValue(m_fs / 200);
  }

  if (!sufeq(m_bw, bw, 1e-6)) {
    m_bw = bw;
    ui->costasArmBwSpin->setValue(bw / 100);
  }

  ui->syncFreqSpin->setMinimum(-m_fs / 2);
  ui->syncFreqSpin->setMaximum(m_fs / 2);
  ui->realWaveform->setSampleRate(fs);
  ui->imagWaveform->setSampleRate(fs);

  m_roDataPtr    = data;
  m_roDataLength = size;

  setDisplayData(data, size);
  onCarrierSlidersChanged();
}

void
TimeWindow::setData(std::vector<SUCOMPLEX> const &data, qreal fs, qreal bw)
{
  setData(data.data(), data.size(), fs, bw);
}

void
TimeWindow::adjustButtonToSize(QPushButton *button, QString text)
{
  if (text.size() == 0)
    text = button->text();

  button->setMaximumWidth(
        SuWidgetsHelpers::getWidgetTextWidth(button, text) +
        5 * SuWidgetsHelpers::getWidgetTextWidth(button, " "));
}

void
TimeWindow::closeEvent(QCloseEvent *)
{
  emit closed();
}

TimeWindow::TimeWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::TimeWindow)
{
  ui->setupUi(this);

  m_histogramDialog = new HistogramDialog(this);
  m_samplerDialog   = new SamplerDialog(this);
  m_dopplerDialog   = new DopplerDialog(this);

  // We can do this because both labels have the same font
  ui->notchWidthLabel->setFixedWidth(
        SuWidgetsHelpers::getWidgetTextWidth(
          ui->notchWidthLabel,
          QStringLiteral("XXXX.XX XHz")));

  ui->averagerSpanLabel->setFixedWidth(
        SuWidgetsHelpers::getWidgetTextWidth(
          ui->averagerSpanLabel,
          QStringLiteral("XXXX.XX XHz")));

  ui->realWaveform->setRealComponent(true);
  ui->imagWaveform->setRealComponent(false);

  ui->imagWaveform->reuseDisplayData(ui->realWaveform);

  ui->syncFreqSpin->setExtraDecimals(6);

#ifdef __APPLE__
  QFontMetrics m(ui->selStartButtonsWidget->font());
  // Fix Qt limitations in MacOS
  ui->selStartButtonsWidget->setMaximumHeight(7 * m.height() / 4);
  ui->selEndButtonsWidget->setMaximumHeight(7 * m.height() / 4);

  adjustButtonToSize(ui->selStartDecDeltaTButton, ">>");
  adjustButtonToSize(ui->selStartDecSampleButton, ">>");
  adjustButtonToSize(ui->selStartIncSampleButton, ">>");
  adjustButtonToSize(ui->selStartIncDeltaTButton, ">>");

  adjustButtonToSize(ui->selEndDecDeltaTButton, ">>");
  adjustButtonToSize(ui->selEndDecSampleButton, ">>");
  adjustButtonToSize(ui->selEndIncSampleButton, ">>");
  adjustButtonToSize(ui->selEndIncDeltaTButton, ">>");

  ui->gridLayout_9->setVerticalSpacing(6);
  ui->gridLayout_11->setVerticalSpacing(6);
  ui->gridLayout_12->setVerticalSpacing(6);
#endif

  refreshUi();
  refreshMeasures();
  SigDiggerHelpers::instance()->populatePaletteCombo(ui->paletteCombo);

  ui->toolBox->setCurrentIndex(0);

  onToggleAutoFit();

  connectAll();
}

void
TimeWindow::postLoadInit()
{
  SigDiggerHelpers::instance()->populatePaletteCombo(ui->paletteCombo);
}

TimeWindow::~TimeWindow()
{
  delete ui;
}

//////////////////////////////////// Slots /////////////////////////////////////
void
TimeWindow::onHZoom(qint64 min, qint64 max)
{
  QObject* obj = sender();

  if (!m_adjusting) {
    Waveform *wf = nullptr;
    m_adjusting = true;

    if (obj == ui->realWaveform)
      wf = ui->imagWaveform;
    else
      wf = ui->realWaveform;

    wf->zoomHorizontal(min, max);
    wf->invalidate();
    m_adjusting = false;
  }
}

void
TimeWindow::onVZoom(qreal, qreal)
{
  // QObject* obj = sender();
}

void
TimeWindow::onHSelection(qreal min, qreal max)
{
  QObject *obj = sender();

  if (!m_adjusting) {
    Waveform *curr = static_cast<Waveform *>(obj);
    Waveform *wf;
    m_adjusting = true;

    if (obj == ui->realWaveform)
      wf = ui->imagWaveform;
    else
      wf = ui->realWaveform;

    if (curr->getHorizontalSelectionPresent())
      wf->selectHorizontal(min, max);
    else
      wf->selectHorizontal(0, 0);

    refreshUi();
    refreshMeasures();
    wf->invalidate();

    m_adjusting = false;
  }
}

void
TimeWindow::onVSelection(qreal, qreal)
{
  // QObject* obj = sender();
}

void
TimeWindow::onHoverTime(qreal time)
{
  const SUCOMPLEX *data = getDisplayData();
  qint64 length = static_cast<qint64>(getDisplayDataLength());
  qreal samp = ui->realWaveform->t2samp(time);
  qint64 iSamp = static_cast<qint64>(std::floor(samp));
  qint64 selStart = 0, selEnd = 0, selLen = 0;
  qreal max = std::max<qreal>(
        std::max<qreal>(
          std::fabs(ui->realWaveform->getMax()),
          std::fabs(ui->realWaveform->getMin())),
        std::max<qreal>(
          std::fabs(ui->imagWaveform->getMax()),
          std::fabs(ui->imagWaveform->getMin())));

  qreal ampl = 1;
  if (max > 0)
    ampl = 1. / max;

  if (iSamp < 0)
    samp = iSamp = 0;
  if (iSamp > length)
    samp = iSamp = length - 1;

  SUFLOAT t = static_cast<SUFLOAT>(samp - iSamp);
  SUCOMPLEX val = (1 - t) * data[iSamp] + t * data[iSamp + 1];

  ui->constellation->setGain(ampl);

  if (ui->realWaveform->getHorizontalSelectionPresent()) {
     selStart = static_cast<qint64>(
          ui->realWaveform->getHorizontalSelectionStart());
    selEnd = static_cast<qint64>(
          ui->realWaveform->getHorizontalSelectionEnd());

    if (selStart < 0)
      selStart = 0;
    if (selEnd > length)
      selEnd = length;

    if (selEnd - selStart > TIME_WINDOW_MAX_SELECTION)
      selStart = selEnd - TIME_WINDOW_MAX_SELECTION;

    selLen = selEnd - selStart;

    if (selLen > 0) {
      ui->constellation->setHistorySize(
            static_cast<unsigned int>(selLen));
      ui->constellation->feed(
            data + selStart,
            static_cast<unsigned int>(selLen));
    }
  } else {
    if (iSamp == length - 1) {
      ui->constellation->setHistorySize(1);
      ui->constellation->feed(data + iSamp, 1);
    } else if (iSamp >= 0 && iSamp < length - 1) {
      ui->constellation->setHistorySize(1);
      ui->constellation->feed(&val, 1);
    } else {
      ui->constellation->setHistorySize(0);
    }
  }

  ui->positionLabel->setText(
        SuWidgetsHelpers::formatQuantityFromDelta(
          time,
          1 / m_fs,
          "s",
          true)
        + " (" + SuWidgetsHelpers::formatReal(samp) + ")");
  ui->iLabel->setText(SuWidgetsHelpers::formatScientific(SU_C_REAL(val)));
  ui->qLabel->setText(SuWidgetsHelpers::formatScientific(SU_C_IMAG(val)));
  ui->magPhaseLabel->setText(
        SuWidgetsHelpers::formatReal(SU_C_ABS(val))
        + "("
        + SuWidgetsHelpers::formatReal(SU_C_ARG(val) / M_PI * 180)
        + "º)");

  // Frequency calculations
  qint64 dopplerLen = ui->realWaveform->getHorizontalSelectionPresent()
      ? selLen
      : static_cast<qint64>(
          std::ceil(ui->realWaveform->getSamplesPerPixel()));
  qint64 dopplerStart = ui->realWaveform->getHorizontalSelectionPresent()
      ? selStart
      : iSamp;
  qint64 delta = 1;
  qreal omegaAccum = 0;
  unsigned int count = 0;

  if (dopplerLen > TIME_WINDOW_MAX_DOPPLER_ITERS) {
    delta = dopplerLen / TIME_WINDOW_MAX_DOPPLER_ITERS;
    dopplerLen = TIME_WINDOW_MAX_DOPPLER_ITERS;
  }

  for (auto i = dopplerStart; i < dopplerLen + dopplerStart; i += delta) {
    if (i >= 1 && i < length) {
      omegaAccum += SU_C_ARG(data[i] * SU_C_CONJ(data[i - 1]));
      ++count;
    }
  }

  if (count > 0) {
    SUFLOAT normFreq;
    normFreq = SU_ANG2NORM_FREQ(omegaAccum / count);
    SUFREQ freq = SU_NORM2ABS_FREQ(m_fs, normFreq);
    SUFREQ ifFreq = ui->refFreqSpin->value() - m_centerFreq;
    SUFREQ doppler = -TIME_WINDOW_SPEED_OF_LIGHT / m_centerFreq * (freq - ifFreq);
    ui->freqShiftLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            freq,
            6,
            QStringLiteral("Hz"),
            true));
    ui->dopplerShiftLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            doppler,
            5,
            QStringLiteral("m/s"),
            true));
  } else {
    ui->freqShiftLabel->setText(QStringLiteral("N/A"));
    ui->dopplerShiftLabel->setText(QStringLiteral("N/A"));
  }
}

void
TimeWindow::onTogglePeriodicSelection()
{
  ui->realWaveform->setPeriodicSelection(
        ui->periodicSelectionCheck->isChecked());
  ui->imagWaveform->setPeriodicSelection(
        ui->periodicSelectionCheck->isChecked());

  ui->realWaveform->invalidate();
  ui->imagWaveform->invalidate();

  refreshUi();
}

void
TimeWindow::onPeriodicDivisionsChanged()
{
  ui->realWaveform->setDivsPerSelection(
        getPeriodicDivision());
  ui->imagWaveform->setDivsPerSelection(
        getPeriodicDivision());

  ui->realWaveform->invalidate();
  ui->imagWaveform->invalidate();

  refreshMeasures();
}


void
TimeWindow::onSaveAll()
{
  SigDiggerHelpers::openSaveSamplesDialog(
        this,
        getDisplayData(),
        getDisplayDataLength(),
        m_fs,
        0,
        static_cast<int>(getDisplayDataLength()),
        Suscan::Singleton::get_instance()->getBackgroundTaskController());
}

void
TimeWindow::onSaveSelection()
{
  SigDiggerHelpers::openSaveSamplesDialog(
        this,
        getDisplayData(),
        getDisplayDataLength(),
        m_fs,
        static_cast<int>(ui->realWaveform->getHorizontalSelectionStart()),
        static_cast<int>(ui->realWaveform->getHorizontalSelectionEnd()),
        Suscan::Singleton::get_instance()->getBackgroundTaskController());
}

void
TimeWindow::onFit()
{
  if (ui->realWaveform->isComplete()) {
    ui->realWaveform->fitToEnvelope();
    ui->imagWaveform->fitToEnvelope();

    ui->realWaveform->zoomHorizontalReset();
    ui->imagWaveform->zoomHorizontalReset();

    ui->realWaveform->invalidate();
    ui->imagWaveform->invalidate();
  }
}

void
TimeWindow::onToggleAutoFit()
{
  bool doAutoFit = ui->actionAutoFit->isChecked();

  ui->realWaveform->setAutoFitToEnvelope(doAutoFit);
  ui->imagWaveform->setAutoFitToEnvelope(doAutoFit);

  if (doAutoFit)
    onFit();
}

#if 0
void
TimeWindow::onToggleHorizontalSelection()
{
  if (!adjusting) {
    adjusting = true;
    ui->actionVertical_selection->setChecked(
          !ui->actionHorizontal_selection->isChecked());
    adjusting = false;
  }
}

void
TimeWindow::onToggleVerticalSelection()
{
  if (!adjusting) {
    adjusting = true;
    ui->actionHorizontal_selection->setChecked(
          !ui->actionVertical_selection->isChecked());
    adjusting = false;
  }
}
#endif
void
TimeWindow::onZoomToSelection()
{
  if (ui->realWaveform->getHorizontalSelectionPresent()) {
    ui->realWaveform->zoomHorizontal(
          static_cast<qint64>(
            ui->realWaveform->getHorizontalSelectionStart()),
          static_cast<qint64>(
            ui->realWaveform->getHorizontalSelectionEnd()));
    ui->imagWaveform->zoomHorizontal(
          static_cast<qint64>(
            ui->realWaveform->getHorizontalSelectionStart()),
          static_cast<qint64>(
            ui->realWaveform->getHorizontalSelectionEnd()));
    ui->realWaveform->invalidate();
    ui->imagWaveform->invalidate();
  }
}

void
TimeWindow::onZoomReset()
{
  ui->realWaveform->zoomHorizontalReset();
  ui->imagWaveform->zoomHorizontalReset();

  ui->realWaveform->zoomVerticalReset();
  ui->imagWaveform->zoomVerticalReset();

  ui->realWaveform->invalidate();
  ui->imagWaveform->invalidate();
}

void
TimeWindow::onShowWaveform()
{
  ui->realWaveform->setShowWaveform(
        ui->actionShowWaveform->isChecked());

  ui->imagWaveform->setShowWaveform(
        ui->actionShowWaveform->isChecked());
}

void
TimeWindow::onShowEnvelope()
{
  ui->realWaveform->setShowEnvelope(
        ui->actionShowEnvelope->isChecked());

  ui->imagWaveform->setShowEnvelope(
        ui->actionShowEnvelope->isChecked());

  ui->actionShowPhase->setEnabled(
        ui->actionShowEnvelope->isChecked());

  ui->actionPhaseDerivative->setEnabled(
        ui->actionShowEnvelope->isChecked());
}

void
TimeWindow::onAbort()
{
  m_taskController.cancel();
}

void
TimeWindow::onShowPhase()
{
  ui->realWaveform->setShowPhase(ui->actionShowPhase->isChecked());
  ui->imagWaveform->setShowPhase(ui->actionShowPhase->isChecked());

  ui->actionPhaseDerivative->setEnabled(
        ui->actionShowPhase->isChecked());
}

void
TimeWindow::onPhaseDerivative()
{
  ui->realWaveform->setShowPhaseDiff(
        ui->actionPhaseDerivative->isChecked());

  ui->imagWaveform->setShowPhaseDiff(
        ui->actionPhaseDerivative->isChecked());
}

void
TimeWindow::onPaletteChanged(int index)
{
  const Palette *palette = SigDiggerHelpers::instance()->getPalette(index);

  if (palette != nullptr) {
    ui->realWaveform->setPalette(palette->getGradient());
    ui->imagWaveform->setPalette(palette->getGradient());
  }

  emit configChanged();
}

void
TimeWindow::onChangePaletteOffset(int val)
{
  ui->realWaveform->setPhaseDiffOrigin(static_cast<unsigned>(val));
  ui->imagWaveform->setPhaseDiffOrigin(static_cast<unsigned>(val));

  emit configChanged();
}

void
TimeWindow::onChangePaletteContrast(int contrast)
{
  qreal realContrast = std::pow(
        static_cast<qreal>(10),
        static_cast<qreal>(contrast / 20.));

  ui->realWaveform->setPhaseDiffContrast(realContrast);
  ui->imagWaveform->setPhaseDiffContrast(realContrast);

  emit configChanged();
}

void
TimeWindow::onTaskCancelling()
{
  ui->taskProgressBar->setEnabled(false);
  ui->taskStateLabel->setText("Cancelling...");
}

void
TimeWindow::onTaskProgress(qreal progress, QString status)
{
  ui->taskStateLabel->setText(status);
  ui->taskProgressBar->setValue(static_cast<int>(progress * 100));
}

void
TimeWindow::onTaskDone()
{
  ui->taskStateLabel->setText("Done.");
  ui->taskProgressBar->setValue(0);

  if (m_taskController.getName() == "guessCarrier") {
    const CarrierDetector *cd =
        static_cast<const CarrierDetector *>(m_taskController.getTask());
    SUFLOAT relFreq = SU_ANG2NORM_FREQ(cd->getPeak());
    const SUCOMPLEX *orig = nullptr;
    SUCOMPLEX *dest = nullptr;
    SUSCOUNT len = 0;

    getTransformRegion(
          orig,
          dest,
          len,
          ui->afcSelCheck->isChecked());

    // Some UI feedback
    ui->syncFreqSpin->setValue(SU_NORM2ABS_FREQ(m_fs, relFreq));

    // Translate
    CarrierXlator *cx = new CarrierXlator(orig, dest, len, relFreq, 0);

    // Launch carrier translator
    m_taskController.process("xlateCarrier", cx);
  } else if (m_taskController.getName() == "xlateCarrier") {
    setDisplayData(
          m_processedData.data(),
          m_processedData.size(),
          true);
    notifyTaskRunning(false);
  } else if (m_taskController.getName() == "triggerHistogram") {
    m_histogramDialog->show();
    notifyTaskRunning(false);
  } else if (m_taskController.getName() == "triggerSampler") {
    m_samplerDialog->show();
    notifyTaskRunning(false);
  } else if (m_taskController.getName() == "computeDoppler") {
    SUFLOAT lambda = static_cast<SUFLOAT>(299792458. / ui->refFreqSpin->value());
    // Oh my god. Please provide something better than this
    DopplerCalculator *dc =
        (DopplerCalculator *) m_taskController.getTask();
    std::vector<SUCOMPLEX> spectrum = std::move(dc->takeSpectrum());

    // If the selected wave was captured at a sample rate fs,
    // then the RBW is fs / data.size()
    // Therefore delta V is RBW * lambda

    notifyTaskRunning(false);

    m_dopplerDialog->setVelocityStep(m_fs / spectrum.size() * lambda);
    m_dopplerDialog->setSigmaV(static_cast<qreal>(dc->getSigma()));
    m_dopplerDialog->setCenterFreq(ui->refFreqSpin->value());
    m_dopplerDialog->setDominantVelocity(static_cast<qreal>(dc->getPeak()));
    m_dopplerDialog->giveSpectrum(std::move(spectrum));
    m_dopplerDialog->setMax(dc->getMax());
    m_dopplerDialog->show();
  } else {
    setDisplayData(getData(), getLength(), true);
    setDisplayData(
          m_processedData.data(),
          m_processedData.size(),
          true);
    ui->realWaveform->invalidate();
    ui->imagWaveform->invalidate();
    onFit();
    notifyTaskRunning(false);
  }
}

void
TimeWindow::onTaskCancelled()
{
  ui->taskProgressBar->setEnabled(true);
  ui->taskStateLabel->setText("Idle");
  ui->taskProgressBar->setValue(0);

  notifyTaskRunning(false);
}

void
TimeWindow::onTaskError(QString error)
{
  ui->taskStateLabel->setText("Idle");
  ui->taskProgressBar->setValue(0);

  notifyTaskRunning(false);

  QMessageBox::warning(this, "Background task failed", "Task failed: " + error);
}

void
TimeWindow::onGuessCarrier()
{
  if (ui->realWaveform->getHorizontalSelectionPresent()) {
    const SUCOMPLEX *data = getDisplayData();
    qint64 selStart = static_cast<qint64>(
          ui->realWaveform->getHorizontalSelectionStart());
    qint64 selEnd = static_cast<qint64>(
          ui->realWaveform->getHorizontalSelectionEnd());

    CarrierDetector *cd = new CarrierDetector(
          data + selStart,
          selEnd - selStart,
          static_cast<qreal>(ui->averagerSlider->value())
          / static_cast<qreal>(ui->averagerSlider->maximum()),
          static_cast<qreal>(ui->dcNotchSlider->value())
          / static_cast<qreal>(ui->dcNotchSlider->maximum()));

    notifyTaskRunning(true);
    m_taskController.process("guessCarrier", cd);
  }
}

void
TimeWindow::onSyncCarrier()
{
  SUFLOAT relFreq = SU_ABS2NORM_FREQ(
        m_fs,
        ui->syncFreqSpin->value());
  SUFLOAT phase = SU_DEG2RAD(ui->syncPhaseSpin->value());
  const SUCOMPLEX *orig = nullptr;
  SUCOMPLEX *dest = nullptr;
  SUSCOUNT len = 0;

  getTransformRegion(
        orig,
        dest,
        len,
        ui->afcSelCheck->isChecked());

  CarrierXlator *cx = new CarrierXlator(orig, dest, len, relFreq, phase);

  notifyTaskRunning(true);
  m_taskController.process("xlateCarrier", cx);
}

void
TimeWindow::onResetCarrier()
{
  setDisplayData(getData(), getLength(), true);
  onFit();
  ui->syncFreqSpin->setValue(0);
}

void
TimeWindow::onCarrierSlidersChanged()
{
  qreal notchRelBw =
      static_cast<qreal>(ui->dcNotchSlider->value())
      / static_cast<qreal>(ui->dcNotchSlider->maximum());
  qreal avgRelBw =
      static_cast<qreal>(ui->averagerSlider->value())
      / static_cast<qreal>(ui->averagerSlider->maximum());

  ui->notchWidthLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          m_fs * notchRelBw,
          6,
          "Hz"));

  ui->averagerSpanLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          m_fs * avgRelBw,
          6,
          "Hz"));
}

void
TimeWindow::onHistogramSamples(const float *samples, unsigned int size)
{
  m_histogramDialog->feed(samples, size);
}

void
TimeWindow::onTriggerHistogram()
{
  SamplingProperties props;

  if (!ui->realWaveform->isComplete())
    return;

  populateSamplingProperties(props);

  HistogramFeeder *hf = new HistogramFeeder(props);

  connect(
        hf,
        SIGNAL(data(const float *, unsigned int)),
        this,
        SLOT(onHistogramSamples(const float *, unsigned int)));

  m_histogramDialog->reset();
  m_histogramDialog->setProperties(props);
  m_histogramDialog->show();
  notifyTaskRunning(true);
  m_taskController.process("triggerHistogram", hf);
}

void
TimeWindow::onHistogramBlanked()
{
  if (m_histogramDialog->isVisible())
    onTriggerHistogram();
}

void
TimeWindow::onSampleSet(SigDigger::WaveSampleSet set)
{
  m_samplerDialog->feedSet(set);
}

void
TimeWindow::onTriggerSampler()
{
  if (!ui->realWaveform->isComplete())
    return;

  SamplingProperties props;
  SUCOMPLEX dataMin = 2 * ui->realWaveform->getDataRMS();
  SUCOMPLEX dataMax = 2 * ui->realWaveform->getDataRMS();
  SUFLOAT   maxAmp;

  populateSamplingProperties(props);

  if (props.length == 0) {
    QMessageBox::warning(
          this,
          "Start sampling",
          "Cannot perform sampling: nothing to sample");
    return;
  }

  if (props.rate < props.fs / SCAST(qreal, props.length)) {
    ui->baudSpin->setFocus();
    QMessageBox::warning(
          this,
          "Start sampling",
          "Cannot perform sampling: symbol rate is too small (or zero)");
    return;
  }

  if (props.sync == ZERO_CROSSING) {
    maxAmp = 1;
  } else {
    maxAmp =
        2 * SU_MAX(
          SU_MAX(
            SU_C_REAL(dataMin),
            SU_C_IMAG(dataMin)),
          SU_MAX(
            SU_C_REAL(dataMax),
            SU_C_IMAG(dataMax)));

  }

  if (maxAmp < 0)
    maxAmp = 0;

  m_samplerDialog->reset();
  m_samplerDialog->setAmplitudeLimits(0, maxAmp);
  m_samplerDialog->setProperties(props);

  startSampling();
}

void
TimeWindow::onResample()
{
  if (m_samplerDialog->isVisible()) {
    m_samplerDialog->reset();
    startSampling();
  }
}

bool
TimeWindow::fineTuneSenderIs(const QPushButton *button) const
{
  QPushButton *sender = static_cast<QPushButton *>(this->sender());

  if (ui->lockButton->isChecked()) {
#define CHECKPAIR(a, b)                                   \
  if (button == ui->a || button == ui->b)     \
    return sender == ui->a || sender == ui->b

    CHECKPAIR(selStartIncDeltaTButton, selEndIncDeltaTButton);
    CHECKPAIR(selStartIncSampleButton, selEndIncSampleButton);
    CHECKPAIR(selStartDecDeltaTButton, selEndDecDeltaTButton);
    CHECKPAIR(selStartDecSampleButton, selEndDecSampleButton);
#undef CHECKPAIR
  }

  return button == sender;
}

void
TimeWindow::onFineTuneSelectionClicked()
{
  qint64 newSelStart =
      static_cast<qint64>(ui->realWaveform->getHorizontalSelectionStart());
  qint64 newSelEnd =
      static_cast<qint64>(ui->realWaveform->getHorizontalSelectionEnd());
  qint64 delta = (newSelEnd - newSelStart) / ui->periodicDivisionsSpin->value();

#define CHECKBUTTON(btn) fineTuneSenderIs(ui->btn)

  if (CHECKBUTTON(selStartIncDeltaTButton))
    newSelStart += delta;

  if (CHECKBUTTON(selStartIncSampleButton))
    ++newSelStart;

  if (CHECKBUTTON(selStartDecDeltaTButton))
    newSelStart -= delta;

  if (CHECKBUTTON(selStartDecSampleButton))
    --newSelStart;

  if (CHECKBUTTON(selEndIncDeltaTButton))
    newSelEnd += delta;

  if (CHECKBUTTON(selEndIncSampleButton))
    ++newSelEnd;

  if (CHECKBUTTON(selEndDecDeltaTButton))
    newSelEnd -= delta;

  if (CHECKBUTTON(selEndDecSampleButton))
    --newSelEnd;

#undef CHECKBUTTON

  ui->imagWaveform->selectHorizontal(newSelStart, newSelEnd);
  ui->realWaveform->selectHorizontal(newSelStart, newSelEnd);
}

void
TimeWindow::onClkSourceButtonClicked()
{
  refreshUi();
}

void
TimeWindow::onCalculateDoppler()
{
  if (!ui->realWaveform->isComplete())
    return;

  if (ui->realWaveform->getHorizontalSelectionPresent()) {
    const SUCOMPLEX *data = getDisplayData();
    qint64 selStart = static_cast<qint64>(
          ui->realWaveform->getHorizontalSelectionStart());
    qint64 selEnd = static_cast<qint64>(
          ui->realWaveform->getHorizontalSelectionEnd());

    if (selStart < 0)
      selStart = 0;

    if (selEnd >= static_cast<qint64>(getLength()))
      selEnd = static_cast<qint64>(getLength());

    DopplerCalculator *dc = new DopplerCalculator(
          ui->refFreqSpin->value(),
          data + selStart,
          static_cast<size_t>(selEnd - selStart),
          static_cast<SUFLOAT>(m_fs));

    notifyTaskRunning(true);
    m_taskController.process("computeDoppler", dc);
  }
}

void
TimeWindow::onCostasRecovery()
{
  if (!ui->realWaveform->isComplete())
    return;

  try {
    enum sigutils_costas_kind kind = SU_COSTAS_KIND_BPSK;

    SUFLOAT relBw = SU_ABS2NORM_FREQ(
          m_fs,
          ui->costasBwSpin->value());

    SUFLOAT tau = 1. / SU_ABS2NORM_BAUD(
          m_fs,
          ui->costasArmBwSpin->value());
    const SUCOMPLEX *orig;
    SUCOMPLEX *dest;
    SUSCOUNT len;

    getTransformRegion(
          orig,
          dest,
          len,
          ui->afcSelCheck->isChecked());

    switch (ui->costasOrderCombo->currentIndex()) {
      case 0:
        kind = SU_COSTAS_KIND_BPSK;
        break;

      case 1:
        kind = SU_COSTAS_KIND_QPSK;
        break;

      case 2:
        kind = SU_COSTAS_KIND_8PSK;
        break;
    }

    CostasRecoveryTask *task = new CostasRecoveryTask(
          orig,
          dest,
          len,
          tau,
          relBw,
          kind);

    notifyTaskRunning(true);
    m_taskController.process("costas", task);
  } catch (Suscan::Exception &e) {
    QMessageBox::warning(
          this,
          "Costas carrier recovery",
          "Cannot perform operation: " + QString(e.what()));
  }
}

void
TimeWindow::onPLLRecovery()
{
  if (!ui->realWaveform->isComplete())
    return;

  try {
    SUFLOAT relBw = SU_ABS2NORM_FREQ(
          m_fs,
          ui->pllCutOffSpin->value());
    const SUCOMPLEX *orig;
    SUCOMPLEX *dest;
    SUSCOUNT len;

    getTransformRegion(
          orig,
          dest,
          len,
          ui->afcSelCheck->isChecked());

    PLLSyncTask *task = new PLLSyncTask(orig, dest, len, relBw);

    notifyTaskRunning(true);
    m_taskController.process("pll", task);
  } catch (Suscan::Exception &e) {
    QMessageBox::warning(
          this,
          "PLL carrier recovery",
          "Cannot perform operation: " + QString(e.what()));
  }
}

void
TimeWindow::onCycloAnalysis()
{
  if (!ui->realWaveform->isComplete())
    return;

  try {
    const SUCOMPLEX *orig;
    SUCOMPLEX *dest;
    SUSCOUNT len;

    getTransformRegion(
          orig,
          dest,
          len,
          ui->transSelCheck->isChecked());

    DelayedConjTask *task = new DelayedConjTask(orig, dest, len, 1);

    notifyTaskRunning(true);
    m_taskController.process("cyclo", task);
  } catch (Suscan::Exception &e) {
    QMessageBox::warning(
          this,
          "Cyclostationary analysis",
          "Cannot perform operation: " + QString(e.what()));
  }
}

void
TimeWindow::onQuadDemod()
{
  if (!ui->realWaveform->isComplete())
    return;

  try {
    const SUCOMPLEX *orig;
    SUCOMPLEX *dest;
    SUSCOUNT len;

    getTransformRegion(
          orig,
          dest,
          len,
          ui->transSelCheck->isChecked());

    QuadDemodTask *task = new QuadDemodTask(orig, dest, len);

    notifyTaskRunning(true);
    m_taskController.process("quadDemod", task);
  } catch (Suscan::Exception &e) {
    QMessageBox::warning(
          this,
          "Quadrature demodulator",
          "Cannot perform operation: " + QString(e.what()));
  }
}

void
TimeWindow::onAGC()
{
  if (!ui->realWaveform->isComplete())
    return;

  try {
    SUFLOAT rate = SU_ABS2NORM_BAUD(
          m_fs,
          ui->agcRateSpin->value());
    SUFLOAT tau = 1 / rate;
    const SUCOMPLEX *orig;
    SUCOMPLEX *dest;
    SUSCOUNT len;

    getTransformRegion(
          orig,
          dest,
          len,
          ui->transSelCheck->isChecked());

    if (isinf(tau) || isnan(tau)) {
      QMessageBox::warning(
            this,
            "Automatic Gain Control",
            "Cannot perform automatic gain control: rate is too slow");
    } else if (tau <= 1.) {
      QMessageBox::warning(
            this,
            "Automatic Gain Control",
            "Cannot perform automatic gain control: rate is faster than sample rate");
    } else {
      AGCTask *task = new AGCTask(orig, dest, len, tau);

      notifyTaskRunning(true);
      m_taskController.process("agc", task);
    }
  } catch (Suscan::Exception &e) {
    QMessageBox::warning(
          this,
          "Automatic Gain Control",
          "Cannot perform operation: " + QString(e.what()));
  }
}

void
TimeWindow::onLPF()
{
  if (!ui->realWaveform->isComplete())
    return;

  try {
    SUFLOAT bw = SU_ABS2NORM_FREQ(
          m_fs,
          ui->lpfSpin->value());
    const SUCOMPLEX *orig;
    SUCOMPLEX *dest;
    SUSCOUNT len;

    getTransformRegion(
          orig,
          dest,
          len,
          ui->transSelCheck->isChecked());

    if (bw >= 1.f)
      bw = 1;

    LPFTask *task = new LPFTask(orig, dest, len, bw);

    notifyTaskRunning(true);
    m_taskController.process("lpf", task);
  } catch (Suscan::Exception &e) {
    QMessageBox::warning(
          this,
          "Low-pass filter",
          "Cannot perform operation: " + QString(e.what()));
  }
}

void
TimeWindow::onDelayedConjugate()
{
  if (!ui->realWaveform->isComplete())
    return;

  try {
    SUFLOAT rate = SU_ABS2NORM_BAUD(
          m_fs,
          ui->dcmRateSpin->value());
    SUFLOAT tau = 1. / rate;
    SUSCOUNT samples = static_cast<SUSCOUNT>(tau);
    const SUCOMPLEX *orig = nullptr;
    SUCOMPLEX *dest = nullptr;
    SUSCOUNT len = 0;

    getTransformRegion(
          orig,
          dest,
          len,
          ui->transSelCheck->isChecked());

    if (isinf(tau) || isnan(tau) || tau >= getDisplayDataLength()) {
      QMessageBox::warning(
            this,
            "Product by the delayed conjugate",
            "Product by the delayed conjugate exceeds capture length");
    } else if (samples <= 1) {
      QMessageBox::warning(
            this,
            "Product by the delayed conjugate",
            "Product by the delayed conjugate: rate is faster than sample rate");
    } else {
      DelayedConjTask *task = new DelayedConjTask(orig, dest, len, samples);

      notifyTaskRunning(true);
      m_taskController.process("delayedConj", task);
    }
  } catch (Suscan::Exception &e) {
    QMessageBox::warning(
          this,
          "Product by the delayed conjugate",
          "Cannot perform operation: " + QString(e.what()));
  }
}

void
TimeWindow::onAGCRateChanged()
{
  SUFLOAT absRate = ui->agcRateSpin->value();
  SUFLOAT rate = SU_ABS2NORM_BAUD(m_fs, absRate);
  SUFLOAT tau = 1 / rate;
  SUSCOUNT samples = static_cast<SUSCOUNT>(tau);

  ui->agcTauLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          1. / absRate,
          4) + " (" +
        SuWidgetsHelpers::formatQuantity(
          static_cast<qreal>(samples),
          4,
          "sp") + ")");
}

void
TimeWindow::onDelayedConjChanged()
{
  SUFLOAT absRate = ui->dcmRateSpin->value();
  SUFLOAT rate = SU_ABS2NORM_BAUD(m_fs, absRate);
  SUFLOAT tau = 1 / rate;
  SUSCOUNT samples = static_cast<SUSCOUNT>(tau);

  ui->dcmTauLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          1. / absRate,
          4) + " (" +
        SuWidgetsHelpers::formatQuantity(
          static_cast<qreal>(samples),
          4,
          "sp") + ")");
}

void
TimeWindow::onWaveViewChanged()
{
  refreshMeasures();
  onFit();
}

void
TimeWindow::onZeroCrossingComponentChanged()
{
  qreal min = -1, max = 1;
  int digits;

  switch (ui->clkComponentCombo->currentIndex()) {
    case 0:
      min = ui->realWaveform->getMin();
      max = ui->realWaveform->getMax();
      break;

    case 1:
      min = ui->imagWaveform->getMin();
      max = ui->imagWaveform->getMax();
      break;

    case 2:
      min = 0;
      max = ui->realWaveform->getEnvelope();
      break;
  }

  digits = SCAST(int, floor(log10(max - min)));

  ui->zeroPointSpin->setDecimals(7 - digits);
  ui->zeroPointSpin->setSingleStep(5e-3 * (max - min));
  ui->zeroPointSpin->setMinimum(min);
  ui->zeroPointSpin->setMaximum(max);

  onZeroPointChanged();
}

void
TimeWindow::onZeroPointChanged()
{
  QList<WaveVCursor> vCursors, vEmpty;
  QList<WaveACursor> aCursors, aEmpty;
  bool isAmplitude = false;
  bool imag = false;
  bool real = false;

  isAmplitude = ui->decAmplitudeButton->isChecked();

  ui->zeroPointSpin->setEnabled(isAmplitude);
  ui->clkComponentCombo->setEnabled(isAmplitude);

  if (ui->clkZcButton->isChecked() && isAmplitude) {
    WaveVCursor vCursor;
    WaveACursor aCursor;

    vCursor.color  = QColor(0xff, 0xff, 0x00);
    vCursor.string = "Zero point";

    switch (ui->clkComponentCombo->currentIndex()) {
      case 0:
        vCursor.level = SCAST(SUCOMPLEX, ui->zeroPointSpin->value());
        vCursors.append(vCursor);
        real = true;
        break;

      case 1:
        vCursor.level = SU_I * SCAST(SUCOMPLEX, ui->zeroPointSpin->value());
        vCursors.append(vCursor);
        imag = true;
        break;

      case 2:
        aCursor.color     = QColor(0xc1, 0x2e, 0x81);
        aCursor.string    = vCursor.string;
        aCursor.amplitude = ui->zeroPointSpin->value();
        aCursors.append(aCursor);
        real = imag = true;
        break;
    }
  }

  if (real) {
    ui->realWaveform->setVCursorList(vCursors);
    ui->realWaveform->setACursorList(aCursors);
  } else {
    ui->realWaveform->setVCursorList(vEmpty);
    ui->realWaveform->setACursorList(aEmpty);
  }

  if (imag) {
    ui->imagWaveform->setVCursorList(vCursors);
    ui->imagWaveform->setACursorList(aCursors);
  } else {
    ui->imagWaveform->setVCursorList(vEmpty);
    ui->imagWaveform->setACursorList(aEmpty);
  }
}
