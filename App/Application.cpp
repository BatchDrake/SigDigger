//
//    Application.cpp: SigDigger main class
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

#include <QApplication>
#include <Suscan/Library.h>

#include "Application.h"
#include "Scanner.h"

#include <QMessageBox>
#include <SuWidgetsHelpers.h>

#include "MainSpectrum.h"

using namespace SigDigger;

DeviceDetectWorker::DeviceDetectWorker()
{
  m_instance = Suscan::Singleton::get_instance();
}

DeviceDetectWorker::~DeviceDetectWorker()
{

}

void
DeviceDetectWorker::process()
{
  m_instance->detect_devices();
  emit finished();
}

Application::Application(QWidget *parent) : QMainWindow(parent), m_ui(this)
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();

  sing->init_plugins();

  m_mediator = new UIMediator(this, &m_ui);
  m_deviceDetectThread = new QThread(this);
  m_deviceDetectWorker = new DeviceDetectWorker();
  m_deviceDetectWorker->moveToThread(m_deviceDetectThread);
  m_deviceDetectThread->start();
}

Suscan::Object &&
Application::getConfig()
{
  return m_mediator->getConfig()->serialize();
}

void
Application::refreshConfig()
{
  m_mediator->saveUIConfig();
}

void
Application::updateRecent()
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();

  m_mediator->clearRecent();
  for (auto p = sing->getFirstRecent(); p != sing->getLastRecent(); ++p)
    m_mediator->addRecent(*p);
  m_mediator->finishRecent();
}

void
Application::run(Suscan::Object const &config)
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();
  m_ui.postLoadInit(m_mediator, this);

  m_mediator->loadSerializedConfig(config);

  m_mediator->setState(UIMediator::HALTED);

  // New devices may have been discovered after config deserialization
  sing->refreshDevices();
  m_mediator->refreshDevicesDone();

  connectUI();
  connectDeviceDetect();
  updateRecent();

  show();

  m_uiTimer.start(100);
  m_cfgTimer.start();

  //mediator->notifyStartupErrors();
}


void
Application::connectUI()
{
  connect(
        m_mediator,
        SIGNAL(captureStart()),
        this,
        SLOT(onCaptureStart()));

  connect(
        m_mediator,
        SIGNAL(captureEnd()),
        this,
        SLOT(onCaptureStop()));

  connect(
        m_mediator,
        SIGNAL(profileChanged(bool)),
        this,
        SLOT(onProfileChanged(bool)));

  connect(
        m_mediator,
        SIGNAL(frequencyChanged(qint64, qint64)),
        this,
        SLOT(onFrequencyChanged(qint64, qint64)));

  connect(
        m_mediator,
        SIGNAL(seek(struct timeval)),
        this,
        SLOT(onSeek(struct timeval)));

  connect(
        m_mediator,
        SIGNAL(uiQuit()),
        this,
        SLOT(quit()));

  connect(
        m_mediator,
        SIGNAL(refreshDevices()),
        this,
        SLOT(onDeviceRefresh()));

  connect(
        m_mediator,
        SIGNAL(recentSelected(QString)),
        this,
        SLOT(onRecentSelected(QString)));

  connect(
        m_mediator,
        SIGNAL(recentCleared()),
        this,
        SLOT(onRecentCleared()));

  connect(
        m_mediator,
        SIGNAL(panSpectrumStart()),
        this,
        SLOT(onPanSpectrumStart()));

  connect(
        m_mediator,
        SIGNAL(panSpectrumRangeChanged(qint64, qint64, bool)),
        this,
        SLOT(onPanSpectrumRangeChanged(qint64, qint64, bool)));

  connect(
        m_mediator,
        SIGNAL(panSpectrumStop()),
        this,
        SLOT(onPanSpectrumStop()));

  connect(
        m_mediator,
        SIGNAL(panSpectrumSkipChanged()),
        this,
        SLOT(onPanSpectrumSkipChanged()));

  connect(
        m_mediator,
        SIGNAL(panSpectrumRelBwChanged()),
        this,
        SLOT(onPanSpectrumRelBwChanged()));

  connect(
        m_mediator,
        SIGNAL(panSpectrumReset()),
        this,
        SLOT(onPanSpectrumReset()));

  connect(
        m_mediator,
        SIGNAL(panSpectrumStrategyChanged(QString)),
        this,
        SLOT(onPanSpectrumStrategyChanged(QString)));

  connect(
        m_mediator,
        SIGNAL(panSpectrumPartitioningChanged(QString)),
        this,
        SLOT(onPanSpectrumPartitioningChanged(QString)));

  connect(
        m_mediator,
        SIGNAL(panSpectrumGainChanged(QString, float)),
        this,
        SLOT(onPanSpectrumGainChanged(QString, float)));

  connect(
        &m_uiTimer,
        SIGNAL(timeout()),
        this,
        SLOT(onTick()));

  connect(
        m_mediator,
        SIGNAL(triggerSaveConfig()),
        this,
        SIGNAL(triggerSaveConfig()));
}

