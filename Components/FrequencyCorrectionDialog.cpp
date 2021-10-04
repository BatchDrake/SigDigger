//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
#include "FrequencyCorrectionDialog.h"
#include "ui_FrequencyCorrectionDialog.h"
#include <QPainter>
#include <SuWidgetsHelpers.h>
#include <Suscan/Analyzer.h>

#define FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING 2
#define FREQUENCY_EV_TICKS 10
#define FREQUENCY_AZ_TICKS 12
#define SAT_PATH_POINTS    20

using namespace SigDigger;

void
FrequencyCorrectionDialog::paintAzimuthElevationMap(QPixmap &pixmap)
{
  QPainter p(&pixmap);
  QPen pen(
        this->colors.constellationAxes,
        FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING);
  qreal x_c, y_c;
  qreal radius, deltaR, deltaAz = 2 * M_PI  / FREQUENCY_AZ_TICKS;
  int i;

  x_c = .5 * pixmap.width();
  y_c = .5 * pixmap.height();

  radius = .9 * MIN(x_c, y_c);
  deltaR = radius / FREQUENCY_EV_TICKS;

  if (azElAxesPixmap.size() != pixmap.size()) {
    azElAxesPixmap = QPixmap(pixmap.size());
    QPainter p(&this->azElAxesPixmap);

    p.fillRect(
          0, // x
          0, // y
          pixmap.width(),
          pixmap.height(),
          QBrush(this->colors.constellationBackground));

    // Draw elevation circles
    p.setPen(pen);
    p.drawEllipse(QPointF(x_c, y_c), radius, radius);

    pen.setStyle(Qt::DashLine);
    p.setPen(pen);

    for (i = 1; i < FREQUENCY_EV_TICKS; ++i)
      p.drawEllipse(QPointF(x_c, y_c), i * deltaR, i * deltaR);

    // Draw azimuth lines
    for (i = 0; i < FREQUENCY_AZ_TICKS; ++i)
      p.drawLine(QLineF(
            x_c,
            y_c,
            x_c + radius * cos(i * deltaAz),
            y_c + radius * sin(i * deltaAz)));
  }

  p.drawPixmap(0, 0, this->azElAxesPixmap);
}

void
FrequencyCorrectionDialog::paintTextAt(
    QPainter &p,
    int x,
    int y,
    QString text)
{
  int tw, th;
  QFont font;

  font.setPointSizeF(
        FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING * font.pointSizeF());

  QFontMetrics metrics(font);
  QRect rect;

  th = metrics.height();

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        tw = metrics.horizontalAdvance(text);
#else
        tw = metrics.width(text);
#endif // QT_VERSION_CHECK

  rect.setRect(x, y - th / 2, tw, th);
  p.setFont(font);
  p.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
}

