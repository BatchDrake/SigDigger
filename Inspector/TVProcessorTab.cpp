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

#include <TVProcessorTab.h>
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
  this->sampleRate = rate;

  ui->setupUi(this);

  this->tvThread = new QThread();
  this->tvWorker = new TVProcessorWorker();
  this->tvWorker->moveToThread(this->tvThread);

  connect(
        this->tvThread,
        &QThread::finished,
        this->tvWorker,
        &QObject::deleteLater);

  this->tvThread->start();

  this->connectAll();
  this->onTVProcessorUiChanged();
}

TVProcessorTab::~TVProcessorTab()
{
  if (this->tvThread != nullptr)
    this->tvThread->quit();

  delete ui;
}

void
TVProcessorTab::connectAll(void)
{
  connect(
        this->tvWorker,
        SIGNAL(paramsChanged(struct sigutils_tv_processor_params)),
        this,
        SLOT(onTVProcessorParamsChanged(struct sigutils_tv_processor_params)));

  connect(
        this->tvWorker,
        SIGNAL(error(QString)),
        this,
        SLOT(onTVProcessorError(QString)));

  connect(
        this->tvWorker,
        SIGNAL(frame(struct sigutils_tv_frame_buffer *)),
        this,
        SLOT(onTVProcessorFrame(struct sigutils_tv_frame_buffer *)));

  connect(
        this->ui->enableTvButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleTVProcessor(void)));

  connect(
        this->ui->accumButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onAccumChanged(void)));

  connect(
        this->ui->lpfButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onEnableLPFChanged(void)));

  connect(
        this->ui->accumSpinBox,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onAccumSpinChanged(void)));

  connect(
        this->ui->snapshotButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveSnapshot(void)));

  connect(
        this,
        SIGNAL(startTVProcessor(void)),
        this->tvWorker,
        SLOT(start(void)));

  connect(
        this,
        SIGNAL(stopTVProcessor(void)),
        this->tvWorker,
        SLOT(stop(void)));

  connect(
        this,
        SIGNAL(tvProcessorData()),
        this->tvWorker,
        SLOT(process()));

  connect(
        this,
        SIGNAL(tvProcessorDisposeFrame(struct sigutils_tv_frame_buffer *)),
        this->tvWorker,
        SLOT(returnFrame(struct sigutils_tv_frame_buffer *)));

  connect(
        this,
        SIGNAL(tvProcessorParams(sigutils_tv_processor_params)),
        this->tvWorker,
        SLOT(setParams(sigutils_tv_processor_params)));

  // UI signals
  connect(
        this->ui->ntscRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->palRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->customRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->agcCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->enableSyncCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->invertSyncCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->invertImageCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->combFilterCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->swapCombFilterButton,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->nonInterlacedRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->fieldOneRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->fieldTwoRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->linesSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->vsyncTrainLengthSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->agcTauSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->slowTrackTauSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->fastTrackTauSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->lineLenTauSpin,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->hsyncSpin,
        SIGNAL(changed(qreal, qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->vsyncSpin,
        SIGNAL(changed(qreal, qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->linePeriodSpin,
        SIGNAL(changed(qreal, qreal)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->hugeErrorSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->hsyncErrorRange,
        SIGNAL(rangeChanged(int, int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->timeTolSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->levelTolSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->geomTolSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVProcessorUiChanged()));

  connect(
        this->ui->brightnessDial,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVBrightnessChanged()));

  connect(
        this->ui->contrastDial,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVContrastChanged()));


  connect(
        this->ui->gammaDial,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVGammaChanged()));

  connect(
        this->ui->rotationDial,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVAspectChanged()));

  connect(
        this->ui->tvZoomSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTVAspectChanged()));

  connect(
        this->ui->flipHorizontalCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVAspectChanged()));


  connect(
        this->ui->flipVerticalCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onTVAspectChanged()));
}

SUFLOAT
TVProcessorTab::getSampleRateFloat(void) const
{
  return static_cast<SUFLOAT>(this->sampleRate);
}

unsigned int
TVProcessorTab::getSampleRate(void) const
{
  return static_cast<unsigned int>(this->sampleRate);
}

void
TVProcessorTab::setSampleRate(qreal sampleRate)
{
  this->sampleRate = sampleRate;
  this->refreshUiState();
  this->emitParameters();
}

void
TVProcessorTab::setDecisionMode(Decider::DecisionMode mode)
{
  this->decisionMode = mode;
}

