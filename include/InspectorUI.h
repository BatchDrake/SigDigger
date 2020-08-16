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
#include <memory>
#include <map>
#include <InspectorCtl.h>
#include <Suscan/SpectrumSource.h>
#include <Suscan/Estimator.h>
#include <SNREstimator.h>
#include <sys/time.h>
#include <SocketForwarder.h>

#include "ThrottleableWidget.h"
#include "Decider.h"
#include "Palette.h"
#include "ColorConfig.h"
#include "DataSaverUI.h"
#include "FileDataSaver.h"
#include "EstimatorControl.h"
#include "NetForwarderUI.h"

#include "SymViewTab.h"
#include "TVProcessorTab.h"
#include "WaveformTab.h"

namespace Ui {
  class Inspector;
}

namespace SigDigger {
  class AppConfig;
  class MultitaskController;

  class InspectorUI : public QObject {
    Q_OBJECT

    public:
    enum State {
      DETACHED,
      ATTACHED
    };

    private:

    unsigned int basebandSampleRate;
    float sampleRate;

    bool recording = false;
    bool forwarding = false;
    bool adjusting = false;

    unsigned int recordingRate = 0;

    // Inspector config
    Suscan::Config *config; // Weak
    QWidget *owner;

    // Decider goes here
    unsigned int bps = 0;
    Decider decider;
    SNREstimator estimator;

    bool estimating = false;
    struct timeval last_estimator_update;
    std::vector<SUCOMPLEX> buffer;

    // UI objects
    std::vector<Suscan::Estimator> estimators;
    std::vector<Suscan::SpectrumSource> spectsrcs;
    std::map<Suscan::EstimatorId, EstimatorControl *> estimatorCtls;

    ThrottleControl throttle;
    Ui::Inspector *ui = nullptr;
    std::vector<InspectorCtl *> controls;
    DataSaverUI *saverUI = nullptr;
    NetForwarderUI *netForwarderUI = nullptr;
    FileDataSaver *dataSaver = nullptr;
    SocketForwarder *socketForwarder = nullptr;

    TVProcessorTab *tvTab = nullptr;
    WaveformTab *wfTab = nullptr;
    SymViewTab *symViewTab = nullptr;

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
    void populate(void);
    void connectDataSaver(void);
    void connectNetForwarder(void);
    void refreshSizes(void);
    std::string captureFileName(void) const;
    unsigned int getVScrollPageSize(void) const;
    unsigned int getHScrollOffset(void) const;
    void refreshVScrollBar(void) const;
    void refreshHScrollBar(void) const;

    int fd = -1;

    public:
      InspectorUI(
          QWidget *owner,
          Suscan::Config *config);
      ~InspectorUI();

      void feed(const SUCOMPLEX *data, unsigned int size);
      void feedSpectrum(const SUFLOAT *data, SUSCOUNT len, SUSCOUNT rate);
      void updateEstimator(Suscan::EstimatorId id, float val);

      void setMultitaskController(MultitaskController *mt);
      void setState(enum State state);
      void refreshUi(void);
      bool setPalette(std::string const &str);
      void addSpectrumSource(Suscan::SpectrumSource const &src);
      void addEstimator(Suscan::Estimator const &estimator);
      void setAppConfig(AppConfig const &cfg);

      bool installDataSaver(void);
      void uninstallDataSaver(void);

      bool installNetForwarder(void);
      void uninstallNetForwarder(void);

      void setBasebandRate(unsigned int);
      void setSampleRate(float rate);
      void setBandwidth(unsigned int bw);
      void setLo(int lo);
      void refreshInspectorCtls(void);
      unsigned int getBandwidth(void) const;
      int getLo(void) const;
      void adjustSizes(void);

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
      void onRangeChanged(void);
      void onToggleSNR(void);
      void onResetSNR(void);
      void onToggleRecord(void);
      void onToggleNetForward(void);
      void onChangeLo(void);
      void onChangeBandwidth(void);
      void onToggleEstimator(Suscan::EstimatorId, bool);
      void onApplyEstimation(QString, float);

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
  };
}

#endif // INSPECTORUI_H

