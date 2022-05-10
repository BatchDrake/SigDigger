//
//    EqualizerControl.cpp: Configure equalizer
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


#include "EqualizerControl.h"
#include "ui_EqualizerControl.h"
#include <analyzer/inspector/params.h>

using namespace SigDigger;

EqualizerControl::EqualizerControl(QWidget *parent, Suscan::Config *config) :
  InspectorCtl(parent, config),
  ui(new Ui::EqualizerControl)
{
  ui->setupUi(this);

  this->refreshUi();

  this->ui->rateEdit->setValidator(new QDoubleValidator(0.0, 3e6, 0, this));

  this->registerWidget(this->ui->typeCombo,    SIGNAL(activated(int)));
  this->registerWidget(this->ui->rateEdit,     SIGNAL(editingFinished()));
  this->registerWidget(this->ui->lockedButton, SIGNAL(clicked(bool)));
}

bool
EqualizerControl::applicable(QString const &key)
{
  return strncmp(
        key.toStdString().c_str(),
        "equalizer.",
        3) == 0;
}

void
EqualizerControl::refreshUi(void)
{
  int index = 0;

  if (this->getUint64("equalizer.type") == SUSCAN_INSPECTOR_EQUALIZER_CMA)
    index = 1;

  this->ui->rateEdit->setText(QString::number(this->getFloat("equalizer.rate")));
  this->ui->typeCombo->setCurrentIndex(index);
  this->ui->lockedButton->setChecked(this->getBoolean("equalizer.locked"));
}

void
EqualizerControl::parseConfig(void)
{
  uint64_t eqTypes[] =
  {
    SUSCAN_INSPECTOR_EQUALIZER_BYPASS,
    SUSCAN_INSPECTOR_EQUALIZER_CMA
  };

  this->refreshEntry("equalizer.rate", this->ui->rateEdit->text().toDouble());
  this->refreshEntry("equalizer.type", eqTypes[this->ui->typeCombo->currentIndex()]);
  this->refreshEntry("equalizer.locked", this->ui->lockedButton->isChecked());
}

EqualizerControl::~EqualizerControl()
{
  delete ui;
}
