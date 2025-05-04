//
//    TVProcessorTab.cpp: Inspector's TV processor tab
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

#include "TVProcessorTab.h"
#include "ui_TVProcessorTab.h"
#include <QMessageBox>
#include <SuWidgetsHelpers.h>
#include <QThread>
#include <QFileDialog>

using namespace SigDigger;

#define EP 1e2
#define EP_INV (1. / (EP))

TVProcessorTab::TVProcessorTab(QWidget *parent, qreal rate) :
  QWidget(parent),
  ui(new Ui::TVProcessorTab)
{
  m_sampleRate = rate;

  ui->setupUi(this);

  m_tvThread = new QThread();
  m_tvWorker = new TVProcessorWorker();
  m_tvWorker->moveToThread(m_tvThread);

  connect(
        m_tvThread,
        &QThread::finished,
        m_tvWorker,
        &QObject::deleteLater);

  connect(
        m_tvThread,
        &QThread::finished,
        m_tvThread,
        &QObject::deleteLater);

  m_tvThread->start();

  ui->linePeriodSpin->setDecimals(3);
  connectAll();
  onTVProcessorUiChanged();
}

TVProcessorTab::~TVProcessorTab()
{
  if (m_tvThread != nullptr)
    m_tvThread->quit();

  delete ui;
}

void
TVProcessorTab::connectAll(void)
{
  connect(
        m_tvWorker,
        SIGNAL(paramsChanged(struct sigutils_tv_processor_params)),
        this,
        SLOT(onTVProcessorParamsChanged(struct sigutils_tv_processor_params)));

  connect(
        m_tvWorker,
        SIGNAL(error(QString)),
        this,
        SLOT(onTVProcessorError(QString)));

  connect(
        m_tvWorker,
        SIGNAL(frame(struct sigutils_tv_frame_buffer *)),
        this,
        SLOT(onTVProcessorFrame(struct sigutils_tv_frame_buffer *)));

  connect(
        ui->enableTvButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleTVProcessor(void)));

  connect(
        ui->accumButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onAccumChanged(void)));

  connect(
        ui->lpfButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onEnableLPFChanged(void)));

  connect(
        ui->accumSpinBox,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onAccumSpinChanged(void)));

  connect(
        ui->snapshotButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveSnapshot(void)));

  connect(
        this,
        SIGNAL(startTVProcessor(void)),
        m_tvWorker,
        SLOT(start(void)));

  connect(
        this,
        SIGNAL(stopTVProcessor(void)),
        m_tvWorker,
        SLOT(stop(void)));

  connect(
        this,
        SIGNAL(tvProcessorData()),
        m_tvWorker,
        SLOT(process()));

  connect(
        this,
        SIGNAL(tvProcessorDisposeFrame(struct sigutils_tv_frame_buffer *)),
        m_tvWorker,
        SLOT(returnFrame(struct sigutils_tv_frame_buffer *)));

  connect(
        this,
        SIGNAL(tvProcessorParams(sigutils_tv_processor_params)),
        m_tvWorker,
        SLOT(setParams(sigutils_tv_processor_params)));

  // UI signals
  connect(
        ui->ntscRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->palRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->customRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->agcCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->enableSyncCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->invertSyncCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->invertImageCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->combFilterCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->swapCombFilterButton,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->nonInterlacedRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->fieldOneRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->fieldTwoRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->linesSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->vsyncTrainLengthSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->agcTauSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->slowTrackTauSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->fastTrackTauSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->lineLenTauSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->hsyncSpin,
        SIGNAL(changed(qreal, qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->vsyncSpin,
        SIGNAL(changed(qreal, qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->linePeriodSpin,
        SIGNAL(changed(qreal, qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->hugeErrorSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->hsyncErrorRange,
        SIGNAL(rangeChanged(int, int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->timeTolSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->levelTolSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->geomTolSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        ui->brightnessDial,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVBrightnessChanged()));

  connect(
        ui->contrastDial,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVContrastChanged()));


  connect(
        ui->gammaDial,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVGammaChanged()));

  connect(
        ui->rotationDial,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVAspectChanged()));

  connect(
        ui->tvZoomSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVAspectChanged()));

  connect(
        ui->flipHorizontalCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVAspectChanged()));


  connect(
        ui->flipVerticalCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVAspectChanged()));
}

SUFLOAT
TVProcessorTab::getSampleRateFloat(void) const
{
  return static_cast<SUFLOAT>(m_sampleRate);
}

unsigned int
TVProcessorTab::getSampleRate(void) const
{
  return static_cast<unsigned int>(m_sampleRate);
}

void
TVProcessorTab::setSampleRate(qreal sampleRate)
{
  m_sampleRate = sampleRate;
  refreshUiState();
  emitParameters();
}

