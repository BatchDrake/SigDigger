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
#include "SigDiggerHelpers.h"
#include <SuWidgetsHelpers.h>
#include <Suscan/Analyzer.h>

#define FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING 2
#define FREQUENCY_EV_TICKS 10
#define FREQUENCY_AZ_TICKS 12
#define SAT_PATH_POINTS    20

using namespace SigDigger;

QPointF
FrequencyCorrectionDialog::azElToPoint(xyz_t const &p)
{
  qreal z = .5 * M_PI - p.elevation; // Zenith distance
  qreal k = 2 * this->azElAxesRadius / M_PI;

  return QPointF(
        this->azElCenterX - z * k * cos(-.5 * M_PI + p.azimuth),
        this->azElCenterY + z * k * sin(-.5 * M_PI + p.azimuth));
}

void
FrequencyCorrectionDialog::paintAzimuthElevationMap(QPixmap &pixmap)
{
  QPainter p(&pixmap);
  QPen pen(
        this->colors.constellationAxes,
        FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING);
  int i;


  if (azElAxesPixmap.size() != pixmap.size()) {
    this->azElAxesPixmap = QPixmap(pixmap.size());
    QPainter p(&this->azElAxesPixmap);
    QPointF center;
    qreal deltaEl;
    qreal deltaAz;
    xyz_t azEl = xyz_INITIALIZER;

    // Init parameters
    this->azElCenterX = .5 * pixmap.width();
    this->azElCenterY = .5 * pixmap.height();
    this->azElAxesRadius = .8 * MIN(this->azElCenterX, this->azElCenterY);

    azEl.elevation = .5 * M_PI;
    deltaEl  = this->azElAxesRadius / FREQUENCY_EV_TICKS;
    deltaAz = 2 * M_PI  / FREQUENCY_AZ_TICKS;
    center  = this->azElToPoint(azEl);

    p.fillRect(
          0, // x
          0, // y
          pixmap.width(),
          pixmap.height(),
          QBrush(this->colors.constellationBackground));

    // Paint cardinal points
    p.setPen(pen);
    azEl.azimuth = 0;
    azEl.elevation = -.5 * M_PI / FREQUENCY_EV_TICKS;

    this->paintTextAt(p, this->azElToPoint(azEl), "N", true);

    azEl.azimuth = .5 * M_PI;
    this->paintTextAt(p, this->azElToPoint(azEl), "E", true);

    azEl.azimuth = M_PI;
    this->paintTextAt(p, this->azElToPoint(azEl), "S", true);

    azEl.azimuth = 1.5 * M_PI;
    this->paintTextAt(p, this->azElToPoint(azEl), "W", true);

    // Draw elevation circles
    p.drawEllipse(center, this->azElAxesRadius, this->azElAxesRadius);

    pen.setStyle(Qt::DashLine);
    p.setPen(pen);

    // TODO: draw individual lines.
    for (i = 1; i < FREQUENCY_EV_TICKS; ++i)
      p.drawEllipse(center, i * deltaEl, i * deltaEl);

    // Draw azimuth lines
    azEl.elevation = 0;
    for (i = 0; i < FREQUENCY_AZ_TICKS; ++i) {
      azEl.azimuth = i * deltaAz;
      p.drawLine(QLineF(center, this->azElToPoint(azEl)));
    }
  }

  p.drawPixmap(0, 0, this->azElAxesPixmap);
}

void
FrequencyCorrectionDialog::paintTextAt(
    QPainter &p,
    QPointF where,
    QString text,
    bool center)
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

  if (center)
    rect.setRect(
          static_cast<int>(where.x() - .5 * tw),
          static_cast<int>(where.y() - .5 * th),
          tw,
          th);
  else
    rect.setRect(
        static_cast<int>(where.x()),
        static_cast<int>(where.y() - .5 * th),
        tw,
        th);

  p.setFont(font);
  p.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
}

