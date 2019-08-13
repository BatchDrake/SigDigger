//
//    AfcControl.cpp: Automatic frequency control
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


#include "AfcControl.h"
#include "ui_AfcControl.h"

using namespace SigDigger;

AfcControl::AfcControl(
    QWidget *parent,
    Suscan::Config *config) :
  InspectorCtl(parent, config),
  ui(new Ui::AfcControl)
{
  ui->setupUi(this);

  this->refreshUi();

  this->registerWidget(this->ui->typeCombo,  SIGNAL(activated(int)));
  this->registerWidget(this->ui->orderCombo, SIGNAL(activated(int)));
  this->registerWidget(this->ui->offsetSpin, SIGNAL(valueChanged(double)));
  this->registerWidget(this->ui->bwSpin,     SIGNAL(valueChanged(double)));
}

bool
AfcControl::applicable(QString const &key)
{
  return strncmp(
        key.toStdString().c_str(),
        "afc.",
        4) == 0;
}

void
AfcControl::refreshUi(void)
{
  int order = static_cast<int>(this->getUint64("afc.costas-order"));
  int bps   = static_cast<int>(this->getUint64("afc.bits-per-symbol"));

  this->ui->typeCombo->setCurrentIndex(order);

  if (bps > 0)
    this->ui->orderCombo->setCurrentIndex(bps - 1);
  this->ui->bwSpin->setValue(this->getFloat("afc.loop-bw"));
  this->ui->offsetSpin->setValue(this->getFloat("afc.offset"));

  this->ui->orderCombo->setEnabled(order == 0);
  this->ui->offsetSpin->setEnabled(order == 0);
  this->ui->bwSpin->setEnabled(order != 0);
}

void
AfcControl::parseConfig(void)
{
  uint64_t order = static_cast<uint64_t>(this->ui->typeCombo->currentIndex());
  uint64_t bps   = static_cast<uint64_t>(this->ui->orderCombo->currentIndex());
  bps += 1;

  this->refreshEntry("afc.costas-order", order);
  this->refreshEntry("afc.bits-per-symbol", order == 0 ? bps : order);
  this->refreshEntry(
        "afc.loop-bw",
        this->ui->bwSpin->value());
  this->refreshEntry(
        "afc.offset",
        this->ui->offsetSpin->value());
}

AfcControl::~AfcControl()
{
  delete ui;
}