void
TVProcessorTab::setDecisionMode(Decider::DecisionMode mode)
{
  m_decisionMode = mode;
}

void
TVProcessorTab::emitParameters(void)
{
  if (getSampleRate() > 0) {
    struct sigutils_tv_processor_params params;

    refreshUiState();
    parseUi(params);

    emit tvProcessorParams(params);
    if (m_tvProcessing)
      emit startTVProcessor();
    ui->enableTvButton->setEnabled(true);

    ui->tvDisplay->setPicGeometry(
          static_cast<int>(params.line_len),
          static_cast<int>(params.frame_lines));
  } else {
    ui->enableTvButton->setEnabled(false);
    if (m_tvProcessing) {
      emit stopTVProcessor();
      m_tvProcessing = false;
      ui->enableTvButton->setChecked(false);
    }
  }
}

void
TVProcessorTab::refreshUiState(void)
{
  bool customParamsEnabled = ui->customRadio->isChecked();

  if (getSampleRate() > 0) {
    ui->linePeriodSpin->setSampleRate(getSampleRate());
    ui->hsyncSpin->setSampleRate(getSampleRate());
    ui->vsyncSpin->setSampleRate(getSampleRate());
  }

  ui->geometryGroup->setEnabled(customParamsEnabled);
  ui->agcCheck->setEnabled(customParamsEnabled);
  ui->syncTab->setEnabled(customParamsEnabled);
  ui->invertImageCheck->setEnabled(customParamsEnabled);

  ui->nonInterlacedRadio->setEnabled(customParamsEnabled);
  ui->fieldOneRadio->setEnabled(customParamsEnabled);
  ui->fieldTwoRadio->setEnabled(customParamsEnabled);


  if (!customParamsEnabled)
    ui->invertImageCheck->setChecked(
        ui->invertSyncCheck->isChecked());

  ui->swapCombFilterButton->setEnabled(
        ui->combFilterCheck->isChecked());

  ui->hsyncSpin->setEnabled(ui->enableSyncCheck->isChecked());
  ui->vsyncSpin->setEnabled(ui->enableSyncCheck->isChecked());

  ui->levelTolLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          100 * std::pow(10., ui->levelTolSlider->value() / 100.),
          3,
          ""));

  ui->timeTolLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          100 * std::pow(10., ui->timeTolSlider->value() / 100.),
          3,
          ""));

  ui->geomTolLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          100 * std::pow(10., ui->geomTolSlider->value() / 100.),
          3,
          ""));

  ui->hsyncErrorRangeLabel->setText(
        QString::number(
          EP_INV * std::round(
            EP * 100 * std::pow(
              10.,
              ui->hsyncErrorRange->minimumPosition() / 100.)))
        + "% - "
        + QString::number(
            EP_INV * std::round(
              EP * 100 * std::pow(
                10.,
                ui->hsyncErrorRange->maximumPosition() / 100.)))
        + "%");

  ui->hugeErrorLabel->setText(
        QString::number(
          EP_INV * std::round(
            EP * 100 * std::pow(10., ui->hugeErrorSlider->value() / 100.)))
        + "%");

  ui->frameRateSpin->setValue(
        1. / (ui->linesSpin->value()
              * ui->linePeriodSpin->timeValue()));
}

void
TVProcessorTab::refreshUi(
    struct sigutils_tv_processor_params const &params)
{
  bool oldState = m_editingTVProcessorParams;
  m_editingTVProcessorParams = true;

  BLOCKSIG(ui->combFilterCheck, setChecked(params.enable_comb));
  BLOCKSIG(ui->agcCheck, setChecked(params.enable_agc));
  BLOCKSIG(ui->enableSyncCheck, setChecked(params.enable_sync));
  BLOCKSIG(ui->invertImageCheck, setChecked(params.reverse));
  BLOCKSIG(ui->swapCombFilterButton, setChecked(params.comb_reverse));

  if (!params.interlace) {
    BLOCKSIG(ui->nonInterlacedRadio, setChecked(true));
  } else if (params.dominance) {
    BLOCKSIG(ui->fieldOneRadio, setChecked(true));
  } else {
    BLOCKSIG(ui->fieldTwoRadio, setChecked(true));
  }

  refreshUiState();

  BLOCKSIG(ui->linesSpin, setValue(
        static_cast<qreal>(params.frame_lines) + params.frame_spacing));

  BLOCKSIG(ui->hsyncSpin, setSamplesValue(
        static_cast<qreal>(params.hsync_len)));

  BLOCKSIG(ui->vsyncSpin, setSamplesValue(
        static_cast<qreal>(params.vsync_len)));

  BLOCKSIG(ui->linePeriodSpin, setSamplesValue(
        static_cast<qreal>(params.line_len)));

