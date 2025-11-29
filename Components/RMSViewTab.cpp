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
#include <util/npy.h>

#define MAX_LINE_SIZE  4096
#define TIMER_INTERVAL_MS 100
using namespace SigDigger;

RMSViewTab::RMSViewTab(QWidget *parent, QTcpSocket *socket) :
  QWidget(parent),
  socket(socket),
  ui(new Ui::RMSViewTab)
{
  setlocale(LC_ALL, "C");

  ui->setupUi(this);

  ui->waveform->setData(&m_data);
  ui->waveform->setHorizontalUnits("s");
  ui->waveform->setAutoFitToEnvelope(false);
  ui->waveform->setHorizontalUnits("unix");
  onToggleModes();

  ui->averageTimeLabel->setVisible(false);
  ui->averageTimeSpinBox->setVisible(false);


  m_timer.start(TIMER_INTERVAL_MS);

  if (socket != nullptr)
    processSocketData();

  connectAll();

}

void
RMSViewTab::setIntegrationTimeMode(qreal min, qreal max)
{
  ui->stopButton->setIcon(QIcon(":/icons/start-capture.png"));
  ui->stopButton->setToolTip("Toggle capture on / off");

  ui->timeSpinBox->setTimeMin(min);
  ui->timeSpinBox->setTimeMax(max);

  ui->averageTimeLabel->setVisible(true);
  ui->averageTimeSpinBox->setVisible(true);

  ui->intSpin->setValue(1);
  ui->stackedWidget->setCurrentIndex(1);
}

qreal
RMSViewTab::getCurrentTimeDelta() const
{
  if (intTimeMode())
    return ui->intSpin->value() * ui->timeSpinBox->timeValue();
  else
    return ui->intSpin->value() / m_rate;
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
  rate = rate;
  ui->waveform->setSampleRate(rate / ui->intSpin->value());

  bool blocked = ui->averageTimeSpinBox->blockSignals(true);
  ui->averageTimeSpinBox->setTimeMin(1. / rate);
  ui->averageTimeSpinBox->setTimeMax(3600);
  ui->averageTimeSpinBox->setSampleRate(rate);
  ui->averageTimeSpinBox->setBestUnits(false);
  ui->averageTimeSpinBox->blockSignals(blocked);
}

void
RMSViewTab::feed(qreal timeStamp, qreal mag)
{
  bool firstTime = m_data.size() == 0;

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

  integrateMeasure(timeStamp, SCAST(SUFLOAT, mag));

  if (m_data.size() > 0) {
    QDateTime date;
    date.setSecsSinceEpoch(static_cast<qint64>(m_last));
    ui->lastLabel->setText("Last: " + date.toString());

    if (m_data.size() == 1) {
      m_first = m_last;
      if (!ui->dateTimeEdit->isEnabled())
        ui->dateTimeEdit->setDateTime(
              QDateTime::fromSecsSinceEpoch(m_first));

      refreshUi();

      ui->sinceLabel->setText("Since: " + date.toString());
    }

    if (firstTime) {
      qint64 width = ui->waveform->getVerticalAxisWidth();
      ui->waveform->zoomHorizontal(
            -width,
            static_cast<qint64>(ui->waveform->size().width()) - width);
    }
  }
}

void
RMSViewTab::setColorConfig(ColorConfig const &cfg)
{
  ui->waveform->setBackgroundColor(cfg.spectrumBackground);
  ui->waveform->setForegroundColor(cfg.spectrumForeground);
  ui->waveform->setAxesColor(cfg.spectrumAxes);
  ui->waveform->setTextColor(cfg.spectrumText);
  ui->waveform->setSelectionColor(cfg.selection);
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
  qreal first = m_data.size() > 0 ? m_first : SCAST(qreal, time(nullptr));

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

  if ((fp = fopen(path.toStdString().c_str(), "w")) == nullptr)
    throw std::runtime_error((
      "Failed to save data to MATLAB file: " + QString(strerror(errno))
    ).toStdString());

  fprintf(fp, "RATE=%.9f;\n", m_rate / ui->intSpin->value());
  fprintf(fp, "TIMESTAMP=%.6f;\n", m_first);
  fprintf(fp, "X=[\n");
  for (size_t i = 0; i < m_data.size(); ++i)
    fprintf(
          fp,
          "  %.9e, %.9f\n",
          SU_C_REAL(m_data[i]),
          SU_C_IMAG(m_data[i]));

  fprintf(fp, "];\n");
  fclose(fp);

  return true;
}

