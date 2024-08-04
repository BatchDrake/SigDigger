//
//    SamplerDialog.cpp: Sampler dialog implementation
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

#include "SamplerDialog.h"
#include "ui_SamplerDialog.h"

#include <SuWidgetsHelpers.h>
#include <QMessageBox>
#include <QFileDialog>

using namespace SigDigger;

SamplerDialog::SamplerDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SamplerDialog)
{
  ui->setupUi(this);

  setWindowFlags(
        windowFlags() | Qt::Window | Qt::WindowMaximizeButtonHint);
  setModal(true);

  connectAll();
}

void
SamplerDialog::connectAll(void)
{
  connect(
        ui->buttonBox,
        SIGNAL(clicked(QAbstractButton *)),
        this,
        SLOT(onClose()));

  connect(
        ui->bpsSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onBpsChanged(void)));

  connect(
        ui->rowSize,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onRowSizeChanged(void)));

  connect(
        ui->zoomSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onZoomChanged(void)));

  connect(
        ui->histogram,
        SIGNAL(blanked(void)),
        this,
        SIGNAL(resample(void)));

  connect(
        ui->horizontalScrollBar,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onHScroll(int)));

  connect(
        ui->verticalScrollBar,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onVScroll(int)));

  connect(
        ui->symView,
        SIGNAL(zoomChanged(unsigned int)),
        this,
        SLOT(onSymViewZoomChanged(unsigned int)));

  connect(
        ui->symView,
        SIGNAL(offsetChanged(unsigned int)),
        this,
        SLOT(onOffsetChanged(unsigned int)));

  connect(
        ui->symView,
        SIGNAL(hOffsetChanged(int)),
        this,
        SLOT(onHOffsetChanged(int)));

  connect(
        ui->symView,
        SIGNAL(strideChanged(unsigned int)),
        this,
        SLOT(onStrideChanged(unsigned int)));

  connect(
        ui->symView,
        SIGNAL(hoverSymbol(unsigned int)),
        this,
        SLOT(onHoverSymbol(unsigned int)));

  connect(
        ui->saveButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveSymView()));
}

void
SamplerDialog::setAmplitudeLimits(SUFLOAT min, SUFLOAT max)
{
  m_minAmp = min;
  m_maxAmp = max;
}

void
SamplerDialog::setProperties(SamplingProperties const &prop)
{
  m_properties = prop;

  switch (prop.space) {
    case AMPLITUDE:
      m_decider.setDecisionMode(Decider::MODULUS);
      m_decider.setMinimum(m_minAmp);
      m_decider.setMaximum(m_maxAmp);

      ui->histogram->overrideDisplayRange(std::fmax(m_maxAmp, m_minAmp));
      ui->histogram->overrideUnits("");
      ui->histogram->overrideDataRange(std::fmax(m_maxAmp, m_minAmp));
      break;

    case PHASE:
      m_decider.setDecisionMode(Decider::ARGUMENT);
      m_decider.setMinimum(-PI);
      m_decider.setMaximum(PI);

      ui->histogram->overrideDataRange(2 * M_PI);
      ui->histogram->overrideDisplayRange(360);
      ui->histogram->overrideUnits("º");
      break;

    case FREQUENCY:
      m_decider.setDecisionMode(Decider::ARGUMENT);
      m_decider.setMinimum(-PI);
      m_decider.setMaximum(PI);

      ui->histogram->overrideDataRange(2 * M_PI);
      ui->histogram->overrideDisplayRange(m_properties.fs);
      ui->histogram->overrideUnits("Hz");
      break;
  }

  ui->histogram->setDecider(&m_decider);
}

void
SamplerDialog::reset(void)
{
  ui->symView->clear();
  ui->histogram->reset();

  m_minVal = +INFINITY;
  maxVal = -INFINITY;
}

void
SamplerDialog::setColorConfig(ColorConfig const &cfg)
{
  ui->histogram->setForegroundColor(cfg.histogramForeground);
  ui->histogram->setBackgroundColor(cfg.histogramBackground);
  ui->histogram->setAxesColor(cfg.histogramAxes);

  ui->symView->setBackgroundColor(cfg.symViewBackground);
  ui->symView->setLoColor(cfg.symViewLow);
  ui->symView->setHiColor(cfg.symViewHigh);
}