void
Application::connectAnalyzer()
{
  connect(
        m_analyzer.get(),
        SIGNAL(halted()),
        this,
        SLOT(onAnalyzerHalted()));

  connect(
        m_analyzer.get(),
        SIGNAL(eos()),
        this,
        SLOT(onAnalyzerEos()));

  connect(
        m_analyzer.get(),
        SIGNAL(read_error()),
        this,
        SLOT(onAnalyzerReadError()));

  connect(
        m_analyzer.get(),
        SIGNAL(psd_message(const Suscan::PSDMessage &)),
        this,
        SLOT(onPSDMessage(const Suscan::PSDMessage &)));

  connect(
        m_analyzer.get(),
        SIGNAL(source_info_message(const Suscan::SourceInfoMessage &)),
        this,
        SLOT(onSourceInfoMessage(const Suscan::SourceInfoMessage &)));

  connect(
        m_analyzer.get(),
        SIGNAL(status_message(const Suscan::StatusMessage &)),
        this,
        SLOT(onStatusMessage(const Suscan::StatusMessage &)));

  connect(
        m_analyzer.get(),
        SIGNAL(analyzer_params(const Suscan::AnalyzerParams &)),
        this,
        SLOT(onAnalyzerParams(const Suscan::AnalyzerParams &)));
}

void
Application::connectScanner()
{
  connect(
        m_scanner,
        SIGNAL(spectrumUpdated()),
        this,
        SLOT(onScannerUpdated()));

  connect(
        m_scanner,
        SIGNAL(stopped()),
        this,
        SLOT(onScannerStopped()));
}

void
Application::connectDeviceDetect()
{
  connect(
        this,
        SIGNAL(detectDevices()),
        m_deviceDetectWorker,
        SLOT(process()));

  connect(
        m_deviceDetectWorker,
        SIGNAL(finished()),
        this,
        SLOT(onDetectFinished()));
}

QString
Application::getLogText(int howMany)
{
  QString text = "";
  std::lock_guard<Suscan::Logger> guard(*Suscan::Logger::getInstance());
  QStringList msgList;

  for (const auto &p : *Suscan::Logger::getInstance()) {
    switch (p.severity) {
      case SU_LOG_SEVERITY_CRITICAL:
        text += "critical: ";
        break;

      case SU_LOG_SEVERITY_DEBUG:
        text += "debug: ";
        break;

      case SU_LOG_SEVERITY_ERROR:
        text += "error: ";
        break;

      case SU_LOG_SEVERITY_INFO:
        text += "info: ";
        break;

      case SU_LOG_SEVERITY_WARNING:
        text += "warning: ";
        break;
    }

    msgList.append(p.message.c_str());
  }

  if (howMany < 0) {
    text = msgList.join("");
  } else {
    int first = msgList.size() - howMany;

    if (first < 0)
      first = 0;

    for (int i = first; i < msgList.size(); ++i)
      text += msgList[i];
  }

  return text;
}


