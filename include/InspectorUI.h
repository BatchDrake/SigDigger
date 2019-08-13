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
#include <memory>
#include <InspectorCtl.h>
#include <Suscan/SpectrumSource.h>
#include <Suscan/Estimator.h>
#include <SNREstimator.h>
#include <sys/time.h>

#include "ThrottleableWidget.h"
#include "Decider.h"
#include "Palette.h"
#include "ColorConfig.h"
#include "DataSaverUI.h"
#include "AsyncDataSaver.h"

namespace Ui {
  class Inspector;
}

namespace SigDigger {
  class InspectorUI : public QObject {
    Q_OBJECT

    public:
    enum State {
      DETACHED,
      ATTACHED
    };

    private:

    bool scrolling = false;
    bool demodulating = true;
    bool recording = false;
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
    std::vector<Palette> palettes;
    std::vector<Suscan::Estimator> estimators;
    std::vector<Suscan::SpectrumSource> spectsrcs;

    ThrottleControl throttle;
    Ui::Inspector *ui = nullptr;
    std::vector<InspectorCtl *> controls;
    DataSaverUI *saverUI = nullptr;
    std::unique_ptr<AsyncDataSaver> dataSaver = nullptr;

    State state = DETACHED;
    SUSCOUNT lastLen = 0;
    SUSCOUNT lastRate = 0;

    void pushControl(InspectorCtl *ctl);
    void setBps(unsigned int bps);
    unsigned int getBps(void) const;
    unsigned int getBaudRate(void) const;
    std::string getClassName(void) const;
    void populate(void);
    void connectDataSaver(void);

    std::string captureFileName(void) const;
    int fd = -1;

    public:
      InspectorUI(
          QWidget *owner,
          Suscan::Config *config);
      ~InspectorUI();

      void feed(const SUCOMPLEX *data, unsigned int size);
      void feedSpectrum(const SUFLOAT *data, SUSCOUNT len, SUSCOUNT rate);
      void setState(enum State state);
      void refreshUi(void);
      bool setPalette(std::string const &str);
      void addSpectrumSource(Suscan::SpectrumSource const &src);
      void addEstimator(Suscan::Estimator const &estimator);
      void setColors(ColorConfig const &colors);
      bool installDataSaver(void);
      void uninstallDataSaver(void);

      enum State getState(void) const;

    public slots:
      void onInspectorControlChanged();
      void onOffsetChanged(unsigned int);
      void onStrideChanged(unsigned int);
      void onScrollBarChanged(int val);
      void onCPUBurnClicked(void);
      void onFPSReset(void);
      void onFPSChanged(void);
      void onSymViewControlsChanged(void);
      void onSaveSymView(void);
      void onClearSymView(void);
      void onSpectrumConfigChanged(void);
      void onSpectrumSourceChanged(void);
      void onRangeChanged(void);
      void onToggleSNR(void);
      void onResetSNR(void);
      void onToggleRecord(void);

      // DataSaver slots
      void onSaveError(void);
      void onSaveSwamped(void);
      void onSaveRate(qreal rate);
      void onCommit(void);

    signals:
      void configChanged(void);
      void setSpectrumSource(unsigned int index);
  };
}

#endif // INSPECTORUI_H