void
SamplerDialog::fitToSamples(void)
{
  if (isfinite(m_minVal) && isfinite(maxVal)) {
    m_decider.setMinimum(m_minVal);
    m_decider.setMaximum(maxVal);

    ui->histogram->setDecider(&m_decider);
  }
}

void
SamplerDialog::feedSet(WaveSampleSet const &set)
{
  if (m_decider.getDecisionMode() == Decider::MODULUS) {
    for (SUSCOUNT i = 0; i < set.len; ++i) {
      SUFLOAT amp = SU_C_ABS(set.block[i]);
      if (amp > maxVal)
        maxVal = SU_C_ABS(set.block[i]);
      if (amp < m_minVal)
        m_minVal = SU_C_ABS(set.block[i]);
    }
  } else {
    for (SUSCOUNT i = 0; i < set.len; ++i) {
      SUFLOAT arg = SU_C_ARG(set.block[i]);
      if (arg > maxVal)
        maxVal = SU_C_ARG(set.block[i]);
      if (arg < m_minVal)
        m_minVal = SU_C_ARG(set.block[i]);
    }
  }

  ui->histogram->feed(set.block, set.len);
  ui->symView->feed(set.symbols, set.len);

  refreshHScrollBar();
  refreshVScrollBar();
}

WaveSampler *
SamplerDialog::makeSampler(void)
{
  return new WaveSampler(m_properties, &m_decider);
}

unsigned int
SamplerDialog::getVScrollPageSize(void) const
{
  return
      (ui->symView->getStride()
       * static_cast<unsigned>(ui->symView->height()))
      / ui->symView->getZoom();
}

unsigned int
SamplerDialog::getHScrollOffset(void) const
{
  return static_cast<unsigned>(ui->horizontalScrollBar->value());
}

void
SamplerDialog::refreshHScrollBar(void) const
{
  unsigned int visible =
      static_cast<unsigned>(ui->symView->width()) /
      ui->symView->getZoom();

  if (visible < ui->symView->getStride()) {
    unsigned int max = ui->symView->getStride() - visible;
    ui->horizontalScrollBar->setPageStep(static_cast<int>(visible));
    ui->horizontalScrollBar->setMaximum(static_cast<int>(max));
    ui->horizontalScrollBar->setVisible(true);
  } else {
    ui->horizontalScrollBar->setPageStep(static_cast<int>(0));
    ui->horizontalScrollBar->setMaximum(static_cast<int>(0));
    ui->horizontalScrollBar->setVisible(false);
  }

  if (!ui->symView->getAutoStride())
    ui->horizontalScrollBar->setEnabled(
          ui->symView->getLength() >= visible);
  else
    ui->horizontalScrollBar->setEnabled(false);
}

void
SamplerDialog::refreshVScrollBar(void) const
{
  unsigned int pageSize = getVScrollPageSize();
  unsigned long lines =
      (ui->symView->getLength() + ui->symView->getStride() - 1) /
      ui->symView->getStride();
  unsigned long max = lines * ui->symView->getStride();

  if (max > pageSize) {
    ui->verticalScrollBar->setPageStep(static_cast<int>(pageSize));
    ui->verticalScrollBar->setMaximum(static_cast<int>(max - pageSize));
    ui->verticalScrollBar->setVisible(true);
  } else {
    ui->verticalScrollBar->setPageStep(0);
    ui->verticalScrollBar->setMaximum(0);
    ui->verticalScrollBar->setVisible(false);
  }

  ui->verticalScrollBar->setSingleStep(
        static_cast<int>(ui->symView->getStride()));

  if (!ui->symView->getAutoScroll())
    ui->verticalScrollBar->setEnabled(
          ui->symView->getLength() >= pageSize);
  else
    ui->verticalScrollBar->setEnabled(false);
}


void
SamplerDialog::closeEvent(QCloseEvent *)
{
  emit stopTask();
}

SamplerDialog::~SamplerDialog()
{
  delete ui;
}

