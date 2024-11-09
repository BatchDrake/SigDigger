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

DeviceGain::DeviceGain(QWidget *parent, Suscan::DeviceGainDesc const &desc) :
  QWidget(parent),
  m_ui(new Ui_DeviceGain)
{
  m_ui->setupUi(this);

  m_name = desc.getName();
  m_min  = static_cast<float>(desc.getMin());
  m_max  = static_cast<float>(desc.getMax());
  m_step = static_cast<float>(desc.getStep());

  if (m_step < 1)
    m_step = 1;

  m_defl = static_cast<float>(desc.getDefault());

  // The slider must be configured as follows
  // Min value: 0
  // Max value: (m_max - m_min) / m_step
  // Tick interval: 1
  // Single step: 1

  // Gain to value:
  // value = (gain - m_min) / m_step

  // Value to gain
  // gain = m_min + m_step * value

  m_ui->gainSlider->setMinimum(0);
  m_ui->gainSlider->setMaximum(static_cast<int>((m_max - m_min) / m_step));
  m_ui->gainSlider->setTickInterval(1);
  m_ui->gainSlider->setSingleStep(1);

  m_ui->nameLabel->setText(QString::fromStdString(m_name));

  connect(
        m_ui->gainSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onValueChanged(int)));

  connect(
        m_ui->resetButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onResetClicked(void)));

  setGain(m_defl);
}

float
DeviceGain::getGain() const
{
  int value = m_ui->gainSlider->value();

  return value * m_step + m_min;
}

void
DeviceGain::setGain(float gain)
{
  int value = static_cast<int>((gain - m_min) / m_step);

  m_current = value;

  m_ui->gainSlider->setValue(value);
  m_ui->valueLabel->setText(QString::number(static_cast<qreal>(getGain())) + " dB");
}

void
DeviceGain::onValueChanged(int val)
{
  if (val != m_current) {
    setGain(getGain());
    emit gainChanged(QString::fromStdString(m_name), getGain());
  }
}

void
DeviceGain::onResetClicked(void)
{
  setGain(m_defl);
  emit gainChanged(QString::fromStdString(m_name), getGain());
}

DeviceGain::~DeviceGain()
{
  delete m_ui;
}
