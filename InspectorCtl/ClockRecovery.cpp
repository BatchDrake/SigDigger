//
//    ClockRecovery.cpp: Configure clock recovery
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

#include "ClockRecovery.h"
#include "ui_ClockRecovery.h"
#include <analyzer/inspector/params.h>

using namespace SigDigger;

ClockRecovery::ClockRecovery(QWidget *parent, Suscan::Config *config) :
  InspectorCtl(parent, config),
  ui(new Ui::ClockRecovery)
{
  ui->setupUi(this);

  this->refreshUi();

  this->registerWidget(this->ui->baudRateSpin, SIGNAL(valueChanged(double)));
  this->registerWidget(this->ui->startButton, SIGNAL(clicked(bool)));
  this->registerWidget(this->ui->typeCombo, SIGNAL(activated(int)));
  this->registerWidget(this->ui->gainSpin, SIGNAL(valueChanged(double)));
  this->registerWidget(this->ui->phaseSlider, SIGNAL(valueChanged(int)));

  this->ui->baudRateSpin->setUnits("baud");
}

bool
ClockRecovery::applicable(QString const &key)
{
  return strncmp(
        key.toStdString().c_str(),
        "clock.",
        6) == 0;
}

void
ClockRecovery::refreshUi(void)
{
  this->ui->baudRateSpin->setValue(this->getFloat("clock.baud"));
  this->ui->startButton->setChecked(this->getBoolean("clock.running"));
  this->ui->gainSpin->setValue(this->getFloat("clock.gain"));
  this->ui->phaseSlider->setValue(
        static_cast<int>(this->getFloat("clock.phase") * 100.));

  switch (this->getUint64("clock.type")) {
    case SUSCAN_INSPECTOR_BAUDRATE_CONTROL_MANUAL:
      this->ui->typeCombo->setCurrentIndex(0);
      this->ui->phaseSlider->setEnabled(true);
      this->ui->gainSpin->setEnabled(false);
      break;

    case SUSCAN_INSPECTOR_BAUDRATE_CONTROL_GARDNER:
      this->ui->typeCombo->setCurrentIndex(1);
      this->ui->phaseSlider->setEnabled(false);
      this->ui->gainSpin->setEnabled(true);
      break;
  }
}

void
ClockRecovery::parseConfig(void)
{
  uint64_t recTypes[] =
  {
    SUSCAN_INSPECTOR_BAUDRATE_CONTROL_MANUAL,
    SUSCAN_INSPECTOR_BAUDRATE_CONTROL_GARDNER
  };

  this->refreshEntry("clock.baud", this->ui->baudRateSpin->value());
  this->refreshEntry("clock.running", this->ui->startButton->isChecked());
  this->refreshEntry("clock.gain", this->ui->gainSpin->value());
  this->refreshEntry("clock.phase", this->ui->phaseSlider->value() / 100.);
  this->refreshEntry("clock.type", recTypes[this->ui->typeCombo->currentIndex()]);
}

ClockRecovery::~ClockRecovery()
{
  delete ui;
}