void
TVProcessorTab::emitParameters(void)
{
  if (this->getSampleRate() > 0) {
    struct sigutils_tv_processor_params params;

    this->refreshUiState();
    this->parseUi(params);

    emit tvProcessorParams(params);
    if (this->tvProcessing)
      emit startTVProcessor();
    this->ui->enableTvButton->setEnabled(true);

    this->ui->tvDisplay->setPicGeometry(
          static_cast<int>(params.line_len),
          static_cast<int>(params.frame_lines));
  } else {
    this->ui->enableTvButton->setEnabled(false);
    if (this->tvProcessing) {
      emit stopTVProcessor();
      this->tvProcessing = false;
      this->ui->enableTvButton->setChecked(false);
    }
  }
}

void
TVProcessorTab::refreshUiState(void)
{
  bool customParamsEnabled = this->ui->customRadio->isChecked();

  if (this->getSampleRate() > 0) {
    this->ui->linePeriodSpin->setSampleRate(this->getSampleRate());
    this->ui->hsyncSpin->setSampleRate(this->getSampleRate());
    this->ui->vsyncSpin->setSampleRate(this->getSampleRate());
  }

  this->ui->geometryGroup->setEnabled(customParamsEnabled);
  this->ui->agcCheck->setEnabled(customParamsEnabled);
  this->ui->syncTab->setEnabled(customParamsEnabled);
  this->ui->invertImageCheck->setEnabled(customParamsEnabled);

  this->ui->nonInterlacedRadio->setEnabled(customParamsEnabled);
  this->ui->fieldOneRadio->setEnabled(customParamsEnabled);
  this->ui->fieldTwoRadio->setEnabled(customParamsEnabled);


  if (!customParamsEnabled)
    this->ui->invertImageCheck->setChecked(
        this->ui->invertSyncCheck->isChecked());

  this->ui->swapCombFilterButton->setEnabled(
        this->ui->combFilterCheck->isChecked());

  this->ui->hsyncSpin->setEnabled(this->ui->enableSyncCheck->isChecked());
  this->ui->vsyncSpin->setEnabled(this->ui->enableSyncCheck->isChecked());

  this->ui->levelTolLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          100 * std::pow(10., this->ui->levelTolSlider->value() / 100.),
          3,
          ""));

  this->ui->timeTolLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          100 * std::pow(10., this->ui->timeTolSlider->value() / 100.),
          3,
          ""));

  this->ui->geomTolLabel->setText(
        SuWidgetsHelpers::formatQuantity(
          100 * std::pow(10., this->ui->geomTolSlider->value() / 100.),
          3,
          ""));

  this->ui->hsyncErrorRangeLabel->setText(
        QString::number(
          EP_INV * std::round(
            EP * 100 * std::pow(
              10.,
              this->ui->hsyncErrorRange->minimumPosition() / 100.)))
        + "% - "
        + QString::number(
            EP_INV * std::round(
              EP * 100 * std::pow(
                10.,
                this->ui->hsyncErrorRange->maximumPosition() / 100.)))
        + "%");

  this->ui->hugeErrorLabel->setText(
        QString::number(
          EP_INV * std::round(
            EP * 100 * std::pow(10., this->ui->hugeErrorSlider->value() / 100.)))
        + "%");

  this->ui->frameRateSpin->setValue(
        1. / (this->ui->linesSpin->value()
              * this->ui->linePeriodSpin->timeValue()));
}

void
TVProcessorTab::refreshUi(
    struct sigutils_tv_processor_params const &params)
{
  bool oldState = this->editingTVProcessorParams;
  this->editingTVProcessorParams = true;

  this->ui->combFilterCheck->setChecked(params.enable_comb);
  this->ui->agcCheck->setChecked(params.enable_agc);
  this->ui->enableSyncCheck->setChecked(params.enable_sync);
  this->ui->invertImageCheck->setChecked(params.reverse);
  this->ui->swapCombFilterButton->setChecked(params.comb_reverse);

  if (!params.interlace) {
    this->ui->nonInterlacedRadio->setChecked(true);
  } else if (params.dominance) {
    this->ui->fieldOneRadio->setChecked(true);
  } else {
    this->ui->fieldTwoRadio->setChecked(true);
  }

  refreshUiState();

