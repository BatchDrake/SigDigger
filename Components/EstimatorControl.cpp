//
//    EstimatorControl.cpp: Parameter estimation control
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

#include "EstimatorControl.h"
#include "ui_EstimatorControl.h"

using namespace SigDigger;

EstimatorControl::EstimatorControl(
    QWidget *parent,
    Suscan::Estimator const &estim) :
  QWidget(parent),
  ui(new Ui::EstimatorControl)
{
  ui->setupUi(this);

  this->estimator = estim;
  this->ui->paramLabel->setText(QString::fromStdString(estim.desc));

  connect(
        this->ui->estimateButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onEstimate(void)));

  connect(
        this->ui->applyButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onApply(void)));

  this->updateUI();
}

void
EstimatorControl::updateUI(void)
{
  if (this->available)
    this->ui->paramEdit->setText(
        QString::number(
          static_cast<qreal>(this->paramValue)));
  else
    this->ui->paramEdit->setText("");

  this->ui->applyButton->setEnabled(this->available);
}

void
EstimatorControl::setParameterValue(float param)
{
  this->paramValue = param;

  if (this->available)
    this->updateUI();
}

void
EstimatorControl::setParameterAvailable(bool avail)
{
  if (this->ui->estimateButton->isChecked()) {
    this->available = avail;
    this->updateUI();
  }
}


float
EstimatorControl::getParameterValue(void) const
{
  return this->paramValue;
}

bool
EstimatorControl::isParamAvailable(void) const
{
  return this->available;
}


EstimatorControl::~EstimatorControl()
{
  delete ui;
}

///////////////////////////////////// Slots ////////////////////////////////////
void
EstimatorControl::onEstimate(void)
{
  emit estimatorChanged(
        this->estimator.id,
        this->ui->estimateButton->isChecked());
}

void
EstimatorControl::onApply(void)
{
  emit apply(
        QString::fromStdString(this->estimator.field),
        this->paramValue);
}