  ui->hsyncSpin->setBestUnits(true);
  ui->vsyncSpin->setBestUnits(true);
  ui->linePeriodSpin->setBestUnits(true);

  BLOCKSIG(ui->timeTolSlider, setValue(
        static_cast<int>(100 * std::log10(params.t_tol))));
  BLOCKSIG(ui->levelTolSlider, setValue(
        static_cast<int>(100 * std::log10(params.l_tol))));
  BLOCKSIG(ui->geomTolSlider, setValue(
        static_cast<int>(100 * std::log10(params.g_tol))));

  BLOCKSIG(ui->hsyncErrorRange, setMinimumPosition(
        static_cast<int>(100 * std::log10(params.hsync_min_err))));
  BLOCKSIG(ui->hsyncErrorRange, setMaximumPosition(
        static_cast<int>(100 * std::log10(params.hsync_max_err))));
  BLOCKSIG(ui->hugeErrorSlider, setValue(
        static_cast<int>(100 * std::log10(params.hsync_huge_err))));
  BLOCKSIG(ui->vsyncTrainLengthSpin, setValue(
        static_cast<int>(params.vsync_odd_trigger)));

  BLOCKSIG(ui->agcTauSpin, setValue(
        static_cast<qreal>(params.agc_tau)));
  BLOCKSIG(ui->lineLenTauSpin, setValue(
        static_cast<qreal>(params.line_len_tau)));
  BLOCKSIG(ui->fastTrackTauSpin, setValue(
        static_cast<qreal>(params.hsync_fast_track_tau)));
  BLOCKSIG(ui->slowTrackTauSpin, setValue(
        static_cast<qreal>(params.hsync_slow_track_tau)));

  m_editingTVProcessorParams = oldState;
}

bool
TVProcessorTab::parseUi(
    struct sigutils_tv_processor_params &params)
{
  qreal lines = ui->linesSpin->value();
  params.enable_sync  = ui->enableSyncCheck->isChecked();
  params.reverse      = ui->invertImageCheck->isChecked();
  params.interlace    = !ui->nonInterlacedRadio->isChecked();
  params.dominance    = !ui->fieldTwoRadio->isChecked();
  params.enable_agc   = ui->agcCheck->isChecked();
  params.frame_lines  = static_cast<SUSCOUNT>(floor(lines));
  params.frame_spacing = lines - params.frame_lines;

  params.enable_comb  = ui->combFilterCheck->isChecked();
  params.comb_reverse = ui->swapCombFilterButton->isChecked();
  params.x_off        = 0;
  params.hsync_len    = static_cast<SUFLOAT>(
        ui->hsyncSpin->samplesValue());
  params.vsync_len    = static_cast<SUFLOAT>(
        ui->vsyncSpin->samplesValue());
  params.line_len     = static_cast<SUFLOAT>(
        ui->linePeriodSpin->samplesValue());
  params.t_tol        = static_cast<SUFLOAT>(
        std::pow(10., ui->timeTolSlider->value() / 100.));
  params.l_tol        = static_cast<SUFLOAT>(
        std::pow(10., ui->levelTolSlider->value() / 100.));
  params.g_tol        = static_cast<SUFLOAT>(
        std::pow(10., ui->geomTolSlider->value() / 100.));

  params.hsync_huge_err = static_cast<SUFLOAT>(
        std::pow(10., ui->hugeErrorSlider->value() / 100.));
  params.hsync_min_err  = static_cast<SUFLOAT>(
        std::pow(10., ui->hsyncErrorRange->minimumPosition() / 100.));
  params.hsync_max_err  = static_cast<SUFLOAT>(
        std::pow(10., ui->hsyncErrorRange->maximumPosition() / 100.));
  params.vsync_odd_trigger  = static_cast<SUSCOUNT>(
        ui->vsyncTrainLengthSpin->value());

  params.hsync_len_tau = 9.5;

  params.line_len_tau  = static_cast<SUFLOAT>(
        ui->lineLenTauSpin->value());
  params.hsync_fast_track_tau  = static_cast<SUFLOAT>(
        ui->fastTrackTauSpin->value());
  params.hsync_slow_track_tau  = static_cast<SUFLOAT>(
        ui->slowTrackTauSpin->value());
  params.agc_tau  = static_cast<SUFLOAT>(
        ui->agcTauSpin->value());

  return true;
}

void
TVProcessorTab::feed(const SUCOMPLEX *data, unsigned int size)
{
  SUFLOAT k = ui->invertSyncCheck->isChecked() ? -1 : 1;
  SUFLOAT dc = static_cast<SUFLOAT>(ui->dcSpin->value()) / 100;

  m_floatBuffer.resize(size);

  if (m_decisionMode == Decider::MODULUS) {
    for (unsigned i = 0; i < size; ++i)
      m_floatBuffer[i] = k * SU_C_ABS(data[i]) + dc;
  } else {
    for (unsigned i = 0; i < size; ++i)
      m_floatBuffer[i] = k * SU_C_ARG(data[i]) / PI + dc;
  }

  m_tvWorker->pushData(m_floatBuffer);

  emit tvProcessorData();
}


