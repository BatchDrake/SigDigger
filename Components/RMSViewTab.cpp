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
#include <QMessageBox>
#include <complex.h>
#include <QToolTip>
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
  this->ui->waveform->setHorizontalUnits("unix");
  this->onToggleModes();

  this->ui->averageTimeLabel->setVisible(false);
  this->ui->averageTimeSpinBox->setVisible(false);


  this->timer.start(TIMER_INTERVAL_MS);

  if (socket != nullptr)
    this->processSocketData();

  this->connectAll();

}

void
RMSViewTab::setIntegrationTimeMode(qreal min, qreal max)
{
  this->ui->stopButton->setIcon(QIcon(":/icons/start-capture.png"));
  this->ui->stopButton->setToolTip("Toggle capture on / off");

  this->ui->timeSpinBox->setTimeMin(min);
  this->ui->timeSpinBox->setTimeMax(max);

  this->ui->averageTimeLabel->setVisible(true);
  this->ui->averageTimeSpinBox->setVisible(true);

  ui->intSpin->setValue(1);
  ui->stackedWidget->setCurrentIndex(1);
}

qreal
RMSViewTab::getCurrentTimeDelta() const
{
  if (intTimeMode())
    return ui->intSpin->value() * ui->timeSpinBox->timeValue();
  else
    return ui->intSpin->value() / rate;
}

bool
RMSViewTab::intTimeMode() const
{
  return ui->stackedWidget->currentIndex() == 1;
}

void
RMSViewTab::setIntegrationTimeHint(qreal hint)
{
  m_time = hint;

  ui->timeSpinBox->setTimeValue(hint);
  ui->timeSpinBox->setBestUnits(true);

  setSampleRate(1 / hint);
}

qreal
RMSViewTab::getIntegrationTimeHint() const
{
  return ui->timeSpinBox->timeValue();
}


void
RMSViewTab::setSampleRate(qreal rate)
{
  this->rate = rate;
  this->ui->waveform->setSampleRate(rate / this->ui->intSpin->value());

  bool blocked = ui->averageTimeSpinBox->blockSignals(true);
  this->ui->averageTimeSpinBox->setTimeMin(1. / rate);
  this->ui->averageTimeSpinBox->setTimeMax(3600);
  this->ui->averageTimeSpinBox->setSampleRate(rate);
  this->ui->averageTimeSpinBox->setBestUnits(false);
  ui->averageTimeSpinBox->blockSignals(blocked);
}

void
RMSViewTab::feed(qreal timeStamp, qreal mag)
{
  bool firstTime = this->data.size() == 0;

  if (firstTime) {
    bool shouldHavePoint = ui->intSpin->value() > 1;
    if (m_haveCurrSamplePoint != shouldHavePoint) {
      m_haveCurrSamplePoint = shouldHavePoint;

      if (shouldHavePoint) {
        m_currSampleIterator = ui->waveform->addPoint(
              0,
              1,
              ui->waveform->getForegroundColor());
      } else {
        ui->waveform->removePoint(m_currSampleIterator);
      }
    }
  }

  this->integrateMeasure(timeStamp, SCAST(SUFLOAT, mag));

  if (this->data.size() > 0) {
    QDateTime date;
    date.setSecsSinceEpoch(static_cast<qint64>(this->last));
    this->ui->lastLabel->setText("Last: " + date.toString());

    if (this->data.size() == 1) {
      this->first = this->last;
      if (!this->ui->dateTimeEdit->isEnabled())
        this->ui->dateTimeEdit->setDateTime(
              QDateTime::fromSecsSinceEpoch(this->first));

      refreshUi();

      this->ui->sinceLabel->setText("Since: " + date.toString());
    }

    if (firstTime) {
      qint64 width = this->ui->waveform->getVerticalAxisWidth();
      this->ui->waveform->zoomHorizontal(
            -width,
            static_cast<qint64>(this->ui->waveform->size().width()) - width);
    }
  }
}

void
RMSViewTab::setColorConfig(ColorConfig const &cfg)
{
  this->ui->waveform->setBackgroundColor(cfg.spectrumBackground);
  this->ui->waveform->setForegroundColor(cfg.spectrumForeground);
  this->ui->waveform->setAxesColor(cfg.spectrumAxes);
  this->ui->waveform->setTextColor(cfg.spectrumText);
  this->ui->waveform->setSelectionColor(cfg.selection);
}
// https://stackoverflow.com/questions/12966957/is-there-an-equivalent-in-c-of-phps-explode-function

