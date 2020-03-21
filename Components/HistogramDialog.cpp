//
//    HistogramDialog.cpp: Histogram dialog
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
#include "include/HistogramDialog.h"
#include "ui_HistogramDialog.h"

#include <SuWidgets/SuWidgetsHelpers.h>

using namespace SigDigger;

HistogramDialog::HistogramDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::HistogramDialog)
{
  this->ui->setupUi(this);

  connect(
        this->ui->buttonBox,
        SIGNAL(clicked(QAbstractButton *)),
        this,
        SLOT(onClose(void)));

  this->ui->histogram->setDecider(&this->dummyDecider);
  this->reset();
}

void
HistogramDialog::reset(void)
{
  this->min = INFINITY;
  this->max = -INFINITY;

  this->ui->histogram->reset();
}

void
HistogramDialog::refreshUi(void)
{
  this->ui->spanLabel->setText(
        QString::number(this->properties.length)
        + " samples ("
        + SuWidgetsHelpers::formatQuantity(
          this->properties.length / this->properties.fs,
          "s")
        + ")");

  switch (this->properties.space) {
    case AMPLITUDE:
      this->ui->spaceLabel->setText("Amplitude");
      this->ui->rangeLabel->setText(
            SuWidgetsHelpers::formatQuantity(
              this->dummyDecider.getMinAngle(),
              "")
            + " to "
            + SuWidgetsHelpers::formatQuantity(
              this->dummyDecider.getMaxAngle(),
              ""));
      break;

    case PHASE:
      this->ui->spaceLabel->setText("Phase");
      this->ui->rangeLabel->setText(
            QString::number(this->dummyDecider.getMinAngle() / M_PI * 180)
            + "º to "
            + QString::number(this->dummyDecider.getMaxAngle() / M_PI * 180)
            + "º");
      break;

    case FREQUENCY:
      this->ui->spaceLabel->setText("Frequency");
      this->ui->rangeLabel->setText(
            SuWidgetsHelpers::formatQuantityNearest(
              .5 * this->dummyDecider.getMinAngle() / M_PI * this->properties.fs,
              2,
              "Hz")
            + " to "
            + SuWidgetsHelpers::formatQuantityNearest(
              .5 * this->dummyDecider.getMaxAngle() / M_PI * this->properties.fs,
              2,
              "Hz"));
      break;
  }
}

void
HistogramDialog::setProperties(SamplingProperties const &prop)
{
  this->properties = prop;

  if (prop.space != SamplingSpace::AMPLITUDE) {
    this->dummyDecider.setMinAngle(-M_PI);
    this->dummyDecider.setMaxAngle(M_PI);
  }

  this->refreshUi();
}

void
HistogramDialog::feed(const SUFLOAT *data, unsigned int len)
{
  bool adjustDecider = false;

  // FIXME: Decision adjustments should be better
  if (this->properties.space == SamplingSpace::AMPLITUDE) {
    for (unsigned int i = 0; i < len; ++i) {
      if (data[i] < min) {
        min = data[i];
        adjustDecider = true;
      }

      if (data[i] > max) {
        max = data[i];
        adjustDecider = true;
      }
    }
  }

  if (adjustDecider) {
    this->dummyDecider.setMinAngle(this->min);
    this->dummyDecider.setMaxAngle(this->max);
    this->ui->histogram->reset();
    this->refreshUi();
  }

  this->ui->histogram->feed(data, len);
}

HistogramDialog::~HistogramDialog()
{
  delete ui;
}

void
HistogramDialog::onClose(void)
{
  this->hide();
}