void
FrequencyCorrectionDialog::paintAzimuthElevationSatPath(QPixmap &pixmap)
{
  QPainter p(&pixmap);
  qreal x_c, y_c;
  qreal radius, k;

  x_c = .5 * pixmap.width();
  y_c = .5 * pixmap.height();

  radius = .9 * MIN(x_c, y_c);

  QPen pen(
        this->colors.constellationForeground,
        FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING);


  if (this->haveALOS) {
    qreal delta, z = 0., prevZ;
    struct timeval diff;
    struct timeval tdelta;
    struct timeval t;
    struct tm losTm, aosTm;
    bool visible;
    QVector<qreal> dashes;
    xyz_t azel, prevAzel = {{0}, {0}, {0}};
    xyz_t firstAzel = {{0}, {0}, {0}};

    localtime_r(&this->losTime.tv_sec, &losTm);
    localtime_r(&this->aosTime.tv_sec, &aosTm);

    timersub(&this->losTime, &this->aosTime, &diff);

    delta = (static_cast<qreal>(diff.tv_sec)
             + 1e-6 * static_cast<qreal>(diff.tv_usec)) / SAT_PATH_POINTS;

    tdelta.tv_sec  = static_cast<time_t>(floor(delta));
    tdelta.tv_usec = static_cast<unsigned>(1e6 * (delta - floor(delta)));

    dashes << 3 << 4;

    t = this->aosTime;

    k = 2 * radius / M_PI;

    visible = timercmp(&this->timeStamp, &this->aosTime, >);

    if (visible) {
      pen.setStyle(Qt::SolidLine);
      pen.setWidth(FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING * 2);
    } else {
      pen.setDashPattern(dashes);
      pen.setWidth(FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING);
    }

    p.setPen(pen);

    for (auto i = 0; i <= SAT_PATH_POINTS; ++i) {
      if (visible && !timercmp(&this->timeStamp, &t, >)) {
        // Have we just left the satellite behind?
        int mkwidth = static_cast<int>(
              (radius / 40) * FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING);

        sgdp4_prediction_update(&this->prediction, &this->timeStamp);
        sgdp4_prediction_get_azel(&this->prediction, &azel);


        // Yes, paint it.
        prevZ = .5 * M_PI - prevAzel.elevation;
        z = .5 * M_PI - azel.elevation;

        p.drawLine(
              QLineF(
                x_c + prevZ * k * cos(prevAzel.azimuth),
                y_c + prevZ * k * sin(prevAzel.azimuth),
                x_c + z * k * cos(azel.azimuth),
                y_c + z * k * sin(azel.azimuth)));

        p.setBrush(Qt::cyan);
        p.drawEllipse(
              static_cast<int>(x_c + z * k * cos(azel.azimuth)) - mkwidth / 2,
              static_cast<int>(y_c + z * k * sin(azel.azimuth)) - mkwidth / 2,
              mkwidth,
              mkwidth);

        pen.setColor(this->colors.spectrumText);
        p.setPen(pen);

        this->paintTextAt(
              p,
              static_cast<int>(x_c + z * k * cos(azel.azimuth)),
              static_cast<int>(y_c + z * k * sin(azel.azimuth)),
              "  " + QString(this->currentOrbit.name == nullptr
              ? "NO NAME"
              : this->currentOrbit.name));


        // Back to the dashes. Also, this iteration does not count.
        --i;
        visible = false;
        pen.setColor(this->colors.constellationForeground);
        pen.setDashPattern(dashes);
        pen.setWidth(FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING);
        p.setPen(pen);
      } else {
        // No.
        sgdp4_prediction_update(&this->prediction, &t);
        sgdp4_prediction_get_azel(&this->prediction, &azel);
      }

      if (i > 0) {
        prevZ = .5 * M_PI - prevAzel.elevation;
        z     = .5 * M_PI - azel.elevation;

        p.drawLine(
              QLineF(
                x_c + prevZ * k * cos(prevAzel.azimuth),
                y_c + prevZ * k * sin(prevAzel.azimuth),
                x_c + z * k * cos(azel.azimuth),
                y_c + z * k * sin(azel.azimuth)));
      } else {
        firstAzel = azel;
      }

      prevAzel = azel;
      timeradd(&t, &tdelta, &t);
    }

    pen.setStyle(Qt::SolidLine);
    p.setPen(pen);

    prevZ = .5 * M_PI - firstAzel.elevation;

    this->paintTextAt(
          p,
          static_cast<int>(x_c + prevZ * k * cos(firstAzel.azimuth)),
          static_cast<int>(y_c + prevZ * k * sin(firstAzel.azimuth)),
          QString::asprintf("%02u:%02u", aosTm.tm_hour, aosTm.tm_min));

    this->paintTextAt(
          p,
          static_cast<int>(x_c + z * k * cos(azel.azimuth)),
          static_cast<int>(y_c + z * k * sin(azel.azimuth)),
          QString::asprintf("%02u:%02u", losTm.tm_hour, losTm.tm_min));
  }
}

