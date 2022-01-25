//
//    DataSaverUI.cpp: Data saver UI
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

#include <QFileDialog>
#include <SuWidgetsHelpers.h>
#include "DataSaverUI.h"
#include "ui_DataSaverUI.h"

using namespace SigDigger;

//////////////////////////// DataSaverConfig ///////////////////////////////////
#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

void
DataSaverConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(path);
}

Suscan::Object &&
DataSaverConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("DataSaverConfig");

  STORE(path);

  return this->persist(obj);
}

////////////////////////////// DataSaverUI /////////////////////////////////////
void
DataSaverUI::connectAll(void)
{
  connect(
        this->ui->saveButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onChangeSavePath(void)));

  connect(
        this->ui->recordStartStopButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onRecordStartStop(void)));
}

// Setters
void
DataSaverUI::setRecordSavePath(std::string const &path)
{
  this->ui->savePath->setText(QString::fromStdString(path));
  this->refreshDiskUsage();
}

void
DataSaverUI::setSaveEnabled(bool enabled)
{
  if (!enabled)
    this->ui->saveButton->setChecked(false);

  this->ui->saveButton->setEnabled(enabled);
}

void
DataSaverUI::setCaptureSize(quint64 size)
{
  this->ui->captureSizeLabel->setText(
        SuWidgetsHelpers::formatBinaryQuantity(
          static_cast<qint64>(size)));
}

void
DataSaverUI::setDiskUsage(qreal usage)
{
  if (std::isnan(usage)) {
    this->ui->diskUsageProgress->setEnabled(false);
    this->ui->diskUsageProgress->setValue(100);
  } else {
    this->ui->diskUsageProgress->setEnabled(true);
    this->ui->diskUsageProgress->setValue(static_cast<int>(usage * 100));
  }
}

void
DataSaverUI::setIORate(qreal rate)
{
  this->ui->ioBwProgress->setValue(static_cast<int>(rate * 100));
  this->refreshDiskUsage();
}

void
DataSaverUI::setRecordState(bool state)
{
  this->ui->recordStartStopButton->setChecked(state);

  this->ui->recordStartStopButton->setText(state ? "Stop" : "Record");

  if (!state)
    this->ui->ioBwProgress->setValue(0);
}

// Getters
bool
DataSaverUI::getRecordState(void) const
{
  return this->ui->recordStartStopButton->isChecked();
}

std::string
DataSaverUI::getRecordSavePath(void) const
{
  return this->ui->savePath->text().toStdString();
}


DataSaverUI::DataSaverUI(QWidget *parent) :
  GenericDataSaverUI(parent),
  ui(new Ui::DataSaverUI)
{
  ui->setupUi(this);

  this->setRecordSavePath(QDir::currentPath().toStdString());

  this->connectAll();
}

DataSaverUI::~DataSaverUI()
{
  delete ui;
}


// Overridden methods
Suscan::Serializable *
DataSaverUI::allocConfig(void)
{
  return this->config = new DataSaverConfig();
}

void
DataSaverUI::applyConfig(void)
{
  if (this->config->path.size() > 0)
    this->setRecordSavePath(this->config->path);
}

///////////////////////////////// Slots ////////////////////////////////////////
void
DataSaverUI::onChangeSavePath(void)
{
  QFileDialog dialog(this->ui->saveButton);

  dialog.setFileMode(QFileDialog::DirectoryOnly);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setWindowTitle(QString("Select current save directory"));

  if (dialog.exec()) {
    QString path = dialog.selectedFiles().first();
    this->ui->savePath->setText(path);

    if (this->config != nullptr)
      this->config->path = path.toStdString();

    this->refreshDiskUsage();
    emit recordSavePathChanged(path);
  }
}

void
DataSaverUI::onRecordStartStop(void)
{
  this->ui->recordStartStopButton->setText(
        this->ui->recordStartStopButton->isChecked()
        ? "Stop"
        : "Record");

  emit recordStateChanged(this->ui->recordStartStopButton->isChecked());
}