void
Application::startCapture()
{
  auto iface = m_mediator->getProfile()->getInterface();

#ifdef _WIN32
  if (iface == SUSCAN_SOURCE_REMOTE_INTERFACE) {
    QMessageBox::critical(
          this,
          "SigDigger error",
          "Remote analyzers are not supported in Windows operating systems.\n\n"
          "This is not a SigDigger limitation, but a Windows one. Although "
          "proposals to circumvent this issue exist, they are inherently "
          "non-trivial and are not expected to be implemented any time soon.\n\n"
          "If you are a developer and are curious about the nature of this "
          "limitation (or even feel like helping me out addressing it), please "
          "feel free to e-mail me at BatchDrake@gmail.com",
          QMessageBox::Ok);
    mediator->refreshUI();
    return;
  }
#endif // _WIN32

  try {
    m_filterInstalled = false;

    if (m_mediator->getState() == UIMediator::HALTED) {
      Suscan::AnalyzerParams params = *m_mediator->getAnalyzerParams();
      std::unique_ptr<Suscan::Analyzer> analyzer;
      Suscan::Source::Config profile = *m_mediator->getProfile();

      if (profile.isRealTime()) {
        if (profile.getDecimatedSampleRate() > SIGDIGGER_MAX_SAMPLE_RATE) {
          unsigned decimate =
              static_cast<unsigned>(
                std::ceil(
                  profile.getSampleRate()
                  / static_cast<qreal>(SIGDIGGER_MAX_SAMPLE_RATE)));
          unsigned proposed =
              profile.getSampleRate() / decimate;
          QMessageBox::StandardButton reply
              = m_mediator->shouldReduceRate(
                  QString::fromStdString(profile.label()),
                  profile.getDecimatedSampleRate(),
                  proposed);

          // TODO: Maybe ask for decimation?
          if (reply == QMessageBox::Yes)
            profile.setDecimation(decimate);
          else if (reply == QMessageBox::Cancel) {
            m_mediator->setState(UIMediator::HALTED);
            return;
          }
        }
      }

      // Flush log messages from here
      Suscan::Logger::getInstance()->flush();

      // Allocate objects
      if (profile.instance == nullptr) {
        QMessageBox::warning(
                  this,
                  "SigDigger error",
                  "No source defined yet. Please define a source in the settings window.",
                  QMessageBox::Ok);
        return;
      }

      // Ensure we run this analyzer in channel mode.
      params.mode = Suscan::AnalyzerParams::Mode::CHANNEL;

      analyzer = std::make_unique<Suscan::Analyzer>(params, profile);

      m_sourceInfoReceived = false;

      // All set, move to application
      m_analyzer = std::move(analyzer);

      connectAnalyzer();

      m_mediator->setState(UIMediator::RUNNING, m_analyzer.get());
    }
  } catch (Suscan::Exception &) {
    QMessageBox::critical(
          this,
          "SigDigger error",
          "Failed to start capture due to errors:<p /><pre>"
          + getLogText(10).toHtmlEscaped()
          + "</pre>",
          QMessageBox::Ok);
    m_mediator->setState(UIMediator::HALTED);
  }
}

void
Application::orderedHalt()
{
  m_mediator->setState(UIMediator::HALTING);
  m_analyzer = nullptr;
  m_mediator->setState(UIMediator::HALTED);
}

void
Application::stopCapture()
{
  if (m_mediator->getState() == UIMediator::RUNNING) {
    m_mediator->setState(UIMediator::HALTING);
    m_analyzer.get()->halt();
  }
}

void
Application::restartCapture()
{
  if (m_mediator->getState() == UIMediator::RUNNING) {
    m_mediator->setState(UIMediator::RESTARTING);
    m_analyzer.get()->halt();
  }
}

void
Application::onAnalyzerHalted()
{
  bool restart = m_mediator->getState() == UIMediator::RESTARTING;

  orderedHalt();

  if (restart)
    startCapture();
}

void
Application::onAnalyzerEos()
{
  QMessageBox::information(
        this,
        "End of stream",
        "Capture interrupted due to stream end:<p /><pre>"
        + getLogText()
        + "</pre>",
        QMessageBox::Ok);

  orderedHalt();
}

void
Application::onPSDMessage(const Suscan::PSDMessage &msg)
{
  m_mediator->feedPSD(msg);
}

void
Application::onSourceInfoMessage(const Suscan::SourceInfoMessage &msg)
{
  m_mediator->notifySourceInfo(*msg.info());

  if (!m_sourceInfoReceived)
    m_sourceInfoReceived = true;
}

void
Application::onStatusMessage(const Suscan::StatusMessage &message)
{
  if (message.getCode() == SUSCAN_ANALYZER_INIT_FAILURE) {
    QMessageBox::critical(
          this,
          "Analyzer initialization",
          "Initialization failed: " + message.getMessage(),
          QMessageBox::Ok);
  } else {
    m_mediator->setStatusMessage(message.getMessage());
  }
}

void
Application::onAnalyzerParams(const Suscan::AnalyzerParams &params)
{
  m_mediator->setAnalyzerParams(params);
}

void
Application::onAnalyzerReadError()
{
  QMessageBox::critical(
        this,
        "Source error",
        "Capture stopped due to source read error. Last errors were:<p /><pre>"
        + getLogText()
        + "</pre>",
        QMessageBox::Ok);

  orderedHalt();
}

Application::~Application()
{
  m_uiTimer.stop();

  if (m_scanner != nullptr)
    delete m_scanner;

  m_analyzer = nullptr;

  m_deviceDetectThread->quit();
  m_deviceDetectThread->deleteLater();
  m_deviceDetectWorker->deleteLater();

  if (m_mediator != nullptr)
    delete m_mediator;
}