void
FrequencyCorrectionDialog::repaintSatellitePlot(void)
{
  QPixmap pixmap(
        FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING *
          this->ui->satellitePlotLabel->width(),
        FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING *
          this->ui->satellitePlotLabel->height());

  this->paintAzimuthElevationMap(pixmap);
  this->paintAzimuthElevationSatPath(pixmap);

  this->ui->satellitePlotLabel->setPixmap(pixmap);
}

void
FrequencyCorrectionDialog::setColorConfig(ColorConfig const &colors)
{
  this->colors = colors;
  this->azElAxesPixmap = QPixmap();
  this->repaintSatellitePlot();
}

void
FrequencyCorrectionDialog::setFrequency(SUFREQ freq)
{
  this->centerFreq = freq;
  this->setWindowTitle(
        "Doppler corrections at "
        + SuWidgetsHelpers::formatQuantity(
          freq,
          4,
          "Hz"));
}

void
FrequencyCorrectionDialog::refreshUiState(void)
{
  this->ui->detailsStackedWidget->setCurrentIndex(
        this->ui->correctionTypeCombo->currentIndex());

  this->ui->satCombo->setEnabled(this->ui->satRadio->isChecked());
  this->ui->tleEdit->setEnabled(this->ui->tleRadio->isChecked());
}

void
FrequencyCorrectionDialog::recalcALOS(void)
{
  if (!this->haveOrbit) {
    this->haveALOS = false;
  } else {
    if (!this->haveALOS
        || timercmp(&this->timeStamp, &this->losTime, >=)) {
      xyz_t azel;
      sgdp4_prediction_update(&this->prediction, &this->timeStamp);
      sgdp4_prediction_get_azel(&this->prediction, &azel);

      if (azel.elevation > 0) {
        struct timeval delta, search;

        sgdp4_prediction_find_los(
              &this->prediction,
              &this->timeStamp,
              FREQUENCY_CORRECTION_DIALOG_TIME_WINDOW,
              &this->losTime);

        timersub(&this->losTime, &this->timeStamp, &delta);

        delta.tv_sec += 20 * 60;

        timersub(&this->timeStamp, &delta, &search);

        this->haveALOS =
            sgdp4_prediction_find_aos(
              &this->prediction,
              &search,
              FREQUENCY_CORRECTION_DIALOG_TIME_WINDOW,
              &this->aosTime)
            && sgdp4_prediction_find_los(
              &this->prediction,
              &this->aosTime,
              FREQUENCY_CORRECTION_DIALOG_TIME_WINDOW,
              &this->losTime);
      } else {
        this->haveALOS =
            sgdp4_prediction_find_aos(
              &this->prediction,
              &this->timeStamp,
              FREQUENCY_CORRECTION_DIALOG_TIME_WINDOW,
              &this->aosTime)
            && sgdp4_prediction_find_los(
              &this->prediction,
              &this->aosTime,
              FREQUENCY_CORRECTION_DIALOG_TIME_WINDOW,
              &this->losTime);
      }
    }
  }
}

void
FrequencyCorrectionDialog::setCurrentOrbit(orbit_t *orbit)
{
  if (this->haveOrbit) {
    if (&this->currentOrbit != orbit) {
      orbit_finalize(&this->currentOrbit);
      this->currentOrbit.name = nullptr;
    }
    sgdp4_prediction_finalize(&this->prediction);
    this->haveOrbit = false;
    this->haveALOS  = false;
  }

  if (orbit != nullptr) {
    if (&this->currentOrbit != orbit) {
      this->currentOrbit = *orbit;
      this->currentOrbit.name = orbit->name == nullptr
          ? nullptr
          : strdup(orbit->name);
    }

    if (this->haveQth
        && sgdp4_prediction_init(
          &this->prediction,
          &this->currentOrbit,
          &this->rxSite)) {
      this->haveOrbit = true;
    } else {
      orbit_finalize(&this->currentOrbit);
      this->currentOrbit.name = nullptr;
    }
  }

  this->updatePrediction();
}

void
FrequencyCorrectionDialog::setTimestamp(struct timeval const &tv)
{
  if (!this->realTime) {
    this->timeStamp = tv;
    this->updatePrediction();
  }
}

