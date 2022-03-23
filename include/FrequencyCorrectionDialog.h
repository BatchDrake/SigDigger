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
#ifndef FREQUENCYCORRECTIONDIALOG_H
#define FREQUENCYCORRECTIONDIALOG_H

#include <QDialog>
#include <Suscan/Library.h>
#include <sgdp4/sgdp4.h>
#include <ColorConfig.h>
#include <QTimer>

#define FREQUENCY_CORRECTION_DIALOG_TIME_WINDOW_MIN (1 * 86400.0)  // 1 day
#define FREQUENCY_CORRECTION_DIALOG_TIME_WINDOW_MAX (30 * 86400.0) // 30 days

namespace Ui {
  class FrequencyCorrectionDialog;
}

namespace Suscan {
  struct Orbit;
};

namespace SigDigger {
  class FrequencyCorrectionDialog : public QDialog
  {
    Q_OBJECT

    SUFREQ centerFreq = 0;
    sgdp4_prediction_t prediction;
    QString currentTle;
    orbit_t currentOrbit;
    QTimer timer;
    ColorConfig colors;

    bool haveOrbit = false;
    bool realTime  = true;
    bool haveALOS  = false;
    bool haveQth   = false;

    xyz_t  rxSite;
    struct timeval losTime;
    struct timeval aosTime;
    struct timeval startTime;
    struct timeval endTime;
    struct timeval timeStamp;

    QString desiredSelected;
    bool    desiredFromSat = true;
    QPixmap azElAxesPixmap;
    qreal   azElAxesRadius;
    qreal   azElCenterX;
    qreal   azElCenterY;

    QPointF azElToPoint(xyz_t const &p);
    void paintAzimuthElevationMap(QPixmap &pixmap);
    void paintAzimuthElevationPass(QPainter &p);
    void paintAzimuthElevationSatPath(QPixmap &pixmap);
    void repaintSatellitePlot(void);
    void parseCurrentTLE(void);
    void updatePrediction(void);
    void recalcALOS(void);
    void connectAll(void);
    void refreshUiState(void);
    void refreshOrbit(void);
    void findNewSatellites(void);

    void setCurrentOrbit(const orbit_t *);
    void paintTextAt(
        QPainter &p,
        QPointF where,
        QString text,
        bool center = false);

  public:
    // Setters
    void setColorConfig(ColorConfig const &colors);
    void setFrequency(SUFREQ freq);
    void setTimestamp(struct timeval const &timestamp);
    void resetTimestamp(struct timeval const &timestamp);
    void setTimeLimits(
        struct timeval const &start,
        struct timeval const &end);
    void setQth(xyz_t const &qth);
    void setRealTime(bool);
    void setCorrectionEnabled(bool);
    void setCorrectionFromSatellite(bool);
    void setCurrentSatellite(QString);
    void setCurrentTLE(QString);

    // Getters
    bool isCorrectionEnabled(void) const;
    bool isCorrectionFromSatellite(void) const;
    QString getCurrentSatellite(void) const;
    QString getCurrentTLE(void) const;

    Suscan::Orbit getOrbit(void) const;

    explicit FrequencyCorrectionDialog(
        QWidget *parent,
        SUFREQ centerFreq,
        ColorConfig const &);
    ~FrequencyCorrectionDialog();

  public slots:
    void onSwitchCorrectionType(void);
    void onSwitchSatellite(void);
    void onToggleOrbitType(void);
    void onTLEEdit(void);
    void onTick(void);

  private:
    Ui::FrequencyCorrectionDialog *ui;
  };
};

#endif // FREQUENCYCORRECTIONDIALOG_H