////////////////////////////////// Slots ///////////////////////////////////////
void
TVProcessorTab::onTVProcessorUiChanged(void)
{
  if (!m_editingTVProcessorParams) {
    if (ui->ntscRadio->isChecked()) {
      struct sigutils_tv_processor_params params;
      su_tv_processor_params_ntsc(&params, getSampleRateFloat());
      refreshUi(params);
    } else if (ui->palRadio->isChecked()) {
      struct sigutils_tv_processor_params params;
      su_tv_processor_params_pal(&params, getSampleRateFloat());
      refreshUi(params);
    }

    emitParameters();
  }

  refreshUiState();
}

void
TVProcessorTab::onToggleTVProcessor(void)
{
  m_tvProcessing = ui->enableTvButton->isChecked();

  if (m_tvProcessing) {
    onTVProcessorUiChanged();
    emit startTVProcessor();
  } else {
    emit stopTVProcessor();
  }
}

void
TVProcessorTab::onTVProcessorFrame(struct sigutils_tv_frame_buffer *frame)
{
  m_tvWorker->acknowledgeFrame();
  ui->tvDisplay->putFrame(frame);
  ui->tvDisplay->invalidate();
  emit tvProcessorDisposeFrame(frame);
}

void
TVProcessorTab::onTVProcessorParamsChanged(
    struct sigutils_tv_processor_params params)
{
  refreshUi(params);
}

void
TVProcessorTab::onTVProcessorError(QString error)
{
  QMessageBox::critical(ui->tvDisplay, "TV Processor error", error);
  ui->enableTvButton->setChecked(false);
  onToggleTVProcessor();
}

void
TVProcessorTab::onTVContrastChanged(void)
{
  ui->tvDisplay->setContrast(ui->contrastDial->value() / 100.);
}

void
TVProcessorTab::onTVBrightnessChanged(void)
{
  ui->tvDisplay->setBrightness(ui->brightnessDial->value() / 100.);
}

void
TVProcessorTab::onTVGammaChanged(void)
{
  qreal gamma = ui->gammaDial->value() / 25. + 1;

  if (gamma < 1)
    gamma = -1. / (gamma - 2);

  ui->tvDisplay->setGamma(gamma);
}

void
TVProcessorTab::onTVAspectChanged(void)
{
  ui->tvDisplay->setRotation(ui->rotationDial->value());
  ui->tvDisplay->setHorizontalFlip(
        ui->flipHorizontalCheck->isChecked());
  ui->tvDisplay->setVerticalFlip(
        ui->flipVerticalCheck->isChecked());
  ui->tvDisplay->setZoom(ui->tvZoomSpin->value());
}

void
TVProcessorTab::onSaveSnapshot(void)
{
  QFileDialog dialog(this);
  QStringList filters;

  filters << "Microsoft Windows Bitmap (*.bmp)"
          << "PNG Image (*.png)"
          << "JPEG Image (*.jpg)"
          << "Portable Pixel Map (*.ppm)";

  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setWindowTitle(QString("Save current symbol capture as..."));
  dialog.setNameFilters(filters);

  if (dialog.exec()) {
    QString path = dialog.selectedFiles().first();
    QString filter = dialog.selectedNameFilter();
    QFileInfo fi(path);
    QString ext = fi.suffix().size() > 0
        ? fi.suffix()
        : SuWidgetsHelpers::extractFilterExtension(filter);

    if (!ui->tvDisplay->saveToFile(
          SuWidgetsHelpers::ensureExtension(path, ext)))
      QMessageBox::critical(
            this,
            "Failed to take snapshot",
            "Cannot save snapshot to the specified file. Please verify if "
            "permission and disk space allow this operation.",
            QMessageBox::Close);
  }
}

void
TVProcessorTab::onAccumChanged(void)
{
  ui->tvDisplay->setAccumulate(ui->accumButton->isChecked());
  ui->lpfButton->setEnabled(ui->accumButton->isChecked());
  onEnableLPFChanged();
}

void
TVProcessorTab::onEnableLPFChanged(void)
{
  ui->tvDisplay->setEnableSPLPF(ui->lpfButton->isChecked());
  ui->accumSpinBox->setEnabled(
        ui->lpfButton->isChecked() && ui->accumButton->isChecked());
  onAccumSpinChanged();
}

void
TVProcessorTab::onAccumSpinChanged(void)
{
  ui->tvDisplay->setAccumAlpha(
          SU_SPLPF_ALPHA(ui->accumSpinBox->value() / 5.f));
}