/////////////////////////////// Slots //////////////////////////////////////////
void
SamplerDialog::onClose(void)
{
  emit stopTask();
  hide();
}

void
SamplerDialog::onBpsChanged(void)
{
  unsigned int bps = static_cast<unsigned>(ui->bpsSpin->value());

  m_decider.setBps(bps);
  ui->histogram->setOrderHint(bps);
  ui->symView->setBitsPerSymbol(bps);

  emit resample();
}

void
SamplerDialog::onZoomChanged(void)
{
  ui->symView->setZoom(ui->zoomSpin->value());
}

void
SamplerDialog::onRowSizeChanged(void)
{
  ui->symView->setAutoStride(false);
  ui->symView->setStride(ui->rowSize->value());
}

void
SamplerDialog::onVScroll(int offset)
{
  int relStart = ui->symView->getOffset() % ui->symView->getStride();
  int alignedOffset = ui->symView->getStride() * (
        offset / ui->symView->getStride());

  m_scrolling = true;

  ui->symView->setOffset(
        static_cast<unsigned int>(alignedOffset + relStart));

  m_scrolling = false;
}

void
SamplerDialog::onHScroll(int offset)
{
  m_scrolling = true;
  ui->symView->setHOffset(offset);
  m_scrolling = false;
}

void
SamplerDialog::onOffsetChanged(unsigned int offset)
{
  if (!m_scrolling) {
    refreshVScrollBar();
    ui->verticalScrollBar->setValue(static_cast<int>(offset));
  }
}

void
SamplerDialog::onHOffsetChanged(int offset)
{
  if (!m_scrolling)
    ui->horizontalScrollBar->setValue(offset);
}

void
SamplerDialog::onStrideChanged(unsigned int stride)
{
  ui->rowSize->setValue(static_cast<int>(stride));
  refreshHScrollBar();
}


void
SamplerDialog::onSymViewZoomChanged(unsigned int zoom)
{
  ui->zoomSpin->setValue(static_cast<int>(zoom));
  refreshVScrollBar();
  refreshHScrollBar();
}

void
SamplerDialog::onHoverSymbol(unsigned int index)
{
  ui->positionLabel->setText(
        "Position: " + QString::number(index));
}

void
SamplerDialog::onSaveSymView(void)
{
  QFileDialog dialog(ui->symView);
  QStringList filters;
  enum SymView::FileFormat fmt = SymView::FILE_FORMAT_TEXT;

  filters << "Text file (*.txt)"
          << "Binary file (*.bin)"
          << "C source file (*.c)"
          << "Microsoft Windows Bitmap (*.bmp)"
          << "PNG Image (*.png)"
          << "JPEG Image (*.jpg)"
          << "Portable Pixel Map (*.ppm)";

  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setWindowTitle(QString("Save current symbol capture as..."));
  dialog.setNameFilters(filters);

  if (dialog.exec()) {
    // This sucks
    QString filter = dialog.selectedNameFilter();
    QString path = dialog.selectedFiles().first();
    QFileInfo fi(path);
    QString ext = fi.size() > 0
        ? fi.suffix()
        : SuWidgetsHelpers::extractFilterExtension(filter);

    if (ext == "txt")
      fmt = SymView::FILE_FORMAT_TEXT;
    else if (ext == "bin")
      fmt = SymView::FILE_FORMAT_RAW;
    else if (ext == "c" || ext == "h" || ext == "cpp")
      fmt = SymView::FILE_FORMAT_C_ARRAY;
    else if (ext == "bmp")
      fmt = SymView::FILE_FORMAT_BMP;
    else if (ext == "png")
      fmt = SymView::FILE_FORMAT_PNG;
    else if (ext == "jpg" || ext == "jpeg")
      fmt = SymView::FILE_FORMAT_JPEG;
    else if (ext == "ppm")
      fmt = SymView::FILE_FORMAT_PPM;

    try {
      ui->symView->save(
            SuWidgetsHelpers::ensureExtension(path, ext),
            fmt);
    } catch (std::ios_base::failure const &) {
      (void) QMessageBox::critical(
            ui->symView,
            "Save symbol file",
            "Failed to save file in the specified location. Please try again.",
            QMessageBox::Close);
    }
  }
}
