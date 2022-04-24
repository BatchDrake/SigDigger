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
#include <Suscan/Library.h>
#include <map>
#include <AppConfig.h>
#include <QMessageBox>
#include <WFHelpers.h>
#include <PersistentWidget.h>
#include <Averager.h>
#include <QMessageBox>

#define SIGDIGGER_UI_MEDIATOR_DEFAULT_MIN_FREQ  0
#define SIGDIGGER_UI_MEDIATOR_DEFAULT_MAX_FREQ  6000000000
#define SIGDIGGER_UI_MEDIATOR_PSD_CAL_LEN       10
#define SIGDIGGER_UI_MEDIATOR_PSD_MAX_LAG       .3
#define SIGDIGGER_UI_MEDIATOR_PSD_LAG_THRESHOLD 5e-3
#define SIGDIGGER_UI_MEDIATOR_LOCAL_GRACE_PERIOD_MS  -1
#define SIGDIGGER_UI_MEDIATOR_REMOTE_GRACE_PERIOD_MS 1000

namespace Suscan {
  struct AnalyzerSourceInfo;
};

namespace SigDigger {
  class UIComponent;
  class TabWidget;

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

    QMessageBox *laggedMsgBox = nullptr;
    std::map<std::string, QAction *> bandPlanMap;

    // Cached members
    bool isRealTime;
    struct timeval profileStart;
    struct timeval profileEnd;
    Suscan::Source::Device remoteDevice;

    // UI Data
    Averager averager;
    unsigned int rate = 0;
    unsigned int recentCount = 0;

    // UI State
    bool settingRanges = false;
    struct timeval rtMaxDelta = {0, 10000};
    struct timeval lastPsd;
    qreal psdDelta = 0;
    qreal psdAdj   = 0;
    bool haveRtDelta = false;
    unsigned int rtCalibrations = 0;
    qreal rtDeltaReal = 0;

    // Private methods
    void connectMainWindow();
    void connectTimeSlider();
    void connectSpectrum();
    void connectInspectorPanel();
    void connectDeviceDialog();
    void connectPanoramicDialog();

    // Behavioral methods
    void setSampleRate(unsigned int rate);
    void setBandwidth(unsigned int bandwidth);
    void refreshProfile(bool updateFreqs = true);
    void setCurrentAutoGain();

    // Refactored UI State
    State m_state = HALTED;
    Suscan::Object              *m_compConfig = nullptr;
    Suscan::Analyzer            *m_analyzer = nullptr;
    QList<UIComponent *>         m_components;
    QList<TabWidget *>           m_tabWidgets;
    QMap<TabWidget *, QDialog *> m_floatingTabs;

    // Refactored methods
    void addToolWidgets();
    void registerUIComponent(UIComponent *);
    void unregisterUIComponent(UIComponent *);

    friend class UIComponent;

  public:
    // Refactored methods
    QMainWindow  *getMainWindow() const;
    MainSpectrum *getMainSpectrum() const;
    Averager     *getSpectrumAverager();
    bool          addTabWidget(TabWidget *);
    bool          closeTabWidget(TabWidget *);
    bool          floatTabWidget(TabWidget *);

    void deserializeComponents(Suscan::Object const &conf);
    void serializeComponents(Suscan::Object &conf);

    void setState(enum State, Suscan::Analyzer *analyzer = nullptr);

    // UI State
    void refreshUI();
    State getState() const;
    void notifySourceInfo(Suscan::AnalyzerSourceInfo const &);
    void notifyTimeStamp(struct timeval const &timestamp);
    void notifyOrbitReport(Suscan::InspectorId, Suscan::OrbitReport const &report);
    void notifyDisableCorrection(Suscan::InspectorId);

    // Recent list handling
    void clearRecent();
    void addRecent(std::string const &);
    void finishRecent();

    // Bandplan menu
    void addBandPlan(std::string const &);

    // Data methods
    void feedPSD(const Suscan::PSDMessage &msg);
    void setMinPanSpectrumBw(quint64 bw);
    void feedPanSpectrum(
        quint64 freqStart,
        quint64 freqEnd,
        float *data,
        size_t size);
    void refreshDevicesDone();

    QMessageBox::StandardButton shouldReduceRate(
        QString const &label,
        unsigned int,
        unsigned int);

    void notifyStartupErrors();

    // Inspector handling
    Inspector *lookupInspector(Suscan::InspectorId id) const;
    Inspector *addInspector(
        Suscan::InspectorMessage const &msg,
        Suscan::InspectorId &oId);
    void unbindInspectorWidget(Inspector *insp);
    void closeInspector(Inspector *insp);
    void floatInspector(Inspector *insp);
    void detachAllInspectors();

