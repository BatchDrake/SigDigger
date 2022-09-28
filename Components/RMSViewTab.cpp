//
//    RMSViewTab.cpp: Description
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

#include <RMSViewTab.h>
#include <QMessageBox>
#include <cctype>
#include "ui_RMSViewTab.h"
#include <sstream>
#include <utility>
#include <string>
#include <QDateTime>
#include <QFileDialog>

#define MAX_LINE_SIZE  4096
#define TIMER_INTERVAL_MS 100
using namespace SigDigger;

RMSViewTab::RMSViewTab(QWidget *parent, QTcpSocket *socket) :
  QWidget(parent),
  socket(socket),
  ui(new Ui::RMSViewTab)
{
  setlocale(LC_ALL, "C");

  this->ui->setupUi(this);

  this->ui->waveform->setData(&this->data);
  this->ui->waveform->setHorizontalUnits("s");
  this->ui->waveform->setAutoFitToEnvelope(false);

  this->onToggleModes();

  this->timer.start(TIMER_INTERVAL_MS);
  this->processSocketData();

  this->connectAll();

}

// https://stackoverflow.com/questions/12966957/is-there-an-equivalent-in-c-of-phps-explode-function

bool
RMSViewTab::saveToMatlab(QString const &path)
{
  FILE *fp;

  if ((fp = fopen(path.toStdString().c_str(), "w")) == nullptr) {
    QMessageBox::critical(
          this,
          "Save data to MATLAB file",
          "Failed to save data to MATLAB file: "
          + QString(strerror(errno)));
    return false;
  }

  fprintf(fp, "RATE=%.9f;\n", this->rate / this->ui->intSpin->value());
  fprintf(fp, "TIMESTAMP=%.6f;\n", this->first);
  fprintf(fp, "X=[\n");
  for (size_t i = 0; i < this->data.size(); ++i)
    fprintf(
          fp,
          "  %.9e, %.9f\n",
          SU_C_REAL(this->data[i]),
          SU_C_IMAG(this->data[i]));

  fprintf(fp, "];\n");
  fclose(fp);

  return true;
}

void
RMSViewTab::integrateMeasure(qreal timestamp, SUFLOAT mag)
{
  int intLen = this->ui->intSpin->value();
  QDateTime date;

  this->energy_accum += mag;

  if (++this->accum_ctr == intLen) {
    this->energy_accum /= intLen;
    this->data.push_back(
          this->energy_accum
          + I * SU_ASFLOAT(SU_POWER_DB_RAW(this->energy_accum)));
    this->last = timestamp;
    this->accum_ctr = 0;
    this->energy_accum = 0;
    this->ui->waveform->refreshData();
    if (this->ui->autoFitButton->isChecked())
      this->fitVertical();
    this->ui->waveform->invalidate();
  }
}

bool
RMSViewTab::parseLine(void)
{
  SUFLOAT mag, db;
  QDateTime date;
  long sec;
  qreal usec;
  qreal rate;
  std::vector<std::string> fields;
  std::istringstream iss(this->line);

  // Description line allows commas and stuff
  if (line.compare(0, 5, "DESC,") == 0) {
    emit titleChanged(QString(line.c_str() + 5));
    return true;
  }

  for (std::string token; std::getline(iss, token, ','); )
    fields.push_back(std::move(token));

  if (fields.size() == 2) {
    if (fields[0] == "RATE") {
      if (sscanf(fields[1].c_str(), "%lf", &rate) < 1)
        return false;

      this->rate = rate;
      this->ui->waveform->setSampleRate(rate);
    } else {
      return false;
    }
  } else if (fields.size() == 4) {
    if (sscanf(fields[0].c_str(), "%ld", &sec) < 1)
      return false;
    if (sscanf(fields[1].c_str(), "%lf", &usec) < 1)
      return false;
    if (sscanf(fields[2].c_str(), "%e", &mag) < 1)
      return false;
    if (sscanf(fields[3].c_str(), "%g", &db) < 1)
      return false;

    this->integrateMeasure(
          static_cast<qreal>(sec) + usec,
          mag);

    if (this->data.size() > 0) {
      date.setTime_t(static_cast<unsigned int>(this->last));
      this->ui->lastLabel->setText("Last: " + date.toString());

      if (this->data.size() == 1) {
        this->first = this->last;
        this->ui->sinceLabel->setText("Since: " + date.toString());
      }
    }
    return true;
  }

  return false;
}