void
FrequencyCorrectionDialog::resetTimestamp(struct timeval const &tv)
{
  if (!this->realTime) {
    this->timeStamp = tv;
    this->haveALOS = false;
    this->updatePrediction();
  }
}

bool
FrequencyCorrectionDialog::isCorrectionEnabled(void) const
{
  return this->ui->correctionTypeCombo->currentIndex() > 0
      && this->haveOrbit;
}

bool
FrequencyCorrectionDialog::isCorrectionFromSatellite(void) const
{
  return this->ui->satRadio->isChecked();
}

QString
FrequencyCorrectionDialog::getCurrentSatellite(void) const
{
  return this->ui->satCombo->currentIndex() >= 0
      ? this->ui->satCombo->currentText()
      : "";
}

QString
FrequencyCorrectionDialog::getCurrentTLE(void) const
{
  return this->ui->tleEdit->toPlainText();
}

Suscan::Orbit
FrequencyCorrectionDialog::getOrbit(void) const
{
  if (this->haveOrbit) {
    return Suscan::Orbit(&this->currentOrbit);
  } else {
    return Suscan::Orbit();
  }
}

void
FrequencyCorrectionDialog::updatePrediction(void)
{
  struct timeval diff;
  qreal seconds;
  xyz_t azel, v_azel;

  if (this->haveOrbit) {
    this->recalcALOS();

    sgdp4_prediction_update(&this->prediction, &this->timeStamp);

    sgdp4_prediction_get_azel(&this->prediction, &azel);
    sgdp4_prediction_get_vel_azel(&this->prediction, &v_azel);

    this->ui->visibleLabel->setText(azel.elevation < 0 ? "No" : "Yes");
    this->ui->azimuthLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            SU_RAD2DEG(azel.azimuth),
            0,
            "º",
            false));
    this->ui->elevationLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            SU_RAD2DEG(azel.elevation),
            0,
            "º",
            false));
    this->ui->dopplerLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            -v_azel.distance * this->centerFreq / SPEED_OF_LIGHT_KM_S,
            4,
            "Hz",
            false));
    this->ui->speedLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            v_azel.distance * 1e3,
            4,
            "m/s",
            false));
  } else {
    this->ui->visibleLabel->setText("N / A");
    this->ui->azimuthLabel->setText("N / A");
    this->ui->elevationLabel->setText("N / A");
    this->ui->dopplerLabel->setText("N / A");
    this->ui->speedLabel->setText("N / A");
  }

  if (!this->haveALOS) {
    this->ui->nextEventLabel->setText("N / A");
  } else {
    if (timercmp(&this->timeStamp, &this->aosTime, <)) {
      timersub(&this->aosTime, &this->timeStamp, &diff);
      seconds = static_cast<qreal>(diff.tv_sec)
          + 1e-6 * static_cast<qreal>(diff.tv_usec);
      this->ui->nextEventLabel->setText(
            "Rise in "
            + SuWidgetsHelpers::formatQuantity(seconds, "s"));
    } else {
      timersub(&this->losTime, &this->timeStamp, &diff);
      seconds = static_cast<qreal>(diff.tv_sec)
          + 1e-6 * static_cast<qreal>(diff.tv_usec);
      this->ui->nextEventLabel->setText(
            "Set in "
            + SuWidgetsHelpers::formatQuantity(seconds, "s"));
    }
  }

  this->repaintSatellitePlot();
}

void
FrequencyCorrectionDialog::setRealTime(bool realTime)
{
  this->realTime = realTime;

  if (realTime) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    this->resetTimestamp(tv);
  } else {
    this->haveALOS = false;
  }
}

void
FrequencyCorrectionDialog::setCorrectionEnabled(bool enabled)
{
  this->ui->correctionTypeCombo->setCurrentIndex(enabled ? 1 : 0);
  this->refreshUiState();
}