/////////////////////////////// Overrides //////////////////////////////////////
void
Application::closeEvent(QCloseEvent *)
{
  stopCapture();
}

//////////////////////////////// Slots /////////////////////////////////////////
void
Application::quit()
{
  stopCapture();
  QApplication::quit();
}

void
Application::onCaptureStart()
{
  startCapture();
}

void
Application::onCaptureStop()
{
  stopCapture();
}

void
Application::onProfileChanged(bool needsRestart)
{
  if (m_mediator->getProfile()->label() != "") {
    Suscan::Singleton *sing = Suscan::Singleton::get_instance();
    sing->notifyRecent(m_mediator->getProfile()->label());
    updateRecent();
  }

  if (needsRestart)
    restartCapture();
  else if (m_mediator->getState() == UIMediator::RUNNING)
    hotApplyProfile(m_mediator->getProfile());
}

#define TRYSILENT(x) \
  try { x; } catch (Suscan::Exception const &) { errorsOccurred = true; }

void
Application::onFrequencyChanged(qint64 freq, qint64 lnb)
{
  bool errorsOccurred = false;

  if (m_mediator->isLive()) {
    m_mediator->getProfile()->setFreq(freq);
    m_mediator->getProfile()->setLnbFreq(lnb);
  }

  if (m_mediator->getState() == UIMediator::RUNNING)
    TRYSILENT(m_analyzer->setFrequency(freq, lnb));
}

void
Application::hotApplyProfile(Suscan::Source::Config const *profile)
{
  bool errorsOccurred = false;
  auto sourceInfo = m_analyzer->getSourceInfo();

  if (sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_ANTENNA))
    TRYSILENT(m_analyzer->setAntenna(profile->getAntenna()));

  if (sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_BW))
    TRYSILENT(m_analyzer->setBandwidth(profile->getBandwidth()));

  if (sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_FREQ))
    TRYSILENT(m_analyzer->setFrequency(profile->getFreq(), profile->getLnbFreq()));

  if (sourceInfo.testPermission(SUSCAN_ANALYZER_PERM_SET_DC_REMOVE))
    TRYSILENT(m_analyzer->setDCRemove(profile->getDCRemove()));

  if (errorsOccurred) {
    QMessageBox::warning(
          this,
          "Update analyzer configuration",
          "Some of the settings in the profile could not be applied. See log window for details.",
          QMessageBox::Ok);
  }
}

void
Application::onSeek(struct timeval tv)
{
  if (m_mediator->getState() == UIMediator::RUNNING) {
    try {
      m_analyzer->seek(tv);
    } catch (Suscan::Exception &) {
      QMessageBox::critical(
            this,
            "SigDigger error",
            "Source does not allow seeking",
            QMessageBox::Ok);
    }
  }
}

void
Application::onDeviceRefresh()
{
  emit detectDevices();
}

void
Application::onDetectFinished()
{
  m_mediator->refreshDevicesDone();
}

void
Application::onRecentSelected(QString profile)
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();
  Suscan::Source::Config *config = sing->getProfile(profile.toStdString());

  if (config != nullptr) {
    bool forceStart = m_mediator->getState() == UIMediator::HALTED;
    m_mediator->setProfile(*config);
    if (forceStart)
      startCapture();
  } else {
    sing->removeRecent(profile.toStdString());
    QMessageBox::warning(
          this,
          "Failed to load recent profile",
          "Cannot load this profile. It was either renamed or deleted "
          "before the history was updated. The profile has been removed from history.",
              QMessageBox::Ok);
  }
}

void
Application::onRecentCleared()
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();

  sing->clearRecent();
}