int
RMSViewTab::getTimeScaleSelection() const
{
  return ui->timeScaleCombo->currentIndex();
}

void
RMSViewTab::setTimeScaleSelection(int sel)
{
  ui->timeScaleCombo->setCurrentIndex(sel);
  refreshUi();
}

void
RMSViewTab::refreshUi()
{
  bool haveCustomDateTime = false;
  qreal first = this->data.size() > 0 ? this->first : SCAST(qreal, time(nullptr));

  switch (ui->timeScaleCombo->currentIndex()) {
    case 0:
      ui->waveform->setTimeStart(0);
      ui->waveform->setHorizontalUnits("s");
      break;

    case 1:
      ui->waveform->setTimeStart(first);
      ui->waveform->setHorizontalUnits("unix");
      break;

    case 2:
      haveCustomDateTime = true;
      first = ui->dateTimeEdit->dateTime().toSecsSinceEpoch();
      ui->waveform->setTimeStart(first);
      ui->waveform->setHorizontalUnits("unix");
      break;
  }

  ui->dateTimeEdit->setEnabled(haveCustomDateTime);
}

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

  this->energy_accum += mag;

  if (++this->accum_ctr == intLen) {
    this->energy_accum /= intLen;
    this->data.push_back(
          this->energy_accum
          + SU_I * SU_ASFLOAT(SU_POWER_DB_RAW(this->energy_accum)));
    this->last = timestamp;
    this->accum_ctr = 0;
    this->energy_accum = 0;
    this->ui->waveform->refreshData();
    if (this->ui->autoFitButton->isChecked())
      this->fitVertical();
    this->ui->waveform->invalidate();
  } else {
    if (m_haveCurrSamplePoint) {
      if (this->data.size() == 0) {
        mag = this->energy_accum / this->accum_ctr;
      } else {
        SUFLOAT prev = SU_C_REAL(this->data[this->data.size() - 1]);
        mag = (prev * (intLen - this->accum_ctr) + this->energy_accum) / intLen;
      }

      SUCOMPLEX curr = mag + SU_I * SU_ASFLOAT(SU_POWER_DB_RAW(mag));
      m_currSampleIterator->point = curr;
      m_currSampleIterator->t = this->data.size() * getCurrentTimeDelta();
      m_currSampleIterator = ui->waveform->refreshPoint(m_currSampleIterator);
    }
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

      this->setSampleRate(rate);
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

    this->feed(SCAST(qreal, sec) + usec, SCAST(qreal, mag));

    return true;
  }

  return false;
}

void
RMSViewTab::processSocketData(void)
{
  char c;

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
}

