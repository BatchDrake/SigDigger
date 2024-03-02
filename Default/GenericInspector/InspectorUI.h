//
//    InspectorUI.h: Dynamic inspector UI
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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

#ifndef INSPECTORUI_H
#define INSPECTORUI_H

#include <QWidget>
#include <QVector>
#include <QThread>
#include <QMenu>
#include <memory>
#include <map>
#include "InspectorCtl/InspectorCtl.h"
#include <Suscan/SpectrumSource.h>
#include <Suscan/Analyzer.h>
#include <Suscan/Library.h>
#include <Suscan/Estimator.h>
#include <SNREstimator.h>
#include <sys/time.h>
#include <SocketForwarder.h>
#include <AbstractWaterfall.h>

#include "ThrottleableWidget.h"
#include "Decider.h"
#include "Palette.h"
#include "ColorConfig.h"
#include "DataSaverUI.h"
#include "FileDataSaver.h"
#include "NetForwarderUI.h"

#include "SymViewTab.h"
#include "TVProcessorTab.h"
#include "WaveformTab.h"
#include "FACTab.h"

namespace Ui {
  class Inspector;
}

#define SIGDIGGER_INSPECTOR_UI_DECISION_SPACE 0
#define SIGDIGGER_INSPECTOR_UI_SOFT_BITS      1
#define SIGDIGGER_INSPECTOR_UI_SOFT_BITS_I    2
#define SIGDIGGER_INSPECTOR_UI_SOFT_BITS_Q    3
#define SIGDIGGER_INSPECTOR_UI_SYMBOLS        4

namespace SigDigger {
  class FrequencyCorrectionDialog;
  class AppConfig;
  class EstimatorControl;
  struct GenericInspectorConfig;

  class InspectorUI : public QObject {
    Q_OBJECT

  public:
    enum State {
      DETACHED,
      ATTACHED
    };

    enum DataType {
      SYMBOL,
      FLOAT,
      COMPLEX
    };

  private:

    unsigned int basebandSampleRate;
    float sampleRate;

    bool recording = false;
    bool forwarding = false;
    bool adjusting = false;

    bool haveSpectrumLimits = false;

    unsigned int recordingRate = 0;
    unsigned int spectrumAdjustCounter = 0;

    // Inspector config
    GenericInspectorConfig *m_tabConfig = nullptr; // Weak
    Suscan::Config *config; // Weak
    QWidget *owner;
    bool haveQth = false;
    xyz_t qth;

    // Decider goes here
    unsigned int bps = 0;
    Decider decider;
    SNREstimator estimator;

    bool estimating = false;
    struct timeval last_estimator_update;
    std::vector<SUFLOAT>  floatBuffer;
    std::vector<SUFLOAT>  fftData;

    // UI objects
    AbstractWaterfall *wf = nullptr;
    ColorConfig colors;
    bool usingGlWf = false;
    bool usingMaxBlending = false;
    bool fallBackToWf = true;

    std::vector<Suscan::Estimator> estimators;
    std::vector<Suscan::SpectrumSource> spectsrcs;
    std::map<Suscan::EstimatorId, EstimatorControl *> estimatorCtls;

    Suscan::SpectrumUnit currentUnit;

    ThrottleControl throttle;
    Ui::Inspector *ui = nullptr;
    QString name;
    std::vector<InspectorCtl *> controls;
    DataSaverUI *saverUI = nullptr;
    NetForwarderUI *netForwarderUI = nullptr;
    FileDataSaver *dataSaver = nullptr;
    SocketForwarder *socketForwarder = nullptr;
    TVProcessorTab *tvTab = nullptr;
    FACTab *facTab = nullptr;
    WaveformTab *wfTab = nullptr;
    SymViewTab *symViewTab = nullptr;

    FrequencyCorrectionDialog *fcDialog = nullptr;

    State state = DETACHED;
    SUSCOUNT lastLen = 0;
    SUSCOUNT lastRate = 0;
    bool editingTVProcessorParams = false;

    void pushControl(InspectorCtl *ctl);
    void setBps(unsigned int bps);
    void connectAll(void);

