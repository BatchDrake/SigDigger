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
#include <QTimer>

/* Local includes */
#include "AppConfig.h"
#include "UIMediator.h"
#include "AudioPlayback.h"
#include "FileDataSaver.h"
#include "AudioFileSaver.h"
#include "Scanner.h"
#include <WFHelpers.h>

namespace SigDigger {
  class DeviceDetectWorker : public QObject {
      Q_OBJECT

  public:
      DeviceDetectWorker();
      ~DeviceDetectWorker();

  public slots:
      void process();

  signals:
      void finished();

  private:
      Suscan::Singleton *instance = nullptr;
  };


  class Application : public QMainWindow {
    Q_OBJECT

    // Suscan core object
    std::unique_ptr<Suscan::Analyzer> analyzer = nullptr;
    std::unique_ptr<FileDataSaver> dataSaver = nullptr;

    bool profileSelected = false;
    unsigned int currSampleRate;
    bool filterInstalled = false;

    // UI
    AppUI ui;
    UIMediator *mediator = nullptr;
    QTimer uiTimer;
    bool sourceInfoReceived = false;

    // Panoramic spectrum
    Scanner *scanner = nullptr;
    SUFREQ scanMinFreq;
    SUFREQ scanMaxFreq;

    // Rediscover devices
    QThread *deviceDetectThread;
    DeviceDetectWorker *deviceDetectWorker;

    // Private methods
    QString getLogText(void);
    void connectUI(void);
    void connectAnalyzer(void);
    void connectDeviceDetect(void);
    void connectScanner(void);

    void hotApplyProfile(Suscan::Source::Config const *);
    void orderedHalt(void);

  public:
    // Application methods
    Suscan::Object &&getConfig(void);
    void addComponentConfig(Suscan::Object &);
    void refreshConfig(void);
    void run(Suscan::Object const &config);

    void updateRecent(void);
    void startCapture(void);
    void restartCapture(void);
    void stopCapture(void);
    void setThrottleEnabled(bool);

    FileDataSaver *getSaver(void) const;

    explicit Application(QWidget *parent = nullptr);
    ~Application();


  protected:
    void closeEvent(QCloseEvent *event);

  signals:
    void detectDevices(void);

  public slots:
    // UI Slots
    void onCaptureStart(void);
    void onCaptureStop(void);
    void onProfileChanged(bool);
    void onGainChanged(QString name, float val);
    void onFrequencyChanged(qint64, qint64);
    void onSeek(struct timeval);
    void onParamsChanged(void);
    void onAntennaChanged(QString antenna);
    void onBandwidthChanged(void);
    void onPPMChanged(void);
    void onDeviceRefresh(void);
    void onRecentSelected(QString profile);
    void onRecentCleared(void);
    void onAddBookmark(BookmarkInfo info);
    void onTick(void);
    void quit(void);

    // Analyzer slots
    void onAnalyzerHalted(void);
    void onAnalyzerReadError(void);
    void onAnalyzerEos(void);
    void onPSDMessage(const Suscan::PSDMessage &);
    void onSourceInfoMessage(const Suscan::SourceInfoMessage &);
    void onStatusMessage(const Suscan::StatusMessage &);
    void onAnalyzerParams(const Suscan::AnalyzerParams &);

    // Device detect slots
    void onDetectFinished(void);

    // Panoramic spectrum slots
    void onPanSpectrumStart(void);
    void onPanSpectrumStop(void);
    void onPanSpectrumRangeChanged(qint64, qint64, bool);
    void onPanSpectrumSkipChanged(void);
    void onPanSpectrumRelBwChanged(void);
    void onPanSpectrumReset(void);
    void onPanSpectrumStrategyChanged(QString);
    void onPanSpectrumPartitioningChanged(QString);
    void onPanSpectrumGainChanged(QString, float);
    void onScannerUpdated(void);
    void onScannerStopped(void);
  };
}

#endif // APPLICATION_H
