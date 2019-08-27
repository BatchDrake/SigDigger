//
//    DeviceGain.cpp: Device gain control
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
  this->min  = static_cast<float>(desc.getMin());
  this->max  = static_cast<float>(desc.getMax());
  this->step = static_cast<float>(desc.getStep());

  if (this->step < 1)
    this->step = 1;

  this->defl = static_cast<float>(desc.getDefault());

  // The slider must be configured as follows
  // Min value: 0
  // Max value: (this->max - this->min) / this->step
  // Tick interval: 1
  // Single step: 1

  // Gain to value:
  // value = (gain - this->min) / this->step

  // Value to gain
  // gain = this->min + this->step * value

  this->ui->gainSlider->setMinimum(0);
  this->ui->gainSlider->setMaximum(static_cast<int>((this->max - this->min) / this->step));
  this->ui->gainSlider->setTickInterval(1);
  this->ui->gainSlider->setSingleStep(1);

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

float
DeviceGain::getGain(void) const
{
  int value = this->ui->gainSlider->value();

  return value * this->step + this->min;
}

void
DeviceGain::setGain(float gain)
{
  int value = static_cast<int>((gain - this->min) / this->step);

  this->current = value;

  this->ui->gainSlider->setValue(value);
  this->ui->valueLabel->setText(QString::number(static_cast<qreal>(this->getGain())) + " dB");
}

void
DeviceGain::onValueChanged(int val)
{
  if (val != this->current) {
    this->setGain(this->getGain());
    emit gainChanged(QString::fromStdString(this->name), this->current);
  }
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
