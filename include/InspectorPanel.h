//
//    InspectorPanel.h: Dockable inspector panel
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
#ifndef INSPECTORPANEL_H
#define INSPECTORPANEL_H

#include <PersistentWidget.h>
#include <TimeWindow.h>
#include <ColorConfig.h>

#define SIGDIGGER_DEFAULT_SQUELCH_TRIGGER  10
#define SIGDIGGER_DEFAULT_UPDATEUI_PERIOD_MS 250.

namespace Ui {
  class InspectorPanel;
}

namespace SigDigger {
  class MultitaskController;

  class InspectorPanelConfig : public Suscan::Serializable {
  public:
    bool collapsed = true;
    std::string inspectorClass = "psk";
    std::string palette = "Suscan";
    SUFLOAT autoSquelchTriggerSNR = SIGDIGGER_DEFAULT_SQUELCH_TRIGGER;
    unsigned int paletteOffset;
    int paletteContrast;
    bool precise = false;

    // Overriden methods
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class InspectorPanel : public PersistentWidget
  {
    Q_OBJECT

  public:
    enum State {
      DETACHED,
      ATTACHED
    };

  private:
    // Convenience pointer
    InspectorPanelConfig *panelConfig = nullptr;

    // UI objects
    Ui::InspectorPanel *ui = nullptr;
    // TODO: Allow multiple TimeWindows
    TimeWindow *timeWindow = nullptr;
    qreal timeWindowFs = 1;
    qint64 demodFreq = 0;
    SUFLOAT squelch = 0;
    SUFLOAT hangLevel = 0;
    bool autoSquelch = false;
    bool autoSquelchTriggered = false;

    // UI State
    State state = DETACHED;

    std::vector<SUCOMPLEX> data;
    std::vector<SUCOMPLEX> history;
    unsigned int historyPtr = 0;
    SUFLOAT  currEnergy = 0;
    SUFLOAT  powerAccum = 0;
    SUFLOAT  powerError = 0;
    SUSCOUNT hangCounter = 0;
    SUSCOUNT maxSamples = 0;
    SUSCOUNT hangLength = 0;
    SUSCOUNT powerSamples = 0;
    SUSCOUNT totalSamples = 0;
    SUSCOUNT uiRefreshSamples = 0;

    // Private methods
    void connectAll(void);
    void refreshUi(void);
    void enableAutoSquelch(void);
    void cancelAutoSquelch(void);
    void setInspectorClass(std::string const &cls);
    void refreshCaptureInfo(void);
    void openTimeWindow(void);
    void transferHistory(void);

  public:
    explicit InspectorPanel(QWidget *parent = nullptr);
    void postLoadInit(void);
    ~InspectorPanel() override;

    void setColorConfig(ColorConfig const &);
    void setDemodFrequency(qint64);
    void setBandwidthLimits(unsigned int min, unsigned int max);
    void setBandwidth(unsigned int freq);
    void setPrecise(bool precise);
    void setState(enum State state);

    void resetRawInspector(qreal sampleRate);
    void feedRawInspector(const SUCOMPLEX *data, size_t size);

    unsigned int getBandwidth(void) const;
    std::string getInspectorClass(void) const;
    bool getPrecise(void) const;
    enum State getState(void) const;

    // Overriden methods
    Suscan::Serializable *allocConfig(void) override;
    void applyConfig(void) override;
    bool event(QEvent *) override;

  public slots:
    void onOpenInspector(void);
    void onBandwidthChanged(double);
    void onPreciseChanged(void);
    void onPressHold(void);
    void onReleaseHold(void);

    void onPressAutoSquelch(void);
    void onReleaseAutoSquelch(void);
    void onToggleAutoSquelch(void);

    void onTimeWindowConfigChanged(void);
    void onTriggerSNRChanged(double val);

  signals:
    void bandwidthChanged(int);
    void requestOpenInspector(QString);
    void startRawCapture(void);
    void stopRawCapture(void);
  };
}

#endif // INSPECTORPANEL_H