void
RMSViewTab::disconnectSocket(void)
{
  if (this->socket != nullptr) {
    this->socket->close();
    delete this->socket;
    this->socket = nullptr;

    this->ui->stopButton->setEnabled(false);
    this->ui->stopButton->setChecked(false);
    this->ui->stopButton->setIcon(QIcon(":/icons/offline.png"));
  }
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
RMSViewTab::setVerticalLimitsLinear(qreal min, qreal max)
{
  if (this->ui->dbButton->isChecked()) {
    setVerticalLimitsDb(SU_POWER_DB_RAW(min), SU_POWER_DB_RAW(max));
    return;
  }

  ui->waveform->zoomVertical(min, max);}

void
RMSViewTab::setVerticalLimitsDb(qreal min, qreal max)
{
  if (!this->ui->dbButton->isChecked()) {
    setVerticalLimitsLinear(SU_POWER_MAG_RAW(min), SU_POWER_MAG_RAW(max));
    return;
  }

  ui->waveform->zoomVertical(min, max);
}

qreal
RMSViewTab::getMin() const
{
  return ui->waveform->getMin();
}

qreal
RMSViewTab::getMax() const
{
  return ui->waveform->getMax();
}

void
RMSViewTab::fitVertical(void)
{
  if (this->ui->waveform->getDataLength() > 0) {
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

    if (min == max) {
      min -= 1;
      max += 1;
    }

    this->ui->waveform->zoomVertical(min, max);
  }
}

bool
RMSViewTab::userClear(QString const &message)
{
  if (this->data.size() > 0) {
    auto reply = QMessageBox::question(
          this,
          "Clear current plot",
          message,
          QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel);
    if (reply == QMessageBox::StandardButton::Cancel) {
      ui->timeSpinBox->setTimeValue(m_time);
      return false;
    }
    if (reply == QMessageBox::StandardButton::Yes) {
      QString fileName =
          QFileDialog::getSaveFileName(
            this,
            "Save data to MATLAB file",
            "power.m",
            "MATLAB scripts (*.m)");

      if (fileName.size() == 0) {
        ui->timeSpinBox->setTimeValue(m_time);
        return false;
      }

      this->saveToMatlab(fileName);
    }
  }

  return true;
}

void
RMSViewTab::connectAll(void)
{
  connect(
        &this->timer,
        SIGNAL(timeout()),
        this,
        SLOT(onTimeout()));

  if (socket != nullptr){
    connect(
          this->socket,
          SIGNAL(disconnected()),
          this,
          SLOT(onSocketDisconnected()));
  }

  connect(
        this->ui->stopButton,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleStartStop()));

  connect(
        this->ui->timeSpinBox,
        SIGNAL(changed(qreal, qreal)),
        this,
        SLOT(onTimeChanged(qreal, qreal)));

  connect(
        this->ui->averageTimeSpinBox,
        SIGNAL(changed(qreal, qreal)),
        this,
        SLOT(onAverageTimeChanged()));

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

  connect(
        this->ui->waveform,
        SIGNAL(pointClicked(qreal, qreal, Qt::KeyboardModifiers)),
        this,
        SLOT(onPointClicked(qreal,qreal, Qt::KeyboardModifiers)));

  connect(
        this->ui->waveform,
        SIGNAL(toolTipAt(int, int, qreal, qreal)),
        this,
        SLOT(onToolTip(int,int,qreal,qreal)));

  connect(
        this->ui->timeScaleCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onTimeScaleChanged()));

  connect(
        this->ui->dateTimeEdit,
        SIGNAL(dateTimeChanged(QDateTime)),
        this,
        SLOT(onTimeScaleChanged()));
}

bool
RMSViewTab::running() const
{
  return m_running;
}

RMSViewTab::~RMSViewTab()
{
  this->disconnectSocket();
  delete ui;
}

bool
RMSViewTab::isLogScale() const
{
  return ui->dbButton->isChecked();
}

bool
RMSViewTab::isAutoFit() const
{
  return ui->autoFitButton->isChecked();
}

bool
RMSViewTab::isAutoScroll() const
{
  return ui->autoScrollButton->isChecked();
}

void
RMSViewTab::setLogScale(bool enabled)
{
  ui->dbButton->setChecked(enabled);

  toggleModes(ui->dbButton);
}

void
RMSViewTab::setAutoFit(bool enabled)
{
  ui->autoFitButton->setChecked(enabled);

  toggleModes(ui->autoFitButton);
}

