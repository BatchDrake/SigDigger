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

  QFontMetrics metrics(this->ui->spanLabel->font());

  this->ui->spanLabel->setFixedWidth(
        metrics.width("XXXXXXXXXXXXX (XXXXXXX seconds)"));

  this->ui->selRangeLabel->setFixedWidth(
        metrics.width("XXXXXXXXXXXXX to XXXXXXXXXXXXX"));

  this->ui->histogram->setDecider(&this->dummyDecider);
  this->ui->histogram->setUpdateDecider(false);
  this->ui->histogram->setDrawThreshold(false);

  this->setWindowFlags(
        this->windowFlags() | Qt::Window | Qt::WindowMaximizeButtonHint);

  this->connectAll();

  this->reset();
}

void
HistogramDialog::connectAll(void)
{
  connect(
        this->ui->buttonBox,
        SIGNAL(clicked(QAbstractButton *)),
        this,
        SLOT(onClose(void)));

  connect(
        this->ui->histogram,
        SIGNAL(newLimits(float, float)),
        this,
        SLOT(onNewLimits(float, float)));

  connect(
        this->ui->histogram,
        SIGNAL(resetLimits(void)),
        this,
        SLOT(onResetLimits(void)));

  connect(
        this->ui->histogram,
        SIGNAL(blanked()),
        this,
        SIGNAL(blanked()));
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
      this->ui->histogram->overrideDisplayRange(1);
      this->ui->histogram->overrideUnits("");
      this->ui->histogram->overrideDataRange(1);

      this->ui->rangeLabel->setText(
            SuWidgetsHelpers::formatQuantity(
              this->dummyDecider.getMinimum(),
              "")
            + " to "
            + SuWidgetsHelpers::formatQuantity(
              this->dummyDecider.getMaximum(),
              ""));
      break;

    case PHASE:
      this->ui->spaceLabel->setText("Phase");
      this->ui->histogram->overrideDataRange(2 * M_PI);
      this->ui->histogram->overrideDisplayRange(360);
      this->ui->histogram->overrideUnits("º");

      this->ui->rangeLabel->setText(
            QString::number(this->dummyDecider.getMinimum() / M_PI * 180)
            + "º to "
            + QString::number(this->dummyDecider.getMaximum() / M_PI * 180)
            + "º");
      break;

    case FREQUENCY:
      this->ui->spaceLabel->setText("Frequency");
      this->ui->histogram->overrideDataRange(2 * M_PI);
      this->ui->histogram->overrideDisplayRange(this->properties.fs);
      this->ui->histogram->overrideUnits("Hz");

      this->ui->rangeLabel->setText(
            SuWidgetsHelpers::formatQuantityNearest(
              .5 * this->dummyDecider.getMinimum() / M_PI * this->properties.fs,
              2,
              "Hz")
            + " to "
            + SuWidgetsHelpers::formatQuantityNearest(
              .5 * this->dummyDecider.getMaximum() / M_PI * this->properties.fs,
              2,
              "Hz"));
      break;
  }

  if (this->limits) {
    float min, max;
    QString units;

    switch (this->properties.space) {
      case AMPLITUDE:
        min = this->selMin;
        max = this->selMax;
        units = "";
        break;

      case PHASE:
        min = this->selMin / M_PI * 180;
        max = this->selMax / M_PI * 180;
        units = "º";
        break;

      case FREQUENCY:
        min = .5 * this->selMin / M_PI * this->properties.fs;
        max = .5 * this->selMax / M_PI * this->properties.fs;
        units = "Hz";
        break;
    }

    this->ui->selRangeLabel->setText(
          SuWidgetsHelpers::formatQuantityNearest(
            min,
            2,
            units)
          + " to "
          + SuWidgetsHelpers::formatQuantityNearest(
            max,
            2,
            units));

    this->ui->selRangeWidth->setText(
          SuWidgetsHelpers::formatQuantityNearest(
            max - min,
            2,
            units));

  } else {
    this->ui->selRangeLabel->setText("N/A");
    this->ui->selRangeWidth->setText("N/A");
  }
}

void
HistogramDialog::setProperties(SamplingProperties const &prop)
{
  this->properties = prop;

  if (prop.space == SamplingSpace::AMPLITUDE) {
    this->dummyDecider.setMinimum(0);
    this->dummyDecider.setMaximum(1);
    this->dummyDecider.setDecisionMode(Decider::MODULUS);
  } else {
    this->dummyDecider.setMinimum(-PI);
    this->dummyDecider.setMaximum(PI);
    this->dummyDecider.setDecisionMode(Decider::ARGUMENT);
  }

  this->refreshUi();
}


void
HistogramDialog::setColorConfig(ColorConfig const &colors)
{
  this->ui->histogram->setBackgroundColor(colors.histogramBackground);
  this->ui->histogram->setForegroundColor(colors.histogramForeground);
  this->ui->histogram->setAxesColor(colors.histogramAxes);
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
    this->dummyDecider.setMinimum(this->min);
    this->dummyDecider.setMaximum(this->max);
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

void
HistogramDialog::onNewLimits(float min, float max)
{
  this->limits = true;
  this->selMin = min;
  this->selMax = max;

  this->refreshUi();
}

void
HistogramDialog::onResetLimits(void)
{
  this->limits = false;

  this->refreshUi();
}

