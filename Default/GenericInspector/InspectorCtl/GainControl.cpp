//
//    GainControl.cpp: Inspector gain control
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

#include "GainControl.h"
#include "ui_GainControl.h"

using namespace SigDigger;

GainControl::GainControl(
    QWidget *parent,
    Suscan::Config *config) :
  InspectorCtl(parent, config),
  ui(new Ui::GainControl)
{
  ui->setupUi(this);

  this->refreshUi();

  this->registerWidget(this->ui->autoPushButton, SIGNAL(clicked(bool)));
  this->registerWidget(this->ui->gainSpin, SIGNAL(valueChanged(double)));
}

bool
GainControl::applicable(QString const &key)
{
  return strncmp(
        key.toStdString().c_str(),
        "agc.",
        4) == 0;
}

void
GainControl::refreshUi(void)
{
  this->ui->autoPushButton->setChecked(this->getBoolean("agc.enabled"));
  this->ui->gainSpin->setValue(this->getFloat("agc.gain"));
  this->ui->gainSpin->setEnabled(!this->getBoolean("agc.enabled"));
}

void
GainControl::parseConfig(void)
{
  this->refreshEntry("agc.enabled", this->ui->autoPushButton->isChecked());
  this->refreshEntry("agc.gain", this->ui->gainSpin->value());
}

GainControl::~GainControl()
{
  delete ui;
}
