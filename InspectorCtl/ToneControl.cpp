//
//    ToneControl.cpp: Matched filter control
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



#include "ToneControl.h"
#include "ui_ToneControl.h"

using namespace SigDigger;


ToneControl::ToneControl(
    QWidget *parent,
    Suscan::Config *config) :
  InspectorCtl(parent, config),
  ui(new Ui::ToneControl)
{
  ui->setupUi(this);

  this->refreshUi();

  this->registerWidget(this->ui->toneSpin,       SIGNAL(valueChanged(int)));
  this->registerWidget(this->ui->phaseSlider,    SIGNAL(valueChanged(int)));
  this->registerWidget(this->ui->quadDemodCheck, SIGNAL(stateChanged(int)));
}

bool
ToneControl::applicable(QString const &key)
{
  return strncmp(
        key.toStdString().c_str(),
        "fsk.",
        4) == 0;
}

void
ToneControl::refreshUi(void)
{
  int bits;
  int phaseInt;
  bool quadDemod;

  bits = static_cast<int>(this->getUint64("fsk.bits-per-symbol"));
  phaseInt = static_cast<int>(this->getFloat("fsk.phase") * 1000 / (2 * M_PI));
  quadDemod = this->getBoolean("fsk.quad-demod");

  this->ui->toneSpin->setValue(bits);
  this->ui->phaseSlider->setValue(phaseInt);
  this->ui->quadDemodCheck->setChecked(quadDemod);
}

void
ToneControl::parseConfig(void)
{
  this->refreshEntry(
        "fsk.bits-per-symbol",
        static_cast<uint64_t>(this->ui->toneSpin->value()));

  this->refreshEntry(
        "fsk.phase",
        2 * M_PI * this->ui->phaseSlider->value() / 1000.);

  this->refreshEntry(
        "fsk.quad-demod",
        this->ui->quadDemodCheck->isChecked());
}

ToneControl::~ToneControl()
{
  delete ui;
}