void
FrequencyCorrectionDialog::setCorrectionFromSatellite(bool enabled)
{
  this->ui->satRadio->setChecked(enabled);
  this->ui->tleRadio->setChecked(!enabled);

  this->refreshUiState();
}

void
FrequencyCorrectionDialog::setCurrentSatellite(QString sat)
{
  int ndx;

  if ((ndx = this->ui->satCombo->findText(sat)) > 0)
    this->ui->satCombo->setCurrentIndex(ndx);
}

void
FrequencyCorrectionDialog::setCurrentTLE(QString data)
{
  this->ui->tleEdit->setPlainText(data);
}

void
FrequencyCorrectionDialog::connectAll(void)
{
  connect(
        this->ui->correctionTypeCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onSwitchCorrectionType(void)));

  connect(
        this->ui->satCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onSwitchSatellite(void)));

  connect(
        this->ui->satRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleOrbitType(void)));

  connect(
        this->ui->tleRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleOrbitType(void)));

  connect(
        this->ui->tleEdit,
        SIGNAL(textChanged(void)),
        this,
        SLOT(onTLEEdit(void)));

  connect(
        &this->timer,
        SIGNAL(timeout(void)),
        this,
        SLOT(onTick(void)));
}

FrequencyCorrectionDialog::FrequencyCorrectionDialog(
    QWidget *parent,
    SUFREQ centerFreq,
    ColorConfig const &colors) :
  QDialog(parent),
  colors(colors),
  ui(new Ui::FrequencyCorrectionDialog)
{

  ui->setupUi(this);
  this->connectAll();

  gettimeofday(&this->timeStamp, nullptr);

  this->timer.start(250);
  this->updatePrediction();
  this->setFrequency(centerFreq);
  this->refreshUiState();
}

void
FrequencyCorrectionDialog::setQth(xyz_t const &qth)
{
  this->rxSite = qth;
  this->haveQth = true;

  if (this->haveOrbit)
    this->setCurrentOrbit(&this->currentOrbit);
}

FrequencyCorrectionDialog::~FrequencyCorrectionDialog()
{
  delete ui;
}

////////////////////////////////////// Slots ///////////////////////////////////
void
FrequencyCorrectionDialog::onSwitchCorrectionType(void)
{
  this->refreshUiState();
}

void
FrequencyCorrectionDialog::onSwitchSatellite(void)
{

}

void
FrequencyCorrectionDialog::onToggleOrbitType(void)
{
  this->refreshUiState();
}

void
FrequencyCorrectionDialog::onTLEEdit(void)
{
  QString tleData = this->ui->tleEdit->toPlainText();
  std::string asStdString = tleData.toStdString();
  const char *str = asStdString.c_str();

  if (tleData.length() == 0) {
    this->ui->tleStatusLabel->setText("Please input a valid TLE set");
    this->setCurrentOrbit(nullptr);
  } else {
    orbit_t orbit = orbit_INITIALIZER;

    if (orbit_init_from_data(
          &orbit,
          str,
          static_cast<size_t>(asStdString.length())) > 0) {
      this->setCurrentOrbit(&orbit);
      orbit_finalize(&orbit);
      this->ui->tleStatusLabel->setText("TLE is valid");
    } else {
      this->ui->tleStatusLabel->setText("TLE has errors");
    }
  }
}

void
FrequencyCorrectionDialog::onTick(void)
{
  if (this->realTime) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    // Good timestamp to play around with the following TLES:
    //
    // ISS (ZARYA)
    // 1 25544U 98067A   21275.52277778  .00006056  00000-0  11838-3 0  9993
    // 2 25544  51.6451 172.0044 0004138  50.9000 316.9051 15.48905523305232

    // NOAA-19
    // 1 33591U 09005A   21275.30096120  .00000069  00000-0  62539-4 0  9997
    // 2 33591  99.1789 298.6299 0013801 188.9584 171.1343 14.12498277652005

    // tv.tv_sec = 1633202535 - 100;
    // tv.tv_usec = 0;

    this->timeStamp = tv;
    this->updatePrediction();
  }
}
