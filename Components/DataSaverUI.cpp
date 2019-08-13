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
#include "DataSaverUI.h"
#include "ui_DataSaverUI.h"

#include <sys/statvfs.h>

using namespace SigDigger;

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

void
DataSaverUI::refreshDiskUsage(void)
{
  std::string path = this->getRecordSavePath().c_str();
  struct statvfs svfs;

  if (statvfs(path.c_str(), &svfs) != -1) {
    this->ui->diskUsageProgress->setEnabled(true);
    this->setDiskUsage(
          1. - static_cast<qreal>(svfs.f_bavail) /
          static_cast<qreal>(svfs.f_blocks));
  } else {
    this->ui->diskUsageProgress->setEnabled(false);
    this->setDiskUsage(1);
  }
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

static QString
formatCaptureSize(quint64 size)
{
  if (size < (1ull << 10))
    return QString::number(size) + " bytes";
  else if (size < (1ull << 20))
    return QString::number(size >> 10) + " KiB";
  else if (size < (1ull << 30))
    return QString::number(size >> 20) + " MiB";

  return QString::number(size >> 30) + " GiB";
}

void
DataSaverUI::setCaptureSize(quint64 size)
{
  this->ui->captureSizeLabel->setText(
        formatCaptureSize(size * sizeof(float _Complex)));
}

void
DataSaverUI::setDiskUsage(qreal usage)
{
  this->ui->diskUsageProgress->setValue(static_cast<int>(usage * 100));
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
  QWidget(parent),
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
    emit recordSavePathChanged(path);
  }
}

void
DataSaverUI::onRecordStartStop(void)
{
  emit recordStateChanged(this->ui->recordStartStopButton->isChecked());
}