  this->ui->linesSpin->setValue(static_cast<int>(params.frame_lines));
  this->ui->hsyncSpin->setSamplesValue(
        static_cast<qreal>(params.hsync_len));
  this->ui->vsyncSpin->setSamplesValue(
        static_cast<qreal>(params.vsync_len));
  this->ui->linePeriodSpin->setSamplesValue(
        static_cast<qreal>(params.line_len));

  this->ui->hsyncSpin->setBestUnits(true);
  this->ui->vsyncSpin->setBestUnits(true);
  this->ui->linePeriodSpin->setBestUnits(true);

  this->ui->timeTolSlider->setValue(
        static_cast<int>(100 * std::log10(params.t_tol)));
  this->ui->levelTolSlider->setValue(
        static_cast<int>(100 * std::log10(params.l_tol)));
  this->ui->geomTolSlider->setValue(
        static_cast<int>(100 * std::log10(params.g_tol)));

  this->ui->hsyncErrorRange->setMinimumPosition(
        static_cast<int>(100 * std::log10(params.hsync_min_err)));
  this->ui->hsyncErrorRange->setMaximumPosition(
        static_cast<int>(100 * std::log10(params.hsync_max_err)));
  this->ui->hugeErrorSlider->setValue(
        static_cast<int>(100 * std::log10(params.hsync_huge_err)));
  this->ui->vsyncTrainLengthSpin->setValue(
        static_cast<int>(params.vsync_odd_trigger));

  this->ui->agcTauSpin->setValue(
        static_cast<qreal>(params.agc_tau));
  this->ui->lineLenTauSpin->setValue(
        static_cast<qreal>(params.line_len_tau));
  this->ui->fastTrackTauSpin->setValue(
        static_cast<qreal>(params.hsync_fast_track_tau));
  this->ui->slowTrackTauSpin->setValue(
        static_cast<qreal>(params.hsync_slow_track_tau));

  this->editingTVProcessorParams = oldState;
}

bool
TVProcessorTab::parseUi(
    struct sigutils_tv_processor_params &params)
{
  params.enable_sync  = this->ui->enableSyncCheck->isChecked();
  params.reverse      = this->ui->invertImageCheck->isChecked();
  params.interlace    = !this->ui->nonInterlacedRadio->isChecked();
  params.dominance    = !this->ui->fieldTwoRadio->isChecked();
  params.enable_agc   = this->ui->agcCheck->isChecked();
  params.frame_lines  = static_cast<SUSCOUNT>(this->ui->linesSpin->value());
  params.enable_comb  = this->ui->combFilterCheck->isChecked();
  params.comb_reverse = this->ui->swapCombFilterButton->isChecked();
  params.x_off        = 0;
  params.hsync_len    = static_cast<SUFLOAT>(
        this->ui->hsyncSpin->samplesValue());
  params.vsync_len    = static_cast<SUFLOAT>(
        this->ui->vsyncSpin->samplesValue());
  params.line_len     = static_cast<SUFLOAT>(
        this->ui->linePeriodSpin->samplesValue());
  params.t_tol        = static_cast<SUFLOAT>(
        std::pow(10., this->ui->timeTolSlider->value() / 100.));
  params.l_tol        = static_cast<SUFLOAT>(
        std::pow(10., this->ui->levelTolSlider->value() / 100.));
  params.g_tol        = static_cast<SUFLOAT>(
        std::pow(10., this->ui->geomTolSlider->value() / 100.));

  params.hsync_huge_err = static_cast<SUFLOAT>(
        std::pow(10., this->ui->hugeErrorSlider->value() / 100.));
  params.hsync_min_err  = static_cast<SUFLOAT>(
        std::pow(10., this->ui->hsyncErrorRange->minimumPosition() / 100.));
  params.hsync_max_err  = static_cast<SUFLOAT>(
        std::pow(10., this->ui->hsyncErrorRange->maximumPosition() / 100.));
  params.vsync_odd_trigger  = static_cast<SUSCOUNT>(
        this->ui->vsyncTrainLengthSpin->value());

  params.hsync_len_tau = 9.5;

  params.line_len_tau  = static_cast<SUFLOAT>(
        this->ui->lineLenTauSpin->value());
  params.hsync_fast_track_tau  = static_cast<SUFLOAT>(
        this->ui->fastTrackTauSpin->value());
  params.hsync_slow_track_tau  = static_cast<SUFLOAT>(
        this->ui->slowTrackTauSpin->value());
  params.agc_tau  = static_cast<SUFLOAT>(
        this->ui->agcTauSpin->value());

