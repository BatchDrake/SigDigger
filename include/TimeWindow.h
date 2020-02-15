//
//    TimeWindow.h: Time Window for time view operations
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
#ifndef TIMEWINDOW_H
#define TIMEWINDOW_H

#include <QMainWindow>
#include <sigutils/types.h>
#include "ColorConfig.h"
#include "Palette.h"

#define TIME_WINDOW_MAX_SELECTION 4096
#define TIME_WINDOW_MAX_DOPPLER_ITERS 200

namespace Ui {
  class TimeWindow;
}

namespace SigDigger {
  class TimeWindow : public QMainWindow
  {
    Q_OBJECT

    bool adjusting = false;

    SUCOMPLEX min;
    SUCOMPLEX max;
    SUCOMPLEX mean;
    qreal     fs;
    SUFREQ    centerFreq;
    SUFLOAT   rms;

    std::vector<Palette> palettes;
    int getPeriodicDivision(void) const;

    void connectAll(void);

    static void kahanMeanAndRms(
        SUCOMPLEX *mean,
        SUFLOAT *rms,
        const SUCOMPLEX *data,
        int length);

    static void calcLimits(
        SUCOMPLEX *oMin,
        SUCOMPLEX *oMax,
        const SUCOMPLEX *data,
        int length);

    static QString formatComplex(SUCOMPLEX const &z);
    static QString formatScientific(qreal real);
    static QString formatReal(qreal real);
    static QString formatIntegerPart(qreal real);
    static QString ensureRightExtension(QString const &path, QString const &ext);

    bool exportToMatlab(QString const &path, int start, int end);
    bool exportToWav(QString const &path, int start, int end);
    void recalcLimits(void);
    void refreshMeasures(void);
    void refreshUi(void);
    void deserializePalettes(void);
    void saveSamples(int start, int end);

  public:
    explicit TimeWindow(QWidget *parent = nullptr);
    ~TimeWindow();

    void setCenterFreq(SUFREQ center);
    void setData(std::vector<SUCOMPLEX> const &data, qreal fs);
    void setPalette(std::string const &);
    void setPaletteOffset(unsigned int);
    void setColorConfig(ColorConfig const &);

    std::string getPalette(void) const;
    unsigned int getPaletteOffset(void) const;

  signals:
    void configChanged();

  public slots:
    void onHZoom(qint64 min, qint64 max);
    void onVZoom(qreal min, qreal max);

    void onHSelection(qreal min, qreal max);
    void onVSelection(qreal min, qreal max);

    void onHoverTime(qreal);

    void onTogglePeriodicSelection(void);
    void onPeriodicDivisionsChanged(void);

    void onSaveAll(void);
    void onSaveSelection(void);
    void onFit(void);
    void onToggleAutoFit(void);
    void onZoomToSelection(void);
    void onZoomReset(void);
    void onShowWaveform(void);
    void onShowEnvelope(void);
    void onShowPhase(void);
    void onPhaseDerivative(void);
    void onPaletteChanged(int);
    void onChangePaletteOffset(int);

  private:
    Ui::TimeWindow *ui;
  };
}

#endif // TIMEWINDOW_H
