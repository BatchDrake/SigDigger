//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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

using namespace SigDigger;

SamplerDialog::SamplerDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SamplerDialog)
{
  ui->setupUi(this);
  this->connectAll();
}

void
SamplerDialog::connectAll(void)
{
  connect(
        this->ui->buttonBox,
        SIGNAL(clicked(QAbstractButton *)),
        this,
        SLOT(onClose()));

  connect(
        this->ui->bpsSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onBpsChanged(void)));

  connect(
        this->ui->rowSize,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onRowSizeChanged(void)));

  connect(
        this->ui->zoomSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onZoomChanged(void)));

  connect(
        this->ui->histogram,
        SIGNAL(blanked(void)),
        this,
        SIGNAL(resample(void)));

  connect(
        this->ui->horizontalScrollBar,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onHScroll(int)));

  connect(
        this->ui->verticalScrollBar,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onVScroll(int)));

  connect(
        this->ui->symView,
        SIGNAL(zoomChanged(unsigned int)),
        this,
        SLOT(onSymViewZoomChanged(unsigned int)));

  connect(
        this->ui->symView,
        SIGNAL(offsetChanged(unsigned int)),
        this,
        SLOT(onOffsetChanged(unsigned int)));

  connect(
        this->ui->symView,
        SIGNAL(hOffsetChanged(int)),
        this,
        SLOT(onHOffsetChanged(int)));

  connect(
        this->ui->symView,
        SIGNAL(strideChanged(unsigned int)),
        this,
        SLOT(onStrideChanged(unsigned int)));

  connect(
        this->ui->symView,
        SIGNAL(hoverSymbol(unsigned int)),
        this,
        SLOT(onHoverSymbol(unsigned int)));
}

void
SamplerDialog::setProperties(SamplingProperties const &prop)
{
  this->properties = prop;

  switch (prop.space) {
    case AMPLITUDE:
      this->decider.setDecisionMode(Decider::MODULUS);
      this->decider.setMinimum(0);
      this->decider.setMaximum(1);

      this->ui->histogram->overrideDisplayRange(1);
      this->ui->histogram->overrideUnits("");
      this->ui->histogram->overrideDataRange(1);
      break;

    case PHASE:
      this->decider.setDecisionMode(Decider::ARGUMENT);
      this->decider.setMinimum(-M_PI);
      this->decider.setMaximum(M_PI);

      this->ui->histogram->overrideDataRange(2 * M_PI);
      this->ui->histogram->overrideDisplayRange(360);
      this->ui->histogram->overrideUnits("º");
      break;

    case FREQUENCY:
      this->decider.setDecisionMode(Decider::ARGUMENT);
      this->decider.setMinimum(-M_PI);
      this->decider.setMaximum(M_PI);

      this->ui->histogram->overrideDataRange(2 * M_PI);
      this->ui->histogram->overrideDisplayRange(this->properties.fs);
      this->ui->histogram->overrideUnits("Hz");
      break;
  }

  this->ui->histogram->setDecider(&this->decider);
}

void
SamplerDialog::reset(void)
{
  this->ui->symView->clear();
  this->ui->histogram->reset();
}

void
SamplerDialog::setColorConfig(ColorConfig const &cfg)
{
  this->ui->histogram->setForegroundColor(cfg.histogramForeground);
  this->ui->histogram->setBackgroundColor(cfg.histogramBackground);
  this->ui->histogram->setAxesColor(cfg.histogramAxes);
}

void
SamplerDialog::feedSet(WaveSampleSet const &set)
{
  this->ui->histogram->feed(set.block, set.len);
  this->ui->symView->feed(set.symbols, set.len);

  this->refreshHScrollBar();
  this->refreshVScrollBar();
}

WaveSampler *
SamplerDialog::makeSampler(void)
{
  return new WaveSampler(this->properties, &this->decider);
}

unsigned int
SamplerDialog::getVScrollPageSize(void) const
{
  return
      (this->ui->symView->getStride()
       * static_cast<unsigned>(this->ui->symView->height()))
      / this->ui->symView->getZoom();
}

unsigned int
SamplerDialog::getHScrollOffset(void) const
{
  return static_cast<unsigned>(this->ui->horizontalScrollBar->value());
}

