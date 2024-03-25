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
#include <Suscan/CancellableTask.h>
#include "ColorConfig.h"
#include "Palette.h"
#include "HistogramDialog.h"
#include "SamplerDialog.h"
#include "DopplerDialog.h"

#include "WaveSampler.h"

#define TIME_WINDOW_MAX_SELECTION     4096
#define TIME_WINDOW_MAX_DOPPLER_ITERS 200
#define TIME_WINDOW_SPEED_OF_LIGHT    3e8
#define TIME_WINDOW_EXTRA_WIDTH       72

namespace Ui {
  class TimeWindow;
}

namespace SigDigger {
  class TimeWindow : public QMainWindow
  {
    Q_OBJECT

    // Ui members
    HistogramDialog *m_histogramDialog = nullptr;
    SamplerDialog *m_samplerDialog = nullptr;
    DopplerDialog *m_dopplerDialog = nullptr;

    Ui::TimeWindow *ui = nullptr;

    bool m_hadSelectionBefore = true; // Yep. This must be true.
    bool m_adjusting = false;
    bool m_firstShow = true;

    qreal     m_fs = 0;
    qreal     m_bw = 0;

    std::vector<SUCOMPLEX> const *m_data = nullptr;
    const SUCOMPLEX *m_roDataPtr = nullptr;
    size_t           m_roDataLength = 0;

    std::vector<SUCOMPLEX> m_processedData;

    const SUCOMPLEX *m_displayDataPtr = nullptr;
    size_t           m_displayDataLength = 0;

    SUFREQ    m_centerFreq;

    bool m_taskRunning = false;

    Suscan::CancellableController m_taskController;

    int getPeriodicDivision() const;

    void connectFineTuneSelWidgets();
    void connectTransformWidgets();
    void connectAll();

    void refreshMeasures();
    void refreshUi();

    void notifyTaskRunning(bool);

    void carrierSyncNotifySelection(bool);
    void carrierSyncSetEnabled(bool);

    void samplingNotifySelection(bool, bool);
    void samplingSetEnabled(bool);

    bool fineTuneSenderIs(const QPushButton *sender) const;
    void fineTuneSelNotifySelection(bool);
    void fineTuneSelSetEnabled(bool);

    void getTransformRegion(
        const SUCOMPLEX * &origin,
        SUCOMPLEX *&destination,
        SUSCOUNT &length,
        bool selection);

    void populateSamplingProperties(SamplingProperties &prop);
    void startSampling();

    void setDisplayData(
        const SUCOMPLEX *displayData,
        size_t           displayLen,
        bool keepView = false);
    const SUCOMPLEX *getDisplayData() const;
    size_t getDisplayDataLength() const;

    static void adjustButtonToSize(
            QPushButton *button,
            QString text = "");

    const SUCOMPLEX *getData() const;
    size_t getLength() const;

  public:
    void closeEvent(QCloseEvent *event) override;

    explicit TimeWindow(QWidget *parent = nullptr);
    ~TimeWindow() override;

    void setCenterFreq(SUFREQ center);
    void setData(
        std::vector<SUCOMPLEX> const &data,
        qreal fs,
        qreal bw);
    void setData(
        const SUCOMPLEX *data,
        size_t size,
        qreal fs,
        qreal bw);
    void refresh();
    void setPalette(std::string const &);
    void setPaletteOffset(unsigned int);
    void setPaletteContrast(int);
    void setColorConfig(ColorConfig const &);

    void postLoadInit();

    std::string getPalette() const;
    unsigned int getPaletteOffset() const;
    int getPaletteContrast() const;

    void showEvent(QShowEvent *event) override;

  signals:
    void configChanged();
    void closed();

  public slots:
    void onHZoom(qint64 min, qint64 max);
    void onVZoom(qreal min, qreal max);

    void onHSelection(qreal min, qreal max);
    void onVSelection(qreal min, qreal max);

    void onHoverTime(qreal);

    void onTogglePeriodicSelection();
    void onPeriodicDivisionsChanged();

    void onSaveAll();
    void onSaveSelection();
    void onFit();
    void onToggleAutoFit();
    void onZoomToSelection();
    void onZoomReset();
    void onShowWaveform();
    void onShowEnvelope();
    void onShowPhase();
    void onPhaseDerivative();
    void onPaletteChanged(int);
    void onChangePaletteOffset(int);
    void onChangePaletteContrast(int);

    void onAbort();

    void onTaskCancelling();
    void onTaskProgress(qreal, QString);
    void onTaskDone();
    void onTaskCancelled();
    void onTaskError(QString);

    void onGuessCarrier();
    void onSyncCarrier();
    void onResetCarrier();

    void onTriggerHistogram();
    void onHistogramBlanked();
    void onHistogramSamples(const float *data, unsigned int len);

    void onTriggerSampler();
    void onResample();
    void onSampleSet(SigDigger::WaveSampleSet);

    void onCarrierSlidersChanged();

    void onFineTuneSelectionClicked();
    void onClkSourceButtonClicked();
    void onCalculateDoppler();

    void onCostasRecovery();
    void onPLLRecovery();
    void onCycloAnalysis();
    void onQuadDemod();
    void onAGC();
    void onLPF();
    void onDelayedConjugate();

    void onAGCRateChanged();
    void onDelayedConjChanged();

    void onWaveViewChanged();
    void onZeroCrossingComponentChanged();
    void onZeroPointChanged();
  };
}

#endif // TIMEWINDOW_H
