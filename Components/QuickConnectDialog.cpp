//
//    QuickConnectDialog.cpp: Quick connect dialog
//    Copyright (C) 2022 Gonzalo José Carracedo Carballal
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
#include "QuickConnectDialog.h"
#include "ui_QuickConnectDialog.h"

using namespace SigDigger;

QuickConnectDialog::QuickConnectDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::QuickConnectDialog)
{
  ui->setupUi(this);

  this->setWindowFlags(
    this->windowFlags() & ~Qt::WindowMaximizeButtonHint);
}

void
QuickConnectDialog::setProfile(Suscan::Source::Config const &prof)
{
  if (prof.getInterface() == SUSCAN_SOURCE_REMOTE_INTERFACE) {
    this->ui->hostEdit->setText(
          QString::fromStdString(prof.getParam("host")));
    this->ui->userEdit->setText(
          QString::fromStdString(prof.getParam("user")));
    this->ui->passwordEdit->setText(
          QString::fromStdString(prof.getParam("password")));
    this->ui->portSpin->setValue(
          QString::fromStdString(prof.getParam("port")).toInt());
  }
}

QString
QuickConnectDialog::getHost(void) const
{
  return this->ui->hostEdit->text();
}

QString
QuickConnectDialog::getUser(void) const
{
  return this->ui->userEdit->text();
}

QString
QuickConnectDialog::getPassword(void) const
{
  return this->ui->passwordEdit->text();
}

int
QuickConnectDialog::getPort(void) const
{
  return this->ui->portSpin->value();
}

QuickConnectDialog::~QuickConnectDialog()
{
  delete ui;
}