void
RMSViewTab::setAutoScroll(bool enabled)
{
  ui->autoScrollButton->setChecked(enabled);

  toggleModes(ui->autoScrollButton);
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
RMSViewTab::onToggleStartStop(void)
{
  if (m_running != ui->stopButton->isChecked()) {
    if (m_running) {
      // Disconnect
      this->disconnectSocket();
    } else {
      // Starting
      if (!userClear(
            "Starting a capture will overwrite the current plot. Do you want to save it first?")) {
        ui->stopButton->setChecked(false);
        return;
      }

      onValueChanged(0);
    }
    m_running = ui->stopButton->isChecked();
    emit toggleState();
  }
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

  emit viewTypeChanged();
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

void
RMSViewTab::onTimeChanged(qreal time, qreal)
{
  bool blocked = ui->timeSpinBox->blockSignals(true);

  if (!userClear(
        "Changing the integration time will clear the data of the current plot. "
        "Do you want to save the current plot first?"))
    goto done;

  onValueChanged(0);

  setIntegrationTimeHint(time);

  emit integrationTimeChanged(time);

done:
  ui->timeSpinBox->blockSignals(blocked);
}

void
RMSViewTab::onAverageTimeChanged()
{
  // Changing the averaging time means that we are going to keep the current
  // integration time, but we are going to change the averaging period. This
  // also implies that the sample rate will change, which means that we
  // have to ask the user about this change.
  bool blocked = ui->averageTimeSpinBox->blockSignals(true);
  int points;

  if (!userClear(
        "Changing the integration time will clear the data of the current plot. "
        "Do you want to save the current plot first?"))
    goto done;

  points = SCAST(int, ui->averageTimeSpinBox->timeValue() * this->rate);
  if (points < 1)
    points = 1;
  ui->averageTimeSpinBox->setTimeValue(points / this->rate);
  ui->intSpin->setValue(points);
  onValueChanged(0);

done:
  ui->averageTimeSpinBox->blockSignals(blocked);
}

void
RMSViewTab::onPointClicked(qreal, qreal level, Qt::KeyboardModifiers mod)
{
  WaveVCursor cursor;
  QList<WaveVCursor> list;
  qreal dbLevel, linearLevel;

  if (ui->dbButton->isChecked()) {
    dbLevel = level;
    linearLevel = SU_POWER_MAG_RAW(level);
  } else {
    dbLevel = SU_POWER_DB_RAW(level);
    linearLevel = level;
  }

  if (!m_haveMarker) {
    m_markerDb = dbLevel;
    m_markerLinear = linearLevel;
    m_haveMarker = true;
  } else if (!m_haveDeltaMarker
             || mod.testFlag(Qt::KeyboardModifier::ShiftModifier)) {
    m_markerDeltaDb = dbLevel;
    m_markerDeltaLinear = linearLevel;
    m_haveDeltaMarker = true;
  } else {
    m_haveMarker = m_haveDeltaMarker = false;
  }

  if (m_haveMarker) {
    cursor.color = QColor(0xff, 0xff, 0);
    cursor.string = QString::asprintf("Ref: %.3f dB (", m_markerDb) +
        SuWidgetsHelpers::formatQuantity(m_markerLinear, 4, "") + ")";
    cursor.level = SUCOMPLEX(SU_ASFLOAT(m_markerLinear), SU_ASFLOAT(m_markerDb));

    list.push_back(cursor);
  }

  if (m_haveDeltaMarker) {
    cursor.color = QColor(0, 0xff, 0xff);
    cursor.string = QString::asprintf("R = %.3f dB (Δ = ", m_markerDeltaDb - m_markerDb) +
        SuWidgetsHelpers::formatQuantity(m_markerDeltaLinear - m_markerLinear, 4, "") + ")";
    cursor.level = SUCOMPLEX(SU_ASFLOAT(m_markerDeltaLinear), SU_ASFLOAT(m_markerDeltaDb));

    list.push_back(cursor);
  }


  ui->waveform->setVCursorList(list);
}

void
RMSViewTab::onToolTip(int x, int y, qreal t, qreal level)
{
  qreal levelDb, levelLinear;
  QString valueString;
  QString timeString;

  if (ui->dbButton->isChecked()) {
    levelDb = level;
    levelLinear = SU_POWER_MAG_RAW(level);
  } else {
    levelDb = SU_POWER_DB_RAW(level);
    levelLinear = level;
  }

  if (ui->timeScaleCombo->currentIndex() == 0)
    timeString = "t = " + SuWidgetsHelpers::formatQuantityFromDelta(
          t,
          getCurrentTimeDelta(),
          "s");
  else
    timeString = "t = " + SuWidgetsHelpers::formatQuantity(
          t,
          0,
          "unix");

  if (!m_haveMarker) {
    valueString = QString::asprintf("%.3f dB (", levelDb) +
        SuWidgetsHelpers::formatQuantity(levelLinear, 4, "") + ")";
  } else {
    qreal diffLinear, diffDb;

    diffLinear = levelLinear - m_markerLinear;
    diffDb     = levelDb - m_markerDb;

    if (ui->dbButton->isChecked()) {
      valueString = QString::asprintf("%.3f dB (R = %+.3f dB)", levelDb, diffDb);
    } else {
      valueString = SuWidgetsHelpers::formatQuantity(levelLinear, 4, "")
          + " (Δ = "
          + SuWidgetsHelpers::formatQuantity(diffLinear, 4, "", true)
          + ")";

    }
  }

  QToolTip::showText(QPoint(x, y), timeString + ", pwr = " + valueString);
}

void
RMSViewTab::onTimeScaleChanged()
{
  refreshUi();

  emit viewTypeChanged();
}
