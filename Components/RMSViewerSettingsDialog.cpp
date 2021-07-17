//
//    RMSViewerSettingsDialog.cpp: RMSViewer settings dialog
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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

#include <RMSViewerSettingsDialog.h>
#include "ui_RMSViewerSettingsDialog.h"

using namespace SigDigger;

RMSViewerSettingsDialog::RMSViewerSettingsDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::RMSViewerSettingsDialog)
{
  ui->setupUi(this);
}

RMSViewerSettingsDialog::~RMSViewerSettingsDialog()
{
  delete ui;
}

QString
RMSViewerSettingsDialog::getHost(void) const
{
  return this->ui->listenAddrEdit->text();
}

uint16_t
RMSViewerSettingsDialog::getPort(void) const
{
  return static_cast<uint16_t>(this->ui->listenPortSpin->value());
}