void
RMSViewTab::processSocketData(void)
{
  char c;
  bool firstTime = this->data.size() == 0;

  while (this->socket->bytesAvailable() > 0) {
    if (this->socket->read(&c, 1) < 1) {
      this->disconnectSocket();
      return;
    }

    if (c == '\n') {
      this->parseLine();
      this->line.clear();
    } else if (isprint(c)) {
      this->line += c;
      if (this->line.size() >= MAX_LINE_SIZE) {
        this->line.clear();
        this->disconnect();
        QMessageBox::critical(
              this,
              "Max line size exceeded",
              "Remote peer attempted to flood us. Preventively disconnected");
      }
    }
  }

  if (firstTime && this->data.size() > 0) {
    qint64 width = this->ui->waveform->getVerticalAxisWidth();
    this->ui->waveform->zoomHorizontal(
          -width,
          static_cast<qint64>(this->ui->waveform->size().width()) - width);
  }
}

void
RMSViewTab::disconnectSocket(void)
{
  if (this->socket != nullptr) {
    this->socket->close();
    delete this->socket;
    this->socket = nullptr;
  }

  this->ui->stopButton->setEnabled(false);
  this->ui->stopButton->setChecked(false);
  this->ui->stopButton->setIcon(QIcon(":/icons/offline.png"));
}

void
RMSViewTab::toggleModes(QObject *sender)
{
  if (sender == nullptr || sender == this->ui->autoScrollButton)
    this->ui->waveform->setAutoScroll(this->ui->autoScrollButton->isChecked());

  if (sender == nullptr || sender == this->ui->autoFitButton)
    this->ui->resetButton->setEnabled(!this->ui->autoFitButton->isChecked());

  if (sender == nullptr || sender == this->ui->dbButton) {
    if (this->ui->dbButton->isChecked()) {
      this->ui->waveform->setVerticalUnits("dB");
      this->ui->waveform->setRealComponent(false);
    } else {
      this->ui->waveform->setVerticalUnits("");
      this->ui->waveform->setRealComponent(true);
    }
  }

  if (sender == nullptr || sender == this->ui->autoFitButton) {
    this->ui->waveform->refreshData();
    if (this->ui->autoFitButton->isChecked())
      this->fitVertical();
  }
}

void
RMSViewTab::fitVertical(void)
{
  SUCOMPLEX dataMin = this->ui->waveform->getDataMin();
  SUCOMPLEX dataMax = this->ui->waveform->getDataMax();
  qreal min, max;

  if (this->ui->dbButton->isChecked()) {
    min = SU_C_IMAG(dataMin);
    max = SU_C_IMAG(dataMax);
  } else {
    min = SU_C_REAL(dataMin);
    max = SU_C_REAL(dataMax);
  }

  this->ui->waveform->zoomVertical(min, max);
}

void
RMSViewTab::connectAll(void)
{
  connect(
        &this->timer,
        SIGNAL(timeout()),
        this,
        SLOT(onTimeout()));

  connect(
        this->socket,
        SIGNAL(disconnected()),
        this,
        SLOT(onSocketDisconnected()));

  connect(
        this->ui->stopButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onStop()));

  connect(
        this->ui->saveButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSave()));

  connect(
        this->ui->resetButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onResetZoom()));

  connect(
        this->ui->autoScrollButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleModes()));

  connect(
        this->ui->autoFitButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleModes()));

  connect(
        this->ui->dbButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleModes()));

  connect(
        this->ui->intSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onValueChanged(int)));
}

RMSViewTab::~RMSViewTab()
{
  this->disconnectSocket();
  delete ui;
}


//////////////////////////////////// Slots /////////////////////////////////////
void
RMSViewTab::onSave(void)
{
  QString fileName =
      QFileDialog::getSaveFileName(
        this,
        "Save data to MATLAB file",
        "power.m",
        "MATLAB scripts (*.m)");

  if (fileName.size() > 0)
    this->saveToMatlab(fileName);
}

void
RMSViewTab::onStop(void)
{
  this->disconnectSocket();
}

void
RMSViewTab::onTimeout(void)
{
  if (this->socket != nullptr)
    this->processSocketData();
}

void
RMSViewTab::onToggleModes(void)
{
  QObject *sender = QObject::sender();

  this->toggleModes(sender);
}

void
RMSViewTab::onResetZoom(void)
{
  this->fitVertical();
  this->ui->waveform->zoomHorizontalReset();
}

void
RMSViewTab::onSocketDisconnected(void)
{
  this->socket = nullptr;
  this->disconnectSocket();
}

void
RMSViewTab::onValueChanged(int)
{
  this->energy_accum = 0;
  this->accum_ctr = 0;
  this->ui->sinceLabel->setText("Since: N/A");
  this->ui->lastLabel->setText("Last: N/A");
  this->data.clear();
  this->ui->waveform->setSampleRate(rate / this->ui->intSpin->value());
  this->ui->waveform->refreshData();
  if (this->ui->autoFitButton->isChecked())
    this->fitVertical();
}