void
FrequencyCorrectionDialog::findNewSatellites(void)
{
  auto sus = Suscan::Singleton::get_instance();

  if (this->ui->satCombo->count() != sus->getSatelliteMap().count()) {
    this->ui->satCombo->clear();
    for (auto p : sus->getSatelliteMap())
      this->ui->satCombo->addItem(p.nameToQString());

    if (this->desiredSelected != "") {
      this->setCurrentSatellite(this->desiredSelected);
    } else {
      if (this->ui->satCombo->currentIndex() > 0)
        this->setCurrentSatellite(this->ui->satCombo->currentText());
    }
  }

  this->ui->satRadio->setEnabled(this->ui->satCombo->count() > 0);

  if (this->ui->satCombo->count() > 0)
    this->ui->satRadio->setChecked(this->desiredFromSat);
}

void
FrequencyCorrectionDialog::paintAzimuthElevationPass(QPainter &p)
{
  qreal delta;
  struct timeval diff;
  struct timeval tdelta;
  struct timeval t;
  struct tm losTm, aosTm;
  SigDiggerHelpers *hlp = SigDiggerHelpers::instance();
  bool visible;
  bool haveSourceStart = false;
  bool haveSourceEnd = false;
  QVector<qreal> dashes;
  xyz_t pAzEl = {{0}, {0}, {0}};
  xyz_t firstAzel = {{0}, {0}, {0}};
  xyz_t azel;
  QPen pen(
        this->colors.constellationForeground,
        FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING);

  if (this->haveALOS) {
    qreal mkwidth =
          (this->azElAxesRadius / 80) * FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING;
    time_t lost = this->losTime.tv_sec;
    time_t aost = this->aosTime.tv_sec;
    time_t ssrc = this->startTime.tv_sec;
    time_t esrc = this->endTime.tv_sec;

    if (!this->realTime) {
      // Non-realtime signals have a defined start time and end time
      // We overlay this information on the azel plot indicating the
      // window in which the predictions will apply
      haveSourceStart = (aost < ssrc) && (ssrc <= lost);
      haveSourceEnd   = (aost < esrc) && (esrc <= lost);
    }

    // Convert to something printable
    hlp->pushLocalTZ();
    localtime_r(&lost, &losTm);
    localtime_r(&aost, &aosTm);
    hlp->popTZ();

    timersub(&this->losTime, &this->aosTime, &diff);

    delta = (static_cast<qreal>(diff.tv_sec)
             + 1e-6 * static_cast<qreal>(diff.tv_usec)) / SAT_PATH_POINTS;

    tdelta.tv_sec  = static_cast<time_t>(floor(delta));
    tdelta.tv_usec = static_cast<unsigned>(1e6 * (delta - floor(delta)));

    // Yay C++
    dashes << 3 << 4;

    t = this->aosTime;

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
      // Have we just left the satellite behind?
      if (visible && !timercmp(&this->timeStamp, &t, >)) {
        if (i > 0) {
          sgdp4_prediction_update(&this->prediction, &this->timeStamp);
          sgdp4_prediction_get_azel(&this->prediction, &azel);
          p.drawLine(QLineF(this->azElToPoint(pAzEl), this->azElToPoint(azel)));
        }

        // Back to the dashes. Also, this iteration does not count.
        --i;
        visible = false;
        pen.setColor(this->colors.constellationForeground);
        pen.setDashPattern(dashes);
        pen.setWidth(FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING);
        p.setPen(pen);
      } else { // No.
        sgdp4_prediction_update(&this->prediction, &t);
        sgdp4_prediction_get_azel(&this->prediction, &azel);
      }

      if (i > 0)
        p.drawLine(QLineF(this->azElToPoint(pAzEl), this->azElToPoint(azel)));
      else
        firstAzel = azel;

      pAzEl = azel;
      timeradd(&t, &tdelta, &t);
    }

    pen.setStyle(Qt::SolidLine);
    p.setPen(pen);

    this->paintTextAt(
          p,
          this->azElToPoint(firstAzel),
          QString::asprintf("%02u:%02u", aosTm.tm_hour, aosTm.tm_min));

    this->paintTextAt(
          p,
          this->azElToPoint(azel),
          QString::asprintf("%02u:%02u", losTm.tm_hour, losTm.tm_min));

    if (haveSourceStart) {
      sgdp4_prediction_update(&this->prediction, &this->startTime);
      sgdp4_prediction_get_azel(&this->prediction, &azel);

      p.setBrush(Qt::yellow);
      p.drawEllipse(
            this->azElToPoint(azel),
            mkwidth,
            mkwidth);
    }

    if (haveSourceEnd) {
      sgdp4_prediction_update(&this->prediction, &this->endTime);
      sgdp4_prediction_get_azel(&this->prediction, &azel);

      p.setBrush(Qt::yellow);
      p.drawEllipse(
            this->azElToPoint(azel),
            mkwidth,
            mkwidth);
    }
  }
}

