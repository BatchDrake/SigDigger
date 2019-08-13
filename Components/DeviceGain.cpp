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
#include "DeviceGain.h"
#include "ui_DeviceGain.h"

using namespace SigDigger;

DeviceGain::DeviceGain(
    QWidget *parent,
    Suscan::Source::GainDescription const &desc) :
  QWidget(parent),
  ui(new Ui_DeviceGain)
{
  this->ui->setupUi(this);

  this->name = desc.getName();
  this->min  = static_cast<int>(desc.getMin());
  this->max  = static_cast<int>(desc.getMax());
  this->step = static_cast<int>(desc.getStep());

  if (this->step < 1)
    this->step = 1;

  this->defl = static_cast<int>(desc.getDefault());

  this->ui->gainSlider->setMinimum(this->min);
  this->ui->gainSlider->setMaximum(this->max);
  this->ui->gainSlider->setTickInterval(this->step);
  this->ui->gainSlider->setSingleStep(this->step);

  this->ui->nameLabel->setText(QString::fromStdString(this->name));

  connect(
        this->ui->gainSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onValueChanged(int)));

  connect(
        this->ui->resetButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onResetClicked(void)));

  this->setGain(this->defl);
}

void
DeviceGain::setGain(float val)
{
  float adjusted;

  if (val < this->min)
    val = this->min;
  else if (val > this->max)
    val = this->max;

  adjusted = floor((val - this->min) / this->step) + this->min;

  this->current = static_cast<int>(adjusted);

  this->ui->gainSlider->setValue(this->current);
  this->ui->valueLabel->setText(QString::number(this->current) + " dB");
}

void
DeviceGain::onValueChanged(int val)
{
  int old = this->current;
  this->setGain(val);

  if (old != this->current)
    emit gainChanged(QString::fromStdString(this->name), this->current);
}

void
DeviceGain::onResetClicked(void)
{
  this->setGain(this->defl);
  emit gainChanged(QString::fromStdString(this->name), this->current);
}

DeviceGain::~DeviceGain()
{
  delete ui;
}
