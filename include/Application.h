//
//    Application.h: SigDigger main class
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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <Suscan/Source.h>
#include <Suscan/Analyzer.h>

/* Local includes */
#include "AppConfig.h"
#include "UIMediator.h"
#include "AudioPlayback.h"
#include "AsyncDataSaver.h"

namespace SigDigger {
  class Application : public QMainWindow {
    Q_OBJECT

    // Suscan core object
    std::unique_ptr<Suscan::Analyzer> analyzer = nullptr;
    std::unique_ptr<AsyncDataSaver> dataSaver = nullptr;

    bool profileSelected = false;
    unsigned int currSampleRate;
    bool filterInstalled = false;

    // UI
    AppUI ui;
    UIMediator *mediator = nullptr;

    // Audio
    std::unique_ptr<AudioPlayback> playBack = nullptr;
    Suscan::Handle audioInspHandle = 0;
    unsigned int audioSampleRate = 0;
    suscan_config_t *audioCfgTemplate = nullptr;
    bool audioInspectorOpened = false;
    bool audioConfigured = false;
    SUFREQ maxAudioBw = SIGDIGGER_AUDIO_INSPECTOR_BANDWIDTH;
    SUFREQ lastAudioLo = 0;

    // Delayed audio parameters
    unsigned int delayedRate = 0;
    SUFLOAT delayedCutOff = 0;
    SUFLOAT delayedVolume = 0;
    unsigned int delayedDemod = 0;

    // Private methods
    QString getLogText(void);
    void connectUI(void);
    void connectAnalyzer(void);
    void connectDataSaver(void);
    int  openCaptureFile(void);
    void installDataSaver(int fd);
    void uninstallDataSaver(void);
    void setAudioInspectorParams(
        unsigned int rate,
        SUFLOAT cutOff,
        SUFLOAT volume,
        unsigned int demod);
    SUFREQ getAudioInspectorLo(void) const;
    SUFREQ getAudioInspectorBandwidth(void) const;
    void   assertAudioInspectorLo(void);

  public:
    // Application methods
    Suscan::Object &&getConfig(void);
    void refreshConfig(void);
    void run(Suscan::Object const &config);

    void startCapture(void);
    void restartCapture(void);
    void stopCapture(void);
    void setThrottleEnabled(bool);
    bool openAudio(unsigned int rate);
    void closeAudio(void);

    AsyncDataSaver *getSaver(void) const;

    explicit Application(QWidget *parent = nullptr);
    ~Application();


  protected:
    void closeEvent(QCloseEvent *event);

  public slots:
    // UI Slots
    void onCaptureStart(void);
    void onCaptureStop(void);
    void onProfileChanged(void);
    void onGainChanged(QString name, float val);
    void onFrequencyChanged(qint64);
    void onOpenInspector(void);
    void onThrottleConfigChanged(void);
    void onToggleRecord(void);
    void onToggleDCRemove(void);
    void onToggleIQReverse(void);
    void onToggleAGCEnabled(void);
    void onParamsChanged(void);
    void onLoChanged(qint64);
    void onChannelBandwidthChanged(qreal);
    void onAudioChanged(void);
    void onAntennaChanged(QString antenna);
    void onBandwidthChanged(void);

    // Analyzer slots
    void onAnalyzerHalted(void);
    void onAnalyzerReadError(void);
    void onAnalyzerEos(void);
    void onPSDMessage(const Suscan::PSDMessage &);
    void onInspectorMessage(const Suscan::InspectorMessage &);
    void onInspectorSamples(const Suscan::SamplesMessage &);

    // DataSaver slots
    void onSaveError(void);
    void onSaveSwamped(void);
    void onSaveRate(qreal rate);
    void onCommit(void);
  };
}

#endif // APPLICATION_H
