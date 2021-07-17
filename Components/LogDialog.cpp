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
#include <QMessageBox>
#include <cerrno>
#include <cstring>
#include <QFileDialog>

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
LogDialog::saveLog(QString path)
{
  FILE *fp;

  if ((fp = fopen(path.toStdString().c_str(), "w")) == nullptr) {
    QString error(errno);

    QMessageBox::critical(
          this,
          "Save message log",
          "Failed to save message log to file: " + error + "\n\n");
    return;
  }

  for (int i = 0; i < this->msgVec.size(); ++i) {
    fprintf(
          fp,
          "%ld.%d,%s,%s,%s:%d,%s",
          this->msgVec[i].time.tv_sec,
          static_cast<int>(this->msgVec[i].time.tv_usec),
          su_log_severity_to_string(this->msgVec[i].severity),
          this->msgVec[i].domain.c_str(),
          this->msgVec[i].function.c_str(),
          this->msgVec[i].line,
          this->msgVec[i].message.c_str());
  }

  fclose(fp);
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
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSave(void)));

  connect(
        this->ui->clearButton,
        SIGNAL(clicked(bool)),
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

QString
LogDialog::getErrorHtml(void) const
{
  QString errText = "<ul>";

  for (auto &p: this->msgVec) {
    if (p.severity == SU_LOG_SEVERITY_CRITICAL ||
        p.severity == SU_LOG_SEVERITY_ERROR)
      errText += "<li>" + QString::fromStdString(p.message).trimmed() + "</li>";
  }

  return errText + "</ul>";
}

void
LogDialog::onMessage(Suscan::LoggerMessage msg)
{
  int newRow = this->ui->logTableWidget->rowCount();

  this->msgVec.push_back(msg);
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
        new QTableWidgetItem(QString::fromStdString(msg.message).trimmed()));

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

  if (msg.severity == SU_LOG_SEVERITY_CRITICAL ||
      msg.severity == SU_LOG_SEVERITY_ERROR)
    this->errorFound = true;
}

void
LogDialog::onClear(void)
{
  this->ui->logTableWidget->setRowCount(0);
  this->msgVec.clear();
}

void
LogDialog::onSave(void)
{
  QFileDialog dialog(this);
  QStringList filters;

  dialog.setFileMode(QFileDialog::FileMode::AnyFile);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setWindowTitle(QString("Save message log"));

  filters << "Log files (*.log)"
          << "Any (*)";

  dialog.setNameFilters(filters);

  if (dialog.exec()) {
    QString path = dialog.selectedFiles().first();
    this->saveLog(path);
  }
}

