//
//    RemoteControlTab.cpp: description
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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
#include "RemoteControlTab.h"
#include "ui_RemoteControlTab.h"
#include <SuWidgetsHelpers.h>

using namespace SigDigger;

void
RemoteControlTab::save()
{
  this->rcConfig.enabled = this->ui->enableCheck->isChecked();
  this->rcConfig.host    = this->ui->hostEdit->text().toStdString();
  this->rcConfig.port    = SCAST(unsigned, this->ui->portSpin->value());
}

void
RemoteControlTab::refreshUi()
{
  this->ui->enableCheck->setChecked(this->rcConfig.enabled);
  this->ui->hostEdit->setText(QString::fromStdString(this->rcConfig.host));
  this->ui->portSpin->setValue(this->ui->portSpin->value());

  this->ui->hostEdit->setEnabled(this->ui->enableCheck->isChecked());
  this->ui->portSpin->setEnabled(this->ui->enableCheck->isChecked());
}

void
RemoteControlTab::setRemoteControlConfig(RemoteControlConfig const &config)
{
  this->rcConfig = config;
  this->refreshUi();
  this->modified = false;
}

RemoteControlConfig
RemoteControlTab::getRemoteControlConfig() const
{
  return this->rcConfig;
}

bool
RemoteControlTab::hasChanged() const
{
  return this->modified;
}

void
RemoteControlTab::connectAll()
{
  connect(
        this->ui->enableCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onConfigChanged()));

  connect(
        this->ui->hostEdit,
        SIGNAL(textEdited(QString)),
        this,
        SLOT(onConfigChanged()));

  connect(
        this->ui->portSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onConfigChanged()));
}

RemoteControlTab::RemoteControlTab(QWidget *parent) :
  ConfigTab(parent, "Remote control"),
  ui(new Ui::RemoteControlTab)
{
  ui->setupUi(this);

  this->connectAll();
}

RemoteControlTab::~RemoteControlTab()
{
  delete ui;
}

////////////////////////////////// Slots ///////////////////////////////////////
void
RemoteControlTab::onConfigChanged()
{
  this->ui->hostEdit->setEnabled(this->ui->enableCheck->isChecked());
  this->ui->portSpin->setEnabled(this->ui->enableCheck->isChecked());

  this->modified = true;
  emit changed();
}