bool
RMSViewTab::saveToCSV(QString const &path)
{
  FILE *fp;

  if ((fp = fopen(path.toStdString().c_str(), "w")) == nullptr)
    throw std::runtime_error((
      "Failed to save data to CSV file: " + QString(strerror(errno))
    ).toStdString());

  for (size_t i = 0; i < m_data.size(); ++i)
    fprintf(
          fp,
          "%.9e;%.9f\n",
          SU_C_REAL(m_data[i]),
          SU_C_IMAG(m_data[i]));
  fclose(fp);

  return true;
}

bool
RMSViewTab::saveToNPY(QString const &path)
{
  FILE *fp = nullptr;
  bool ok = false;
  npy_file_t *npy = nullptr;
  std::string stdPath = path.toStdString();
  const char *strPath = stdPath.c_str();

  QString exception;

  if ((fp = fopen(strPath, "w")) == nullptr) {
    exception = "Failed to open NPY file: " + QString(strerror(errno));
    goto done;
  }

  if ((npy = npy_file_new(fp, NPY_DTYPE_FLOAT32, 1, 2, 0)) == nullptr) {
    exception = "Failed to initialize NPY headers: " + QString(strerror(errno));
    goto done;
  }

  if (!npy_file_write_float32(
        npy,
        reinterpret_cast<SUFLOAT *>(m_data.data()),
        m_data.size() * 2)) {
    exception = "Failed to write data to NPY file: " + QString(strerror(errno));
    goto done;
  }

  ok = true;

done:
  if (!ok)
    throw std::runtime_error(exception.toStdString());

  if (npy != nullptr)
    npy_file_destroy(npy);

  if (fp != nullptr)
    fclose(fp);

  return ok;
}

void
RMSViewTab::integrateMeasure(qreal timestamp, SUFLOAT mag)
{
  int intLen = ui->intSpin->value();

  m_energyAccum += mag;

  if (++m_accumCtr == intLen) {
    m_energyAccum /= intLen;
    m_data.push_back(
          m_energyAccum
          + SU_I * SU_ASFLOAT(SU_POWER_DB_RAW(m_energyAccum)));
    m_last = timestamp;
    m_accumCtr = 0;
    m_energyAccum = 0;
    ui->waveform->refreshData();
    if (ui->autoFitButton->isChecked())
      fitVertical();
    ui->waveform->invalidate();
  } else {
    if (m_haveCurrSamplePoint) {
      if (m_data.size() == 0) {
        mag = m_energyAccum / m_accumCtr;
      } else {
        SUFLOAT prev = SU_C_REAL(m_data[m_data.size() - 1]);
        mag = (prev * (intLen - m_accumCtr) + m_energyAccum) / intLen;
      }

      SUCOMPLEX curr = mag + SU_I * SU_ASFLOAT(SU_POWER_DB_RAW(mag));
      m_currSampleIterator->point = curr;
      m_currSampleIterator->t = m_data.size() * getCurrentTimeDelta();
      m_currSampleIterator = ui->waveform->refreshPoint(m_currSampleIterator);
    }
  }
}

