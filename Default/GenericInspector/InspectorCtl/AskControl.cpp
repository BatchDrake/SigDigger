//
//    AskControl.cpp: ASK detection control
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

#include "AskControl.h"
#include "ui_AskControl.h"

using namespace SigDigger;

AskControl::AskControl(QWidget *parent, Suscan::Config *config) :
  InspectorCtl(parent, config),
  ui(new Ui::AskControl)
{
  ui->setupUi(this);

  this->refreshUi();

  this->registerWidget(this->ui->bitsSpin,       SIGNAL(valueChanged(int)));
  this->registerWidget(this->ui->componentCombo, SIGNAL(activated(int)));
  this->registerWidget(this->ui->pllCheck,       SIGNAL(stateChanged(int)));
  this->registerWidget(this->ui->pllCutoffSpin,  SIGNAL(valueChanged(double)));
  this->registerWidget(this->ui->pllCutoffSpin,  SIGNAL(valueChanged(double)));
}

bool
AskControl::applicable(QString const &key)
{
  return strncmp(
        key.toStdString().c_str(),
        "ask.",
        4) == 0;
}


void
AskControl::refreshUi(void)
{
  int bps     = static_cast<int>(this->getUint64("ask.bits-per-symbol"));
  bool usePll = static_cast<int>(this->getUint64("ask.use-pll"));

  this->ui->bitsSpin->setValue(bps);
  this->ui->pllCutoffSpin->setValue(this->getFloat("ask.loop-bw"));
  this->ui->pllOffsetSpin->setValue(this->getFloat("ask.offset"));
  this->ui->componentCombo->setCurrentIndex(
        static_cast<int>(this->getUint64("ask.channel")));
  this->ui->pllCheck->setChecked(usePll);
  this->ui->pllCutoffSpin->setEnabled(usePll);
  this->ui->pllOffsetSpin->setEnabled(usePll);
}

void
AskControl::parseConfig(void)
{
  uint64_t bps = static_cast<uint64_t>(this->ui->bitsSpin->value());

  this->refreshEntry("ask.bits-per-symbol", bps);
  this->refreshEntry("ask.use-pll", this->ui->pllCheck->isChecked());
  this->refreshEntry("ask.loop-bw", this->ui->pllCutoffSpin->value());
  this->refreshEntry("ask.offset", this->ui->pllOffsetSpin->value());
  this->refreshEntry(
        "ask.channel",
        static_cast<uint64_t>(this->ui->componentCombo->currentIndex()));
}

AskControl::~AskControl()
{
  delete ui;
}