void
Application::onPanSpectrumStart()
{
  if (m_scanner == nullptr) {
    qint64 freqMin;
    qint64 freqMax;
    Suscan::Source::Device device;

    if (m_mediator->getPanSpectrumRange(freqMin, freqMax)
        && m_mediator->getPanSpectrumDevice(device)) {
      Suscan::Source::Config config(
            "soapysdr",
            SUSCAN_SOURCE_FORMAT_AUTO);

      m_scanMinFreq = static_cast<SUFREQ>(freqMin);
      m_scanMaxFreq = static_cast<SUFREQ>(freqMax);

      config.setDevice(device);
      config.setAntenna(m_mediator->getPanSpectrumAntenna().toStdString());
      config.setSampleRate(
            static_cast<unsigned int>(
              m_mediator->getPanSpectrumPreferredSampleRate()));
      config.setDCRemove(true);
      config.setBandwidth(m_mediator->getPanSpectrumPreferredSampleRate());
      config.setLnbFreq(m_mediator->getPanSpectrumLnbOffset());
      config.setFreq(.5 * (m_scanMinFreq + m_scanMaxFreq));

      try {
        Suscan::Logger::getInstance()->flush();
        m_scanner = new Scanner(this, freqMin, freqMax, config);
        m_scanner->setRelativeBw(m_mediator->getPanSpectrumRelBw());
        m_scanner->setRttMs(m_mediator->getPanSpectrumRttMs());
        onPanSpectrumStrategyChanged(
              m_mediator->getPanSpectrumStrategy());
        onPanSpectrumPartitioningChanged(
              m_mediator->getPanSpectrumPartition());

        for (auto p = device.getFirstGain();
             p != device.getLastGain();
             ++p) {
          m_scanner->setGain(
                QString::fromStdString(p->getName()),
                m_mediator->getPanSpectrumGain(
                  QString::fromStdString(p->getName())));
        }

        connectScanner();
        Suscan::Logger::getInstance()->flush();
      } catch (Suscan::Exception &) {
        QMessageBox::critical(
              this,
              "SigDigger error",
              "Failed to start capture due to errors:<p /><pre>"
              + getLogText()
              + "</pre>",
              QMessageBox::Ok);
      }
    }
  }

  m_mediator->setPanSpectrumRunning(m_scanner != nullptr);
}

void
Application::onPanSpectrumStop()
{
  if (m_scanner != nullptr) {
    delete m_scanner;
    m_scanner = nullptr;
  }

  m_mediator->setPanSpectrumRunning(m_scanner != nullptr);
}

void
Application::onPanSpectrumRangeChanged(qint64 min, qint64 max, bool noHop)
{
  if (m_scanner != nullptr)
    m_scanner->setViewRange(min, max, noHop);
}

void
Application::onPanSpectrumSkipChanged()
{
  if (m_scanner != nullptr)
    m_scanner->setRttMs(m_mediator->getPanSpectrumRttMs());
}

void
Application::onPanSpectrumRelBwChanged()
{
  if (m_scanner != nullptr)
    m_scanner->setRelativeBw(m_mediator->getPanSpectrumRelBw());
}

void
Application::onPanSpectrumReset()
{
  if (m_scanner != nullptr) {
    m_scanner->flip();
    m_scanner->flip();
  }
}

void
Application::onPanSpectrumStrategyChanged(QString strategy)
{
  if (m_scanner != nullptr) {
    if (strategy.toStdString() == "Stochastic")
      m_scanner->setStrategy(Suscan::Analyzer::STOCHASTIC);
    else if (strategy.toStdString() == "Progressive")
      m_scanner->setStrategy(Suscan::Analyzer::PROGRESSIVE);
  }
}

void
Application::onPanSpectrumPartitioningChanged(QString partitioning)
{
  if (m_scanner != nullptr) {
    if (partitioning.toStdString() == "Continuous")
      m_scanner->setPartitioning(Suscan::Analyzer::CONTINUOUS);
    else if (partitioning.toStdString() == "Discrete")
      m_scanner->setPartitioning(Suscan::Analyzer::DISCRETE);
  }
}

void
Application::onPanSpectrumGainChanged(QString name, float value)
{
  if (m_scanner != nullptr)
    m_scanner->setGain(name, value);
}

void
Application::onScannerStopped()
{
  QString messages = getLogText();

  if (m_scanner != nullptr) {
    delete m_scanner;
    m_scanner = nullptr;
  }

  if (messages.size() > 0) {
    QMessageBox::warning(
          this,
          "Scanner stopped",
          "Running scanner has stopped. The error log was:<p /><pre>"
          + getLogText()
          + "</pre>",
          QMessageBox::Ok);
  }

  m_mediator->setPanSpectrumRunning(m_scanner != nullptr);
}

void
Application::onScannerUpdated()
{
  SpectrumView &view = m_scanner->getSpectrumView();

  m_mediator->setMinPanSpectrumBw(m_scanner->getFs());

  m_mediator->feedPanSpectrum(
        static_cast<quint64>(view.freqMin),
        static_cast<quint64>(view.freqMax),
        view.psd,
        SIGDIGGER_SCANNER_SPECTRUM_SIZE);
}

void
Application::onTick()
{
  if (m_mediator->getState() == UIMediator::RUNNING)
    m_mediator->notifyTimeStamp(m_analyzer->getSourceTimeStamp());

  if (m_cfgTimer.hasExpired(SIGDIGGER_AUTOSAVE_INTERVAL_MS)) {
    m_cfgTimer.restart();
    emit triggerSaveConfig();
  }
}
