//
//    LogDialog.cpp: Display log messages
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
#include <LogDialog.h>
#include "ui_LogDialog.h"

#include <QDateTime>
#include <QTableWidgetItem>

using namespace SigDigger;

LogDialog::LogDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::LogDialog)
{
  this->logger = Suscan::Logger::getInstance();

  ui->setupUi(this);

  this->setWindowTitle("Message log");

  this->connectAll();
}

void
LogDialog::connectAll(void)
{
  connect(
        this->logger,
        SIGNAL(messageEmitted(Suscan::LoggerMessage)),
        this,
        SLOT(onMessage(Suscan::LoggerMessage)));

  connect(
        this->ui->saveButton,
        SIGNAL(triggered(QAction *)),
        this,
        SLOT(onSave(void)));

  connect(
        this->ui->clearButton,
        SIGNAL(triggered(QAction *)),
        this,
        SLOT(onClear(void)));
}

LogDialog::~LogDialog()
{
  delete ui;
}

/////////////////////////////////// Slots //////////////////////////////////////
QTableWidgetItem *
LogDialog::makeSeverityItem(enum sigutils_log_severity severity)
{
  switch (severity) {
    case SU_LOG_SEVERITY_CRITICAL:
      return new QTableWidgetItem(QIcon(":/icons/critical.png"), "");

    case SU_LOG_SEVERITY_ERROR:
      return new QTableWidgetItem(QIcon(":/icons/dialog-error.png"), "");

    case SU_LOG_SEVERITY_WARNING:
      return new QTableWidgetItem(QIcon(":/icons/dialog-warning.png"), "");

    case SU_LOG_SEVERITY_INFO:
      return new QTableWidgetItem(QIcon(":/icons/dialog-information.png"), "");

    case SU_LOG_SEVERITY_DEBUG:
      return new QTableWidgetItem(QIcon(":/icons/system-run.png"), "");
  }

  return new QTableWidgetItem("?");
}

void
LogDialog::onMessage(Suscan::LoggerMessage msg)
{
  int newRow = this->ui->logTableWidget->rowCount();

  this->ui->logTableWidget->insertRow(newRow);

  this->ui->logTableWidget->setItem(
        newRow,
        1,
        new QTableWidgetItem(
          QDateTime::fromTime_t(
            static_cast<unsigned int>(msg.time.tv_sec)).toString()));

  this->ui->logTableWidget->setItem(
        newRow,
        0,
        LogDialog::makeSeverityItem(msg.severity));

  this->ui->logTableWidget->setItem(
        newRow,
        3,
        new QTableWidgetItem(QString::fromStdString(msg.domain)));

  this->ui->logTableWidget->setItem(
        newRow,
        2,
        new QTableWidgetItem(QString::fromStdString(msg.message)));

  this->ui->logTableWidget->setItem(
        newRow,
        4,
        new QTableWidgetItem(QString::fromStdString(msg.function)));

  this->ui->logTableWidget->setItem(
        newRow,
        5,
        new QTableWidgetItem(QString::number(msg.line)));

  this->ui->logTableWidget->resizeColumnsToContents();

  if (this->ui->autoScrollButton->isChecked())
    this->ui->logTableWidget->scrollToBottom();
}

void
LogDialog::onClear(void)
{
  this->ui->logTableWidget->clear();
}

void
LogDialog::onSave(void)
{

}

