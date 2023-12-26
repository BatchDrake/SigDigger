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
#include <QElapsedTimer>

/* Local includes */
#include "AppConfig.h"
#include "UIMediator.h"

#define SIGDIGGER_AUTOSAVE_INTERVAL_MS (60 * 1000)

namespace SigDigger {
  class Scanner;
  class FileDataSaver;

  class DeviceDetectWorker : public QObject {
      Q_OBJECT

  public:
      DeviceDetectWorker();
      ~DeviceDetectWorker() override;

  public slots:
      void process();

  signals:
      void finished();

  private:
      Suscan::Singleton *m_instance = nullptr;
  };


  class Application : public QMainWindow {
    Q_OBJECT

    // Suscan core object
    std::unique_ptr<Suscan::Analyzer> m_analyzer = nullptr;

    bool m_profileSelected = false;
    unsigned int m_currSampleRate;
    bool m_filterInstalled = false;

    // UI
    AppUI m_ui;
    UIMediator *m_mediator = nullptr;
    QTimer m_uiTimer;
    QElapsedTimer m_cfgTimer;
    bool m_sourceInfoReceived = false;

    // Panoramic spectrum
    Scanner *m_scanner = nullptr;
    SUFREQ m_scanMinFreq;
    SUFREQ m_scanMaxFreq;

    // Rediscover devices
    QThread *m_deviceDetectThread;
    DeviceDetectWorker *m_deviceDetectWorker;

    // Private methods
    QString getLogText(int howMany = -1);
    void connectUI();
    void connectAnalyzer();
    void connectDeviceDetect();
    void connectScanner();

    void hotApplyProfile(Suscan::Source::Config const *);
    void orderedHalt();

  public:
    // Application methods
    Suscan::Object &&getConfig();
    void refreshConfig();
    void run(Suscan::Object const &config);

    void updateRecent();
    void startCapture();
    void restartCapture();
    void stopCapture();
    void setThrottleEnabled(bool);

    FileDataSaver *getSaver() const;

    explicit Application(QWidget *parent = nullptr);
    ~Application() override;


  protected:
    void closeEvent(QCloseEvent *event) override;

  signals:
    void detectDevices();
    void triggerSaveConfig();

  public slots:
    // UI Slots
    void onCaptureStart();
    void onCaptureStop();
    void onProfileChanged(bool);
    void onFrequencyChanged(qint64, qint64);
    void onSeek(struct timeval);
    void onDeviceRefresh();
    void onRecentSelected(QString profile);
    void onRecentCleared();
    void onTick();
    void quit();

    // Analyzer slots
    void onAnalyzerHalted();
    void onAnalyzerReadError();
    void onAnalyzerEos();
    void onPSDMessage(const Suscan::PSDMessage &);
    void onSourceInfoMessage(const Suscan::SourceInfoMessage &);
    void onStatusMessage(const Suscan::StatusMessage &);
    void onAnalyzerParams(const Suscan::AnalyzerParams &);

    // Device detect slots
    void onDetectFinished();

    // Panoramic spectrum slots
    void onPanSpectrumStart();
    void onPanSpectrumStop();
    void onPanSpectrumRangeChanged(qint64, qint64, bool);
    void onPanSpectrumSkipChanged();
    void onPanSpectrumRelBwChanged();
    void onPanSpectrumReset();
    void onPanSpectrumStrategyChanged(QString);
    void onPanSpectrumPartitioningChanged(QString);
    void onPanSpectrumGainChanged(QString, float);
    void onScannerUpdated();
    void onScannerStopped();
  };
}

#endif // APPLICATION_H