void
SamplerDialog::refreshHScrollBar(void) const
{
  unsigned int visible =
      static_cast<unsigned>(this->ui->symView->width()) /
      this->ui->symView->getZoom();

  if (visible < this->ui->symView->getStride()) {
    unsigned int max = this->ui->symView->getStride() - visible;
    this->ui->horizontalScrollBar->setPageStep(static_cast<int>(visible));
    this->ui->horizontalScrollBar->setMaximum(static_cast<int>(max));
    this->ui->horizontalScrollBar->setVisible(true);
  } else {
    this->ui->horizontalScrollBar->setPageStep(static_cast<int>(0));
    this->ui->horizontalScrollBar->setMaximum(static_cast<int>(0));
    this->ui->horizontalScrollBar->setVisible(false);
  }

  if (!this->ui->symView->getAutoStride())
    this->ui->horizontalScrollBar->setEnabled(
          this->ui->symView->getLength() >= visible);
  else
    this->ui->horizontalScrollBar->setEnabled(false);
}

void
SamplerDialog::refreshVScrollBar(void) const
{
  unsigned int pageSize = this->getVScrollPageSize();
  unsigned long lines =
      (this->ui->symView->getLength() + this->ui->symView->getStride() - 1) /
      this->ui->symView->getStride();
  unsigned long max = lines * this->ui->symView->getStride();

  if (max > pageSize) {
    this->ui->verticalScrollBar->setPageStep(static_cast<int>(pageSize));
    this->ui->verticalScrollBar->setMaximum(static_cast<int>(max - pageSize));
    this->ui->verticalScrollBar->setVisible(true);
  } else {
    this->ui->verticalScrollBar->setPageStep(0);
    this->ui->verticalScrollBar->setMaximum(0);
    this->ui->verticalScrollBar->setVisible(false);
  }

  this->ui->verticalScrollBar->setSingleStep(
        static_cast<int>(this->ui->symView->getStride()));

  if (!this->ui->symView->getAutoScroll())
    this->ui->verticalScrollBar->setEnabled(
          this->ui->symView->getLength() >= pageSize);
  else
    this->ui->verticalScrollBar->setEnabled(false);
}


SamplerDialog::~SamplerDialog()
{
  delete ui;
}

/////////////////////////////// Slots //////////////////////////////////////////
void
SamplerDialog::onClose(void)
{

}

void
SamplerDialog::onBpsChanged(void)
{
  unsigned int bps = static_cast<unsigned>(this->ui->bpsSpin->value());

  this->decider.setBps(bps);
  this->ui->histogram->setOrderHint(bps);
  this->ui->symView->setBitsPerSymbol(bps);

  emit resample();
}

void
SamplerDialog::onZoomChanged(void)
{
  this->ui->symView->setZoom(this->ui->zoomSpin->value());
}

void
SamplerDialog::onRowSizeChanged(void)
{
  this->ui->symView->setAutoStride(false);
  this->ui->symView->setStride(this->ui->rowSize->value());
}

void
SamplerDialog::onVScroll(int offset)
{
  int relStart = this->ui->symView->getOffset() % this->ui->symView->getStride();
  int alignedOffset = this->ui->symView->getStride() * (
        offset / this->ui->symView->getStride());

  this->scrolling = true;

  this->ui->symView->setOffset(
        static_cast<unsigned int>(alignedOffset + relStart));

  this->scrolling = false;
}

void
SamplerDialog::onHScroll(int offset)
{
  this->scrolling = true;

  this->ui->symView->setHOffset(offset);
  this->scrolling = false;
}

void
SamplerDialog::onOffsetChanged(unsigned int offset)
{
  if (!this->scrolling)
    this->ui->verticalScrollBar->setValue(static_cast<int>(offset));
}

void
SamplerDialog::onHOffsetChanged(int offset)
{
  if (!this->scrolling)
    this->ui->horizontalScrollBar->setValue(offset);
}

void
SamplerDialog::onStrideChanged(unsigned int stride)
{
  this->ui->rowSize->setValue(static_cast<int>(stride));
  this->refreshHScrollBar();
}


void
SamplerDialog::onSymViewZoomChanged(unsigned int zoom)
{
  this->ui->zoomSpin->setValue(static_cast<int>(zoom));
  this->refreshVScrollBar();
  this->refreshHScrollBar();
}

void
SamplerDialog::onHoverSymbol(unsigned int index)
{
  this->ui->positionLabel->setText(
        "Position: " + QString::number(index));
}
