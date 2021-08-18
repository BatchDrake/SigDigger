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
#include <QMessageBox>
#include <BookmarkInfo.h>

#define SIGDIGGER_UI_MEDIATOR_DEFAULT_MIN_FREQ 0
#define SIGDIGGER_UI_MEDIATOR_DEFAULT_MAX_FREQ 6000000000

#define SIGDIGGER_UI_MEDIATOR_LOCAL_GRACE_PERIOD_MS  -1
#define SIGDIGGER_UI_MEDIATOR_REMOTE_GRACE_PERIOD_MS 1000

namespace Suscan {
  struct AnalyzerSourceInfo;
};

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
    std::map<std::string, QAction *> bandPlanMap;

    // UI Data
    Averager averager;
    unsigned int rate = 0;
    unsigned int recentCount = 0;

    // UI State
    State state = HALTED;
    bool settingRanges = false;

    // Private methods
    void connectMainWindow(void);
    void connectSpectrum(void);
    void connectSourcePanel(void);
    void connectFftPanel(void);
    void connectAudioPanel(void);
    void connectInspectorPanel(void);
    void connectDeviceDialog(void);
    void connectPanoramicDialog(void);

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
    void notifySourceInfo(Suscan::AnalyzerSourceInfo const &);

    // Recent list handling
    void clearRecent(void);
    void addRecent(std::string const &);
    void finishRecent(void);

    // Bandplan menu
    void addBandPlan(std::string const &);

    // Data methods
    void setProcessRate(unsigned int rate);
    void feedPSD(const Suscan::PSDMessage &msg);
    void setMinPanSpectrumBw(quint64 bw);
    void feedPanSpectrum(
        quint64 freqStart,
        quint64 freqEnd,
        float *data,
        size_t size);
    void setCaptureSize(quint64 size);
    void refreshDevicesDone(void);

    QMessageBox::StandardButton shouldReduceRate(
        QString const &label,
        unsigned int,
        unsigned int);

    void notifyStartupErrors(void);

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
    bool getAudioRecordState(void) const;
    std::string getAudioRecordSavePath(void) const;

    bool getPanSpectrumDevice(Suscan::Source::Device &) const;
    bool getPanSpectrumRange(qint64 &min, qint64 &max) const;
    unsigned int getPanSpectrumRttMs(void) const;
    float getPanSpectrumRelBw(void) const;
    float getPanSpectrumGain(QString const &) const;
    SUFREQ getPanSpectrumLnbOffset(void) const;
    float getPanSpectrumPreferredSampleRate(void) const;
    QString getPanSpectrumStrategy(void) const;
    QString getPanSpectrumPartition(void) const;
    unsigned int getFftSize(void) const;

    // Mediated setters
    void setAnalyzerParams(Suscan::AnalyzerParams const &params);
    void setStatusMessage(QString const &);
    void setRecordState(bool state);
    void setAudioRecordState(bool);
    void setAudioRecordSize(quint64 size);
    void setAudioRecordIORate(qreal rate);
    void setIORate(qreal rate);
    void saveUIConfig(void);
    void setProfile(Suscan::Source::Config const &config);
    void setPanSpectrumRunning(bool state);
    void resetRawInspector(qreal fs);
    void feedRawInspector(const SUCOMPLEX *, size_t size);

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
    void bookmarkAdded(BookmarkInfo);

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
    void ppmChanged(void);

    void saveStateChanged(void);
    void requestOpenInspector(void);
    void requestOpenRawInspector(void);
    void inspectorClosed(Suscan::Handle handle);
    void requestCloseRawInspector(void);

    void analyzerParamsChanged(void);
    void refreshDevices(void);
    void uiQuit(void);

    void recentSelected(QString);
    void recentCleared(void);

    void audioChanged(void);
    void audioVolumeChanged(float);
    void audioRecordStateChanged(void);

    void panSpectrumStart(void);
    void panSpectrumStop(void);
    void panSpectrumRangeChanged(qint64 min, qint64 max, bool);
    void panSpectrumSkipChanged(void);
    void panSpectrumRelBwChanged(void);
    void panSpectrumReset(void);
    void panSpectrumStrategyChanged(QString);
    void panSpectrumPartitioningChanged(QString);
    void panSpectrumGainChanged(QString, float);

  public slots:
    // Main Window slots
    void onTriggerSetup(bool);
    void onToggleCapture(bool);
    void onToggleFullScreen(bool);
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
    void onTriggerPanoramicSpectrum(bool);
    void onTriggerBandPlan(void);
    void onTriggerLogMessages(void);
    void onTriggerBackgroundTasks(void);
    void onAddBookmark(void);
    void onBookmarkAccepted(void);
    void onOpenBookmarkManager(void);
    void onJumpToBookmark(BookmarkInfo);
    void onBookmarkChanged(void);

    // Spectrum slots
    void onSpectrumBandwidthChanged(void);
    void onFrequencyChanged(qint64);
    void onLoChanged(qint64);
    void onRangeChanged(float, float);
    void onZoomChanged(float);
    void onNewBandPlan(QString);
    void onModulationChanged(QString);

    // Source panel
    void onToggleRecord(void);
    void onThrottleConfigChanged(void);
    void onGainChanged(QString name, float val);
    void onToggleDCRemove(void);
    void onToggleIQReverse(void);
    void onToggleAGCEnabled(void);
    void onAntennaChanged(QString name);
    void onBandwidthChanged(void);
    void onPPMChanged(void);

    // Fft Panel
    void onPaletteChanged(void);
    void onRangesChanged(void);
    void onAveragerChanged(void);
    void onFftSizeChanged(void);
    void onWindowFunctionChanged(void);
    void onRefreshRateChanged(void);
    void onTimeSpanChanged(void);
    void onTimeStampsChanged(void);
    void onBookmarksButtonChanged(void);
    void onGainChanged(float);
    void onZeroPointChanged(float);
    void onUnitChanged(QString, float, float);

    // Audio panel
    void onAudioChanged(void);

    // Inspector
    void onInspBandwidthChanged(void);
    void onOpenInspector(void);
    void onOpenRawInspector(void);
    void onCloseRawInspector(void);

    // Device dialog
    void onRefreshDevices(void);

    // Panoramic spectrum dialog
    void onPanoramicSpectrumStart(void);
    void onPanoramicSpectrumStop(void);
    void onPanoramicSpectrumDetailChanged(qint64 min, qint64 max, bool);
  };
};

#endif // APPLICATIONUI_H