  return true;
}

void
TVProcessorTab::feed(const SUCOMPLEX *data, unsigned int size)
{
  SUFLOAT k = this->ui->invertSyncCheck->isChecked() ? -1 : 1;
  SUFLOAT dc = static_cast<SUFLOAT>(this->ui->dcSpin->value()) / 100;

  this->floatBuffer.resize(size);

  if (this->decisionMode == Decider::MODULUS) {
    for (unsigned i = 0; i < size; ++i)
      this->floatBuffer[i] = k * SU_C_ABS(data[i]) + dc;
  } else {
    for (unsigned i = 0; i < size; ++i)
      this->floatBuffer[i] = k * SU_C_ARG(data[i]) / PI + dc;
  }

  this->tvWorker->pushData(this->floatBuffer);

  emit tvProcessorData();
}


////////////////////////////////// Slots ///////////////////////////////////////
void
TVProcessorTab::onTVProcessorUiChanged(void)
{
  if (!this->editingTVProcessorParams) {
    if (this->ui->ntscRadio->isChecked()) {
      struct sigutils_tv_processor_params params;
      su_tv_processor_params_ntsc(&params, this->getSampleRateFloat());
      this->refreshUi(params);
    } else if (this->ui->palRadio->isChecked()) {
      struct sigutils_tv_processor_params params;
      su_tv_processor_params_pal(&params, this->getSampleRateFloat());
      this->refreshUi(params);
    }

    this->emitParameters();
  }

  this->refreshUiState();
}

void
TVProcessorTab::onToggleTVProcessor(void)
{
  this->tvProcessing = this->ui->enableTvButton->isChecked();

  if (this->tvProcessing)
    emit startTVProcessor();
  else
    emit stopTVProcessor();
}

void
TVProcessorTab::onTVProcessorFrame(struct sigutils_tv_frame_buffer *frame)
{
  this->tvWorker->acknowledgeFrame();
  this->ui->tvDisplay->putFrame(frame);
  this->ui->tvDisplay->invalidate();
  emit tvProcessorDisposeFrame(frame);
}

void
TVProcessorTab::onTVProcessorParamsChanged(
    struct sigutils_tv_processor_params params)
{
  this->refreshUi(params);
}

void
TVProcessorTab::onTVProcessorError(QString error)
{
  QMessageBox::critical(this->ui->tvDisplay, "TV Processor error", error);
  this->ui->enableTvButton->setChecked(false);
  this->onToggleTVProcessor();
}

void
TVProcessorTab::onTVContrastChanged(void)
{
  this->ui->tvDisplay->setContrast(this->ui->contrastDial->value() / 100.);
}

void
TVProcessorTab::onTVBrightnessChanged(void)
{
  this->ui->tvDisplay->setBrightness(this->ui->brightnessDial->value() / 100.);
}

void
TVProcessorTab::onTVGammaChanged(void)
{
  qreal gamma = this->ui->gammaDial->value() / 25. + 1;

  if (gamma < 1)
    gamma = -1. / (gamma - 2);

  this->ui->tvDisplay->setGamma(gamma);
}

void
TVProcessorTab::onTVAspectChanged(void)
{
  this->ui->tvDisplay->setRotation(this->ui->rotationDial->value());
  this->ui->tvDisplay->setHorizontalFlip(
        this->ui->flipHorizontalCheck->isChecked());
  this->ui->tvDisplay->setVerticalFlip(
        this->ui->flipVerticalCheck->isChecked());
  this->ui->tvDisplay->setZoom(this->ui->tvZoomSpin->value());
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

    if (!this->ui->tvDisplay->saveToFile(
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
  this->ui->tvDisplay->setAccumulate(this->ui->accumButton->isChecked());
  this->ui->lpfButton->setEnabled(this->ui->accumButton->isChecked());
  this->onEnableLPFChanged();
}

void
TVProcessorTab::onEnableLPFChanged(void)
{
  this->ui->tvDisplay->setEnableSPLPF(this->ui->lpfButton->isChecked());
  this->ui->accumSpinBox->setEnabled(
        this->ui->lpfButton->isChecked() && this->ui->accumButton->isChecked());
  this->onAccumSpinChanged();
}

void
TVProcessorTab::onAccumSpinChanged(void)
{
  this->ui->tvDisplay->setAccumAlpha(
          SU_SPLPF_ALPHA(this->ui->accumSpinBox->value() / 5.f));
}

