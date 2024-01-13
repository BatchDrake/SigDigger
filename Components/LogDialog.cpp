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
  m_logger = Suscan::Logger::getInstance();

  ui->setupUi(this);

  setWindowTitle("Message log");

  connectAll();
}

void
LogDialog::saveLog(QString path)
{
  FILE *fp;

  if ((fp = fopen(path.toStdString().c_str(), "w")) == nullptr) {
    QString error(strerror(errno));

    QMessageBox::critical(
          this,
          "Save message log",
          "Failed to save message log to file: " + error + "\n\n");
    return;
  }

  for (int i = 0; i < m_msgVec.size(); ++i) {
    fprintf(
          fp,
          "%ld.%d,%s,%s,%s:%d,%s",
          m_msgVec[i].time.tv_sec,
          static_cast<int>(m_msgVec[i].time.tv_usec),
          su_log_severity_to_string(m_msgVec[i].severity),
          m_msgVec[i].domain.c_str(),
          m_msgVec[i].function.c_str(),
          m_msgVec[i].line,
          m_msgVec[i].message.c_str());
  }

  fclose(fp);
}

void
LogDialog::connectAll(void)
{
  connect(
        m_logger,
        SIGNAL(messageEmitted(Suscan::LoggerMessage)),
        this,
        SLOT(onMessage(Suscan::LoggerMessage)));

  connect(
        ui->saveButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSave(void)));

  connect(
        ui->clearButton,
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

  for (auto &p: m_msgVec) {
    if (p.severity == SU_LOG_SEVERITY_CRITICAL ||
        p.severity == SU_LOG_SEVERITY_ERROR)
      errText += "<li>" + QString::fromStdString(p.message).trimmed() + "</li>";
  }

  return errText + "</ul>";
}

bool
LogDialog::isRepeated(std::string const &msg) const
{
  if (m_msgVec.empty())
    return false;

  return m_msgVec.last().message == msg;
}

void
LogDialog::onMessage(Suscan::LoggerMessage msg)
{
  int row;
  bool newMessage = !isRepeated(msg.message);

  // New message
  if (newMessage) {
    row = ui->logTableWidget->rowCount();
    m_msgVec.push_back(msg);
    ui->logTableWidget->insertRow(row);
    m_repeat = 1;
  } else {
    row = ui->logTableWidget->rowCount() - 1;
    msg.message = msg.message + " (" + std::to_string(++m_repeat) + ")";
  }

  ui->logTableWidget->setItem(
        row,
        1,
        new QTableWidgetItem(
          QDateTime::fromSecsSinceEpoch(
            static_cast<qint64>(msg.time.tv_sec)).toString()));

  ui->logTableWidget->setItem(
        row,
        0,
        LogDialog::makeSeverityItem(msg.severity));

  ui->logTableWidget->setItem(
        row,
        3,
        new QTableWidgetItem(QString::fromStdString(msg.domain)));

  ui->logTableWidget->setItem(
        row,
        2,
        new QTableWidgetItem(QString::fromStdString(msg.message).trimmed()));

  ui->logTableWidget->setItem(
        row,
        4,
        new QTableWidgetItem(QString::fromStdString(msg.function)));

  ui->logTableWidget->setItem(
        row,
        5,
        new QTableWidgetItem(QString::number(msg.line)));

  ui->logTableWidget->resizeColumnsToContents();

  if (newMessage && ui->autoScrollButton->isChecked())
    ui->logTableWidget->scrollToBottom();

  if (msg.severity == SU_LOG_SEVERITY_CRITICAL ||
      msg.severity == SU_LOG_SEVERITY_ERROR)
    m_errorFound = true;
}

void
LogDialog::onClear(void)
{
  ui->logTableWidget->setRowCount(0);
  m_msgVec.clear();
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
    saveLog(path);
  }
}