bool
RMSViewTab::parseLine()
{
  SUFLOAT mag, db;
  QDateTime date;
  long sec;
  qreal usec;
  qreal rate;
  std::vector<std::string> fields;
  std::istringstream iss(m_line);

  // Description line allows commas and stuff
  if (m_line.compare(0, 5, "DESC,") == 0) {
    emit titleChanged(QString(m_line.c_str() + 5));
    return true;
  }

  for (std::string token; std::getline(iss, token, ','); )
    fields.push_back(std::move(token));

  if (fields.size() == 2) {
    if (fields[0] == "RATE") {
      if (sscanf(fields[1].c_str(), "%lf", &rate) < 1)
        return false;

      setSampleRate(rate);
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

    feed(SCAST(qreal, sec) + usec, SCAST(qreal, mag));

    return true;
  }

  return false;
}

void
RMSViewTab::processSocketData()
{
  char c;

  while (socket->bytesAvailable() > 0) {
    if (socket->read(&c, 1) < 1) {
      disconnectSocket();
      return;
    }

    if (c == '\n') {
      parseLine();
      m_line.clear();
    } else if (isprint(c)) {
      m_line += c;
      if (m_line.size() >= MAX_LINE_SIZE) {
        m_line.clear();
        disconnect();
        QMessageBox::critical(
              this,
              "Max line size exceeded",
              "Remote peer attempted to flood us. Preventively disconnected");
      }
    }
  }
}

void
RMSViewTab::disconnectSocket()
{
  if (socket != nullptr) {
    socket->close();
    delete socket;
    socket = nullptr;

    ui->stopButton->setEnabled(false);
    ui->stopButton->setChecked(false);
    ui->stopButton->setIcon(QIcon(":/icons/offline.png"));
  }
}

bool
RMSViewTab::doSave()
{
  QString fileName =
      QFileDialog::getSaveFileName(
        this,
        "Save data to MATLAB file",
        "power.npy",
        "NumPy data file (*.npy);;Comma-separated values (CSV) (*.csv);;MATLAB script (*.m)");

  if (fileName.size() == 0) {
    ui->timeSpinBox->setTimeValue(m_time);
    return false;
  }

  QFileInfo fi(fileName);

  QApplication::setOverrideCursor(Qt::WaitCursor);

  try {
    if (fi.suffix() == "m")
      saveToMatlab(fileName);
    else if (fi.suffix() == "csv")
      saveToCSV(fileName);
    else if (fi.suffix() == "npy")
      saveToNPY(fileName);
    else
      throw std::runtime_error("Unrecognized file format");
  } catch (std::runtime_error const &e) {
    QApplication::restoreOverrideCursor();

    QMessageBox::critical(this, "Save RMS data", QString(e.what()));
  }

  QApplication::restoreOverrideCursor();

  return true;
}

void
RMSViewTab::toggleModes(QObject *sender)
{
  if (sender == nullptr || sender == ui->autoScrollButton)
    ui->waveform->setAutoScroll(ui->autoScrollButton->isChecked());

  if (sender == nullptr || sender == ui->autoFitButton)
    ui->resetButton->setEnabled(!ui->autoFitButton->isChecked());

  if (sender == nullptr || sender == ui->dbButton) {
    if (ui->dbButton->isChecked()) {
      ui->waveform->setVerticalUnits("dB");
      ui->waveform->setRealComponent(false);
    } else {
      ui->waveform->setVerticalUnits("");
      ui->waveform->setRealComponent(true);
    }
  }

  if (sender == nullptr || sender == ui->autoFitButton) {
    ui->waveform->refreshData();
    ui->waveform->setAutoFitToEnvelope(ui->autoFitButton->isChecked());
    if (ui->autoFitButton->isChecked())
      fitVertical();
  }
}

void
RMSViewTab::setVerticalLimitsLinear(qreal min, qreal max)
{
  if (ui->dbButton->isChecked()) {
    setVerticalLimitsDb(SU_POWER_DB_RAW(min), SU_POWER_DB_RAW(max));
    return;
  }

  ui->waveform->zoomVertical(min, max);}

void
RMSViewTab::setVerticalLimitsDb(qreal min, qreal max)
{
  if (!ui->dbButton->isChecked()) {
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
RMSViewTab::fitVertical()
{
  if (ui->waveform->getDataLength() > 0) {
    SUCOMPLEX dataMin = ui->waveform->getDataMin();
    SUCOMPLEX dataMax = ui->waveform->getDataMax();
    qreal min, max;

    if (ui->dbButton->isChecked()) {
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

    ui->waveform->zoomVertical(min, max);
  }
}

bool
RMSViewTab::userClear(QString const &message)
{
  if (m_data.size() > 0) {
    auto reply = QMessageBox::question(
          this,
          "Clear current plot",
          message,
          QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel);
    if (reply == QMessageBox::StandardButton::Cancel) {
      ui->timeSpinBox->setTimeValue(m_time);
      return false;
    }

    if (reply == QMessageBox::StandardButton::Yes)
      return doSave();
  }

  return true;
}

void
RMSViewTab::connectAll()
{
  connect(
        &m_timer,
        SIGNAL(timeout()),
        this,
        SLOT(onTimeout()));

  if (socket != nullptr){
    connect(
          socket,
          SIGNAL(disconnected()),
          this,
          SLOT(onSocketDisconnected()));
  }

  connect(
        ui->stopButton,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleStartStop()));

  connect(
        ui->timeSpinBox,
        SIGNAL(changed(qreal, qreal)),
        this,
        SLOT(onTimeChanged(qreal, qreal)));

  connect(
        ui->averageTimeSpinBox,
        SIGNAL(changed(qreal, qreal)),
        this,
        SLOT(onAverageTimeChanged()));

  connect(
        ui->saveButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSave()));

  connect(
        ui->resetButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onResetZoom()));

  connect(
        ui->autoScrollButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleModes()));

  connect(
        ui->autoFitButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleModes()));

  connect(
        ui->dbButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleModes()));

  connect(
        ui->intSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onValueChanged(int)));

  connect(
        ui->waveform,
        SIGNAL(pointClicked(qreal, qreal, Qt::KeyboardModifiers)),
        this,
        SLOT(onPointClicked(qreal,qreal, Qt::KeyboardModifiers)));

  connect(
        ui->waveform,
        SIGNAL(toolTipAt(int, int, qreal, qreal)),
        this,
        SLOT(onToolTip(int,int,qreal,qreal)));

  connect(
        ui->timeScaleCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onTimeScaleChanged()));

  connect(
        ui->dateTimeEdit,
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
  disconnectSocket();
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
RMSViewTab::onSave()
{
  doSave();
}

void
RMSViewTab::onToggleStartStop()
{
  if (m_running != ui->stopButton->isChecked()) {
    if (m_running) {
      // Disconnect
      disconnectSocket();
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
RMSViewTab::onTimeout()
{
  if (socket != nullptr)
    processSocketData();
}

void
RMSViewTab::onToggleModes()
{
  QObject *sender = QObject::sender();

  toggleModes(sender);

  emit viewTypeChanged();
}

void
RMSViewTab::onResetZoom()
{
  fitVertical();
  ui->waveform->zoomHorizontalReset();
}

void
RMSViewTab::onSocketDisconnected()
{
  socket = nullptr;
  disconnectSocket();
}

void
RMSViewTab::onValueChanged(int)
{
  m_energyAccum = 0;
  m_accumCtr = 0;
  ui->sinceLabel->setText("Since: N/A");
  ui->lastLabel->setText("Last: N/A");
  m_data.clear();
  ui->waveform->setSampleRate(m_rate / ui->intSpin->value());
  ui->waveform->refreshData();
  if (ui->autoFitButton->isChecked())
    fitVertical();
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

  points = SCAST(int, ui->averageTimeSpinBox->timeValue() * m_rate);
  if (points < 1)
    points = 1;
  ui->averageTimeSpinBox->setTimeValue(points / m_rate);
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