    void initUi(void);
    unsigned int getBps(void) const;
    unsigned int getBaudRate(void) const;
    SUFLOAT getBaudRateFloat(void) const;
    std::string getClassName(void) const;
    void populateUnits(void);
    void populate(void);
    void connectWf(void);
    void connectGLWf(void);
    void makeWf(QWidget *owner);
    void connectDataSaver(void);
    void connectNetForwarder(void);
    void refreshSizes(void);
    std::string captureFileName(void) const;
    unsigned int getVScrollPageSize(void) const;
    unsigned int getHScrollOffset(void) const;
    void refreshVScrollBar(void) const;
    void refreshHScrollBar(void) const;
    void redrawMeasures(void);
    void addForwarderWidget(QWidget *widget);

    int fd = -1;

    float
    zeroPointToDb(void) const
    {
      return this->getZeroPoint() * this->currentUnit.dBPerUnit;
    }

    float
    dbToZeroPoint(float dB) const
    {
      return dB / this->currentUnit.dBPerUnit;
    }

    public:
      InspectorUI(
          QWidget *owner,
          QString name,
          GenericInspectorConfig *inspTabConfig,
          Suscan::Config *inspConfig,
          AppConfig const &appConfig);
      ~InspectorUI();

      QString
      getName(void) const
      {
        return this->name;
      }

      void feed(const SUCOMPLEX *data, unsigned int size);
      void feedSpectrum(const SUFLOAT *data, SUSCOUNT len, SUSCOUNT rate);
      void updateEstimator(Suscan::EstimatorId id, float val);
      void setQth(xyz_t const &);
      void setState(enum State state);
      void refreshUi(void);
      bool setPalette(std::string const &str);
      void addSpectrumSource(Suscan::SpectrumSource const &src);
      void addEstimator(Suscan::Estimator const &estimator);
      void setAppConfig(AppConfig const &cfg);
      void setZeroPoint(float);
      void setOrbitReport(Suscan::OrbitReport const &report);
      void notifyDisableCorrection(void);
      bool installDataSaver(void);
      void uninstallDataSaver(void);
      void setTunerFrequency(SUFREQ freq);
      void setRealTime(bool);
      void setTimeStamp(struct timeval const &);
      void setTimeLimits(
          struct timeval const &start,
          struct timeval const &end);
      bool installNetForwarder(void);
      void uninstallNetForwarder(void);
      void setBasebandRate(unsigned int);
      void setSampleRate(float rate);
      void setBandwidth(unsigned int bw);
      void beginReparenting(void);
      void doneReparenting(void);
      void setLo(int lo);
      void resetSpectrumLimits(void);
      void refreshInspectorCtls(void);
      unsigned int getBandwidth(void) const;
      int getLo(void) const;
      void adjustSizes(void);
      float getZeroPoint(void) const;
      enum State getState(void) const;

    public slots:
      void onInspectorControlChanged();
      void onAspectSliderChanged(int);
      void onPandapterRangeChanged(float, float);
      void onCPUBurnClicked(void);
      void onFPSReset(void);
      void onFPSChanged(void);
      void onSpectrumConfigChanged(void);
      void onSpectrumSourceChanged(void);
      void onToggleSNR(void);
      void onResetSNR(void);
      void onToggleRecord(void);
      void onToggleNetForward(void);
      void onChangeLo(void);
      void onChangeBandwidth(void);
      void onToggleEstimator(Suscan::EstimatorId, bool);
      void onApplyEstimation(QString, float);
      void onOpenDopplerSettings(void);
      void onDopplerAccepted(void);

      // Spectrum slots
      void onUnitChanged(void);
      void onZeroPointChanged(void);
      void onGainChanged(void);
      void onNewOffset(void);
      void onNewBandwidth(int, int);

      // Subcarrier slots
      void onScFrequencyChanged(void);
      void onScBandwidthChanged(void);
      void onScOpenInspector(void);

      // DataSaver slots
      void onSaveError(void);
      void onSaveSwamped(void);
      void onSaveRate(qreal rate);
      void onCommit(void);

      // Net Forwarder slots
      void onNetReady(void);
      void onNetError(void);
      void onNetSwamped(void);
      void onNetRate(qreal rate);
      void onNetCommit(void);

    signals:
      void configChanged(void);
      void setSpectrumSource(unsigned int index);
      void loChanged(void);
      void bandwidthChanged(void);
      void toggleEstimator(Suscan::EstimatorId, bool);
      void applyEstimation(QString, float);
      void disableCorrection(void);
      void setCorrection(Suscan::Orbit);
      void openInspector(
          QString inspClass,
          qint64 freq,
          qreal bw,
          bool precise);
  };
}

#endif // INSPECTORUI_H