    // Convenience getters
    Suscan::Source::Config *getProfile() const;
    Suscan::AnalyzerParams *getAnalyzerParams() const;
    bool getAudioRecordState() const;
    std::string getAudioRecordSavePath() const;
    bool isAudioDopplerCorrectionEnabled() const;
    Suscan::Orbit getAudioOrbit() const;

    bool getPanSpectrumDevice(Suscan::Source::Device &) const;
    bool getPanSpectrumRange(qint64 &min, qint64 &max) const;
    unsigned int getPanSpectrumRttMs() const;
    float getPanSpectrumRelBw() const;
    float getPanSpectrumGain(QString const &) const;
    SUFREQ getPanSpectrumLnbOffset() const;
    float getPanSpectrumPreferredSampleRate() const;
    QString getPanSpectrumStrategy() const;
    QString getPanSpectrumPartition() const;
    unsigned int getFftSize() const;

    // Mediated setters
    void setAnalyzerParams(Suscan::AnalyzerParams const &params);
    void setStatusMessage(QString const &);
    void saveUIConfig();
    void setProfile(Suscan::Source::Config const &config, bool restart = false);
    void setPanSpectrumRunning(bool state);
    void resetRawInspector(qreal fs);
    void feedRawInspector(const SUCOMPLEX *, size_t size);

    void setSourceTimeStart(struct timeval const &);
    void setSourceTimeEnd(struct timeval const &);
    void setTimeStamp(struct timeval const &);

    // Overriden methods
    Suscan::Serializable *allocConfig() override;
    void applyConfig() override;

    UIMediator(QMainWindow *owner, AppUI *ui);
    ~UIMediator() override;

  signals:
    void captureStart();
    void captureEnd();
    void profileChanged(bool);
    void colorsChanged(ColorConfig config);
    void bookmarkAdded(BookmarkInfo);

    void frequencyChanged(qint64, qint64);
    void loChanged(qint64);
    void channelBandwidthChanged(qreal bw);
    void seek(struct timeval tv);

    void requestOpenInspector();
    void requestOpenRawInspector();
    void inspectorClosed(Suscan::Handle handle);
    void requestCloseRawInspector();

    void analyzerParamsChanged();
    void refreshDevices();
    void uiQuit();

    void recentSelected(QString);
    void recentCleared();

    void audioChanged();
    void audioVolumeChanged(float);
    void audioRecordStateChanged();
    void audioSetCorrection(Suscan::Orbit);
    void audioDisableCorrection();

    void panSpectrumStart();
    void panSpectrumStop();
    void panSpectrumRangeChanged(qint64 min, qint64 max, bool);
    void panSpectrumSkipChanged();
    void panSpectrumRelBwChanged();
    void panSpectrumReset();
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
    void onQuickConnect();
    void onQuickConnectAccepted();
    void onTriggerStart(bool);
    void onTriggerStop(bool);
    void onTriggerImport(bool);
    void onTriggerExport(bool);
    void onTriggerDevices(bool);
    void onTriggerQuit(bool);
    void onTriggerClear(bool);
    void onTriggerRecent(bool);
    void onTriggerPanoramicSpectrum(bool);
    void onTriggerBandPlan();
    void onTriggerLogMessages();
    void onTriggerBackgroundTasks();
    void onAddBookmark();
    void onBookmarkAccepted();
    void onOpenBookmarkManager();
    void onJumpToBookmark(BookmarkInfo);
    void onBookmarkChanged();
    void onInspectorMenuRequested(const QPoint &);
    void onInspectorNameChanged();
    void onInspectorCloseRequested();
    void onInspectorDetachRequested();

    // Time Slider slots
    void onTimeStampChanged();

    // Spectrum slots
    void onSpectrumBandwidthChanged();
    void onFrequencyChanged(qint64);
    void onLoChanged(qint64);
    void onNewBandPlan(QString);

    // Inspector
    void onInspBandwidthChanged();
    void onOpenInspector();
    void onOpenRawInspector();
    void onCloseRawInspector();
    void onCloseInspectorWindow();

    // Device dialog
    void onRefreshDevices();

    // Panoramic spectrum dialog
    void onPanoramicSpectrumStart();
    void onPanoramicSpectrumStop();
    void onPanoramicSpectrumDetailChanged(qint64 min, qint64 max, bool);

    // UI Components
    void onCloseTabWindow();
    void onTabCloseRequested(int i);
    void onTabMenuRequested(const QPoint &);
  };
};

#endif // APPLICATIONUI_H