void
FrequencyCorrectionDialog::paintAzimuthElevationSatPath(QPixmap &pixmap)
{
  QPainter p(&pixmap);
  xyz_t azel;

  QPen pen(
        this->colors.constellationForeground,
        FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING);

  if (this->haveOrbit) {
    qreal mkwidth =
          (this->azElAxesRadius / 40) * FREQUENCY_CORRECTION_DIALOG_OVERSAMPLING;

    this->paintAzimuthElevationPass(p);

    sgdp4_prediction_update(&this->prediction, &this->timeStamp);
    sgdp4_prediction_get_azel(&this->prediction, &azel);

    if (azel.elevation > 0) {
      p.setBrush(Qt::cyan);
      p.drawEllipse(
            this->azElToPoint(azel),
            mkwidth,
            mkwidth);

      pen.setColor(this->colors.spectrumText);
      p.setPen(pen);

      this->paintTextAt(
            p,
            this->azElToPoint(azel),
            "  " + QString(
              this->currentOrbit.name == nullptr
              ? "NO NAME"
              : this->currentOrbit.name));
    }
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

  if (this->ui->satCombo->count() == 0) {
    bool tleSig, satSig;
    tleSig = this->ui->tleRadio->signalsBlocked();
    satSig = this->ui->satRadio->signalsBlocked();

    this->ui->tleRadio->blockSignals(true);
    this->ui->satRadio->blockSignals(true);

    this->ui->tleRadio->setChecked(true);

    this->ui->tleRadio->blockSignals(tleSig);
    this->ui->satRadio->blockSignals(satSig);

    this->ui->satRadio->setEnabled(false);
  } else {
    this->ui->satRadio->setEnabled(false);
  }

  this->ui->satCombo->setEnabled(this->ui->satRadio->isChecked());
  this->ui->tleEdit->setEnabled(this->ui->tleRadio->isChecked());
}

void
FrequencyCorrectionDialog::refreshOrbit(void)
{
  auto sus = Suscan::Singleton::get_instance();

  // TLE-based correction
  if (this->ui->satRadio->isChecked()) {
    // Take TLE from satellite
    if (sus->getSatelliteMap().find(this->desiredSelected) !=
        sus->getLastSatellite()) {
      this->setCurrentOrbit(
            &sus->getSatelliteMap()[this->desiredSelected].getCOrbit());
    }
  } else {
    // Take TLE from textbox
    this->parseCurrentTLE();
  }
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
      SUDOUBLE window;
      SUDOUBLE searchWindow;
      time_t maxDelta;
      struct timeval delta, search;
      this->haveALOS = false;

      // Get current prediction
      if (!sgdp4_prediction_update(&this->prediction, &this->timeStamp))
        return;

      sgdp4_prediction_get_azel(&this->prediction, &azel);

      searchWindow = qBound(
            FREQUENCY_CORRECTION_DIALOG_TIME_WINDOW_MIN,  // 1 day
            3 * 86400.0 / this->currentOrbit.rev,
            FREQUENCY_CORRECTION_DIALOG_TIME_WINDOW_MAX); // 30 days

      window = searchWindow;

      if (azel.elevation > 0) {
        // For visible satellites, the strategy is as follows:
        //   1. In the current timeStamp, look for the next LOS event.
        //   2. Compute the lapse between the current timeStamp and the LOS
        //   3. Perform exponentially bigger backward time steps, of initial
        //      length equal to that length
        //   4. If the satellite is now invisible, assume that the next
        //      AOS event is the corresponding to the current pass.

        // Step 1
        if (!sgdp4_prediction_find_los(
              &this->prediction,
              &this->timeStamp,
              window,
              &this->losTime))
          return; // Something went wrong

        // Step 2
        timersub(&this->losTime, &this->timeStamp, &delta);
        delta.tv_sec += 1;

        // Step 3
        maxDelta = static_cast<time_t>(
              sgdp4_prediction_get_max_delta_t(&this->prediction) / COARSE_SEARCH_REL_STEP);

        do {
          timersub(&this->losTime, &delta, &search);

          delta.tv_sec += maxDelta;

          if (!sgdp4_prediction_update(&this->prediction, &search))
            break;

          sgdp4_prediction_get_azel(&this->prediction, &azel);
        } while (azel.elevation > 0
                 && static_cast<qreal>(delta.tv_sec) < searchWindow);

        // Check if the satellite now is belon the horizon
        if (azel.elevation > 0)
          return; // Nope. Something went wrong.

        // Okay, now if we find the next AOS, it should be our rise time
        this->haveALOS =
            sgdp4_prediction_find_aos(
              &this->prediction,
              &search,
              searchWindow,
              &this->aosTime);

        if (!this->haveALOS)
          return;

        search = this->aosTime;
        search.tv_sec += maxDelta;
      } else {
        // For non-visible satellites, the strategy is as follows:
        //   1. In the current timeStamp, look for the next AOS event.
        //   2. After that AOS, compute the next LOS event
        //   3. If both events were found, assume it belongs to the next pass
        this->haveALOS =
            sgdp4_prediction_find_aos(
              &this->prediction,
              &this->timeStamp,
              window,
              &this->aosTime);

        if (!this->haveALOS)
          return;

        search = this->aosTime;
        maxDelta = static_cast<time_t>(
              sgdp4_prediction_get_max_delta_t(&this->prediction));
        search.tv_sec += maxDelta;

        this->haveALOS = sgdp4_prediction_find_los(
              &this->prediction,
              &search,
              searchWindow,
              &this->losTime);
      }
    }

    sgdp4_prediction_update(&this->prediction, &this->timeStamp);
  }
}

