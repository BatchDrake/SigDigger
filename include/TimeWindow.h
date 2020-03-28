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
#include "SamplingProperties.h"
#include "CancellableTask.h"
#include "ColorConfig.h"
#include "Palette.h"
#include "HistogramDialog.h"
#include "SamplerDialog.h"

#include "WaveSampler.h"

#define TIME_WINDOW_MAX_SELECTION     4096
#define TIME_WINDOW_MAX_DOPPLER_ITERS 200
#define TIME_WINDOW_SPEED_OF_LIGHT    3e8

namespace Ui {
  class TimeWindow;
}

namespace SigDigger {
  class TimeWindow : public QMainWindow
  {
    Q_OBJECT

    // Ui members
    HistogramDialog *histogramDialog = nullptr;
    SamplerDialog *samplerDialog = nullptr;

    Ui::TimeWindow *ui = nullptr;

    bool hadSelectionBefore = true; // Yep. This must be true.
    bool adjusting = false;

    qreal     fs;

    std::vector<SUCOMPLEX> const *data;
    std::vector<SUCOMPLEX> processedData;

    std::vector<SUCOMPLEX> const *displayData = &processedData;

    SUCOMPLEX min;
    SUCOMPLEX max;
    SUCOMPLEX mean;

    SUFREQ    centerFreq;
    SUFLOAT   rms;

    CancellableController taskController;

    std::vector<Palette> palettes;
    int getPeriodicDivision(void) const;

    void connectFineTuneSelWidgets(void);
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

    bool exportToMatlab(QString const &path, int start, int end);
    bool exportToWav(QString const &path, int start, int end);
    void recalcLimits(void);
    void refreshMeasures(void);
    void refreshUi(void);
    void deserializePalettes(void);
    void saveSamples(int start, int end);

    void notifyTaskRunning(bool);

    void carrierSyncNotifySelection(bool);
    void carrierSyncSetEnabled(bool);

    void samplingNotifySelection(bool, bool);
    void samplingSetEnabled(bool);

    bool fineTuneSenderIs(const QPushButton *sender) const;
    void fineTuneSelNotifySelection(bool);
    void fineTuneSelSetEnabled(bool);

    void populateSamplingProperties(SamplingProperties &prop);
    void startSampling(void);

    void setDisplayData(
        std::vector<SUCOMPLEX> const *displayData,
        bool keepView = false);
    const SUCOMPLEX *getDisplayData(void) const;
    size_t getDisplayDataLength(void) const;

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

    void onAbort(void);

    void onTaskCancelling(void);
    void onTaskProgress(qreal, QString);
    void onTaskDone(void);
    void onTaskCancelled(void);
    void onTaskError(QString);

    void onGuessCarrier(void);
    void onSyncCarrier(void);
    void onResetCarrier(void);

    void onTriggerHistogram(void);
    void onHistogramSamples(const float *data, unsigned int len);

    void onTriggerSampler(void);
    void onResample(void);
    void onSampleSet(SigDigger::WaveSampleSet);

    void onCarrierSlidersChanged(void);

    void onFineTuneSelectionClicked(void);
    void onClkSourceButtonClicked(void);
  };
}

#endif // TIMEWINDOW_H
