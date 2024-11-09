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
#include <Suscan/Device.h>
#include "ui_DeviceDialog.h"

using namespace SigDigger;

DeviceDialog::DeviceDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DeviceDialog)
{
  ui->setupUi(this);

  connectAll();
}

DeviceDialog::~DeviceDialog()
{
  delete ui;
}


QPixmap
DeviceDialog::getDeviceIcon(Suscan::DeviceProperties const &dev)
{
  QString iconPath = ":/icons/";

  if (dev.analyzer() == "remote")
    iconPath += "network-device";
  else
    iconPath += "devices";

  return QPixmap(iconPath + ".png");
}

void
DeviceDialog::setRefreshing(bool refreshing)
{
  if (refreshing) {
    ui->okButton->setEnabled(false);
    ui->refreshButton->setEnabled(false);
    ui->refreshProgress->setMaximum(0);
  } else {
    ui->okButton->setEnabled(true);
    ui->refreshButton->setEnabled(true);
    ui->refreshProgress->setMaximum(1);
  }
}

void
DeviceDialog::refreshDone()
{
  setRefreshing(false);
  refreshDevices();
}

void
DeviceDialog::run()
{
  refreshDevices();
  exec();
}

void
DeviceDialog::refreshDevices()
{
  int i = 0;
  Suscan::DeviceFacade *facade = Suscan::DeviceFacade::instance();

  ui->deviceTable->clear();

  // Set headers
  ui->deviceTable->setHorizontalHeaderItem(0, new QTableWidgetItem(""));
  ui->deviceTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Device ID"));
  ui->deviceTable->setHorizontalHeaderItem(2, new QTableWidgetItem("Driver / Transport"));

  ui->deviceTable->horizontalHeaderItem(0)->setTextAlignment(
        Qt::AlignLeft);
  ui->deviceTable->horizontalHeaderItem(1)->setTextAlignment(
        Qt::AlignLeft);
  ui->deviceTable->horizontalHeaderItem(2)->setTextAlignment(
        Qt::AlignLeft);

  auto devices = facade->devices();

  for (auto &dev : devices) {
    QString driver;
    QTableWidgetItem *iconItem = new QTableWidgetItem();
    iconItem->setIcon(QIcon(getDeviceIcon(dev)));
    ui->deviceTable->setRowCount(i + 1);
    ui->deviceTable->setItem(i, 0, iconItem);

    ui->deviceTable->setItem(
          i,
          1,
          new QTableWidgetItem(QString::fromStdString(dev.label())));

    if (dev.analyzer() == "remote")
      driver = "SuRPC";
    else
      driver = dev.get("device").c_str();

    ui->deviceTable->setItem(
          i,
          2,
          new QTableWidgetItem(driver));
    ++i;
  }

  ui->deviceTable->resizeColumnToContents(1);
}

void
DeviceDialog::connectAll()
{
  connect(
        ui->okButton,
        SIGNAL(clicked()),
        this,
        SLOT(onOk()));

  connect(
        ui->refreshButton,
        SIGNAL(clicked()),
        this,
        SLOT(onRefresh()));
}

///////////////////////////////// Slots ////////////////////////////////////////
void
DeviceDialog::onRefresh()
{
  setRefreshing(true);
  emit refreshRequest();
}

void
DeviceDialog::onOk()
{
  emit accept();
}



