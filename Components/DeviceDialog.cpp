//
//    DeviceDialog.cpp: Description
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
#include <DeviceDialog.h>
#include "ui_DeviceDialog.h"

using namespace SigDigger;

DeviceDialog::DeviceDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DeviceDialog)
{
  ui->setupUi(this);

  this->connectAll();
}

DeviceDialog::~DeviceDialog()
{
  delete ui;
}


QPixmap
DeviceDialog::getDeviceIcon(Suscan::Source::Device const &dev)
{
  QString iconPath = ":/icons/";

  if (dev.isRemote())
    iconPath += "network-device";
  else
    iconPath += "devices";

  if (!dev.isAvailable())
    iconPath += "-unavail";

  return QPixmap(iconPath + ".png");
}

void
DeviceDialog::setRefreshing(bool refreshing)
{
  if (refreshing) {
    this->ui->okButton->setEnabled(false);
    this->ui->refreshButton->setEnabled(false);
    this->ui->refreshProgress->setMaximum(0);
  } else {
    this->ui->okButton->setEnabled(true);
    this->ui->refreshButton->setEnabled(true);
    this->ui->refreshProgress->setMaximum(1);
  }
}

void
DeviceDialog::refreshDone(void)
{
  this->setRefreshing(false);
  this->refreshDevices();
}

void
DeviceDialog::run(void)
{
  this->refreshDevices();
  this->exec();
}

void
DeviceDialog::refreshDevices(void)
{
  int i = 0;
  Suscan::Singleton *s = Suscan::Singleton::get_instance();
  std::vector<Suscan::Source::Device>::const_iterator start
      = s->getFirstDevice();
  std::vector<Suscan::Source::Device>::const_iterator end
      = s->getLastDevice();

  this->ui->deviceTable->clear();

  // Set headers
  this->ui->deviceTable->setHorizontalHeaderItem(
        0,
        new QTableWidgetItem(""));
  this->ui->deviceTable->setHorizontalHeaderItem(
        1,
        new QTableWidgetItem("Device ID"));
  this->ui->deviceTable->setHorizontalHeaderItem(
        2,
        new QTableWidgetItem("Driver"));

  this->ui->deviceTable->horizontalHeaderItem(0)->setTextAlignment(
        Qt::AlignLeft);
  this->ui->deviceTable->horizontalHeaderItem(1)->setTextAlignment(
        Qt::AlignLeft);
  this->ui->deviceTable->horizontalHeaderItem(2)->setTextAlignment(
        Qt::AlignLeft);

  for (auto p = start; p != end; ++p) {
    QTableWidgetItem *iconItem = new QTableWidgetItem();
    iconItem->setIcon(QIcon(getDeviceIcon(*p)));
    this->ui->deviceTable->setRowCount(i + 1);
    this->ui->deviceTable->setItem(i, 0, iconItem);
    this->ui->deviceTable->setItem(
          i,
          1,
          new QTableWidgetItem(QString::fromStdString(p->getDesc())));
    this->ui->deviceTable->setItem(
          i,
          2,
          new QTableWidgetItem(QString::fromStdString(p->getDriver())));
    ++i;
  }

  this->ui->deviceTable->resizeColumnToContents(1);
}

void
DeviceDialog::connectAll(void)
{
  connect(
        this->ui->okButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onOk(void)));

  connect(
        this->ui->refreshButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onRefresh(void)));
}

///////////////////////////////// Slots ////////////////////////////////////////
void
DeviceDialog::onRefresh(void)
{
  this->setRefreshing(true);
  emit refreshRequest();
}

void
DeviceDialog::onOk(void)
{
  emit accept();
}



