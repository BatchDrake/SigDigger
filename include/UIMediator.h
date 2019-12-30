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
#ifndef APPLICATIONUI_H
#define APPLICATIONUI_H

#include <QMainWindow>
#include <Suscan/Messages/PSDMessage.h>
#include <map>
#include <AppConfig.h>

namespace SigDigger {

  class UIMediator : public PersistentWidget {
    Q_OBJECT

  public:
    enum State {
      HALTED,
      HALTING,
      RUNNING,
      RESTARTING
    };

  private:
    // Convenience pointers
    AppConfig *appConfig = nullptr;

    // Static part of UI
    QMainWindow *owner = nullptr;
    AppUI *ui = nullptr;
    QDockWidget *sourcePanelDock = nullptr;
    QDockWidget *inspectorPanelDock = nullptr;
    QDockWidget *fftPanelDock = nullptr;
    QDockWidget *audioPanelDock = nullptr;

    // UI Data
    Averager averager;
    unsigned int rate = 0;
    unsigned int recentCount = 0;

    // UI State
    State state = HALTED;

    // Private methods
    void connectMainWindow(void);
    void connectSpectrum(void);
    void connectSourcePanel(void);
    void connectFftPanel(void);
    void connectAudioPanel(void);
    void connectInspectorPanel(void);
    void connectDeviceDialog(void);

    void refreshUI(void);

    // Behavioral methods
    void setSampleRate(unsigned int rate);
    void setBandwidth(unsigned int bandwidth);
    void refreshProfile(void);

    void setCurrentAutoGain(void);
    static QString getInspectorTabTitle(Suscan::InspectorMessage const &msg);

  public:
    // UI State
    void setState(enum State);
    State getState(void) const;

    // Recent list handling
    void clearRecent(void);
    void addRecent(std::string const &);
    void finishRecent(void);

    // Data methods
    void setProcessRate(unsigned int rate);
    void feedPSD(const Suscan::PSDMessage &msg);
    void setCaptureSize(quint64 size);
    void refreshDevicesDone(void);

    // Inspector handling
    Inspector *lookupInspector(Suscan::InspectorId id) const;
    Inspector *addInspectorTab(
        Suscan::InspectorMessage const &msg,
        Suscan::InspectorId &oId);
    void closeInspectorTab(Inspector *insp);
    void detachAllInspectors(void);

    // Convenience getters
    Suscan::Source::Config *getProfile(void) const;
    Suscan::AnalyzerParams *getAnalyzerParams(void) const;
    unsigned int getFftSize(void) const;

    // Mediated setters
    void setRecordState(bool state);
    void setIORate(qreal rate);
    void saveGeometry(void);
    void setProfile(Suscan::Source::Config const &config);

    // Overriden methods
    Suscan::Serializable *allocConfig() override;
    void applyConfig(void) override;

    UIMediator(QMainWindow *owner, AppUI *ui);
    ~UIMediator() override;

  signals:
    void captureStart(void);
    void captureEnd(void);
    void profileChanged();
    void colorsChanged(ColorConfig config);

    void frequencyChanged(qint64, qint64);
    void loChanged(qint64);
    void channelBandwidthChanged(qreal bw);

    void toggleRecord(void);
    void throttleConfigChanged(void);
    void gainChanged(QString name, float val);
    void toggleIQReverse(void);
    void toggleDCRemove(void);
    void toggleAGCEnabled(void);
    void antennaChanged(QString);
    void bandwidthChanged(void);

    void saveStateChanged(void);
    void requestOpenInspector(void);
    void inspectorClosed(Suscan::Handle handle);
    void analyzerParamsChanged(void);
    void refreshDevices(void);
    void uiQuit(void);

    void recentSelected(QString);
    void recentCleared(void);
    void audioChanged(void);

  public slots:
    // Main Window slots
    void onTriggerSetup(bool);
    void onToggleCapture(bool);
    void onToggleAbout(bool);
    void onCloseInspectorTab(int index);
    void onTriggerStart(bool);
    void onTriggerStop(bool);
    void onTriggerImport(bool);
    void onTriggerExport(bool);
    void onTriggerDevices(bool);
    void onTriggerQuit(bool);
    void onTriggerClear(bool);
    void onTriggerRecent(bool);

    // Spectrum slots
    void onSpectrumBandwidthChanged(void);
    void onFrequencyChanged(qint64);
    void onLoChanged(qint64);
    void onRangeChanged(float, float);
    void onZoomChanged(float);

    // Source panel
    void onToggleRecord(void);
    void onThrottleConfigChanged(void);
    void onGainChanged(QString name, float val);
    void onToggleDCRemove(void);
    void onToggleIQReverse(void);
    void onToggleAGCEnabled(void);
    void onAntennaChanged(QString name);
    void onBandwidthChanged(void);

    // Fft Panel
    void onPaletteChanged(void);
    void onRangesChanged(void);
    void onAveragerChanged(void);
    void onFftSizeChanged(void);
    void onWindowFunctionChanged(void);
    void onRefreshRateChanged(void);
    void onTimeSpanChanged(void);

    // Audio panel
    void onAudioChanged(void);

    // Inspector
    void onInspBandwidthChanged(void);
    void onOpenInspector(void);

    // Device dialog
    void onRefreshDevices(void);
  };
};

#endif // APPLICATIONUI_H