void
FrequencyCorrectionDialog::setCurrentOrbit(const orbit_t *orbit)
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
    struct timeval delta;

    timersub(&tv, &this->timeStamp, &delta);

    // If the delta is suspiciously big, or backwards in time, recalculate
    // ALOS
    if (delta.tv_sec >= 1 || delta.tv_sec < 0 || delta.tv_usec < 0)
      this->haveALOS = false;

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

void
FrequencyCorrectionDialog::setTimeLimits(
    struct timeval const &start,
    struct timeval const &end)
{
  this->startTime = start;
  this->endTime   = end;

  this->haveALOS = false;
  this->updatePrediction();
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
            "deg",
            false));
    this->ui->elevationLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            SU_RAD2DEG(azel.elevation),
            0,
            "deg",
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

    if (orbit_is_geo(&this->currentOrbit))
      this->ui->periodLabel->setText("Geostationary");
    else if (orbit_is_decayed(&this->currentOrbit, &this->timeStamp))
      this->ui->periodLabel->setText("Decayed");
    else
      this->ui->periodLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            86400.0 / this->currentOrbit.rev,
            4,
            "s"));

    this->ui->eccLabel->setText(
            QString::asprintf("%g", this->currentOrbit.ecc));

    this->ui->incLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            SU_RAD2DEG(this->currentOrbit.eqinc),
            0,
            "deg"));

  } else {
    this->ui->visibleLabel->setText("N / A");
    this->ui->azimuthLabel->setText("N / A");
    this->ui->elevationLabel->setText("N / A");
    this->ui->dopplerLabel->setText("N / A");
    this->ui->speedLabel->setText("N / A");
    this->ui->periodLabel->setText("N / A");
    this->ui->eccLabel->setText("N / A");
    this->ui->incLabel->setText("N / A");
  }

  if (!this->haveALOS) {
    this->ui->nextEventLabel->setText("N / A");
  } else {
    if (timercmp(&this->timeStamp, &this->aosTime, <)) {
      timersub(&this->aosTime, &this->timeStamp, &diff);
      seconds = static_cast<qreal>(diff.tv_sec)
          + 1e-6 * static_cast<qreal>(diff.tv_usec);

      if (seconds < 1)
        this->ui->nextEventLabel->setText("Rising now");
      else
        this->ui->nextEventLabel->setText(
              "Rise in "
              + SuWidgetsHelpers::formatQuantity(seconds, "s"));
    } else {
      timersub(&this->losTime, &this->timeStamp, &diff);
      seconds = static_cast<qreal>(diff.tv_sec)
          + 1e-6 * static_cast<qreal>(diff.tv_usec);

      if (seconds < 1)
        this->ui->nextEventLabel->setText("Setting now");
      else
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
  this->refreshOrbit();
}

void
FrequencyCorrectionDialog::setCorrectionFromSatellite(bool enabled)
{
  this->desiredFromSat = enabled;

  if (this->ui->satCombo->count() > 0) {
    this->ui->satRadio->setChecked(enabled);
    this->ui->tleRadio->setChecked(!enabled);
  }

  this->refreshUiState();
  this->refreshOrbit();
}

void
FrequencyCorrectionDialog::setCurrentSatellite(QString sat)
{
  int ndx;

  this->desiredSelected = sat;

  if ((ndx = this->ui->satCombo->findText(sat)) > 0) {
    bool blocking = this->ui->satCombo->signalsBlocked();
    this->ui->satCombo->blockSignals(true);
    this->ui->satCombo->setCurrentIndex(ndx);
    this->ui->satCombo->blockSignals(blocking);
  }

  this->refreshOrbit();
}

void
FrequencyCorrectionDialog::setCurrentTLE(QString data)
{
  this->ui->tleEdit->setPlainText(data);
  this->refreshOrbit();
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
  this->findNewSatellites();
  this->refreshUiState();
}

void
FrequencyCorrectionDialog::setQth(xyz_t const &qth)
{
  this->rxSite = qth;
  this->haveQth = true;
  this->refreshOrbit();
}

void
FrequencyCorrectionDialog::parseCurrentTLE(void)
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

FrequencyCorrectionDialog::~FrequencyCorrectionDialog()
{
  if (this->haveOrbit) {
    sgdp4_prediction_finalize(&this->prediction);
    orbit_finalize(&this->currentOrbit);
  }

  delete ui;
}

////////////////////////////////////// Slots ///////////////////////////////////
void
FrequencyCorrectionDialog::onSwitchCorrectionType(void)
{
  this->refreshUiState();
  this->refreshOrbit();
}

void
FrequencyCorrectionDialog::onSwitchSatellite(void)
{
  if (this->ui->satCombo->currentIndex() > 0)
    this->setCurrentSatellite(this->ui->satCombo->currentText());
}

void
FrequencyCorrectionDialog::onToggleOrbitType(void)
{
  this->desiredFromSat = this->ui->satRadio->isChecked();

  this->refreshUiState();
  this->refreshOrbit();
}

void
FrequencyCorrectionDialog::onTLEEdit(void)
{
  this->refreshOrbit();
}

void
FrequencyCorrectionDialog::onTick(void)
{
  this->findNewSatellites();

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
