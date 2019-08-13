//
//    MfControl.cpp: Matched filter control
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


#include "MfControl.h"
#include "ui_MfControl.h"
#include <suscan/analyzer/inspector/params.h>

using namespace SigDigger;

MfControl::MfControl(
    QWidget *parent,
    Suscan::Config *config) :
  InspectorCtl(parent, config),
  ui(new Ui::MfControl)
{
  ui->setupUi(this);

  this->refreshUi();

  this->registerWidget(this->ui->mfCombo, SIGNAL(activated(int)));
  this->registerWidget(this->ui->rollOffSlider, SIGNAL(valueChanged(int)));
}

bool
MfControl::applicable(QString const &key)
{
  return strncmp(
        key.toStdString().c_str(),
        "mf.",
        3) == 0;
}

void
MfControl::refreshUi(void)
{
  int index = 0;
  int rollOffInt;

  if (static_cast<int>(this->getUint64("mf.type"))
      == SUSCAN_INSPECTOR_MATCHED_FILTER_MANUAL)
    index = 1;

  rollOffInt = static_cast<int>(this->getFloat("mf.roll-off") * 1000);

  this->ui->mfCombo->setCurrentIndex(index);
  this->ui->rollOffSlider->setValue(rollOffInt);
}

void
MfControl::parseConfig(void)
{
  uint64_t types[] =
  {
    SUSCAN_INSPECTOR_MATCHED_FILTER_BYPASS,
    SUSCAN_INSPECTOR_MATCHED_FILTER_MANUAL
  };

  this->refreshEntry("mf.type", types[this->ui->mfCombo->currentIndex()]);
  this->refreshEntry(
        "mf.roll-off",
        this->ui->rollOffSlider->value() / 1000.);
}

MfControl::~MfControl()
{
  delete ui;
}
