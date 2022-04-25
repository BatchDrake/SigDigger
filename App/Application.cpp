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

#include <QMessageBox>
#include <SuWidgetsHelpers.h>

#include "MainSpectrum.h"

using namespace SigDigger;

DeviceDetectWorker::DeviceDetectWorker()
{
  this->instance = Suscan::Singleton::get_instance();
}

DeviceDetectWorker::~DeviceDetectWorker()
{

}

void
DeviceDetectWorker::process()
{
  this->instance->detect_devices();
  emit finished();
}


Application::Application(QWidget *parent) : QMainWindow(parent), ui(this)
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();

  sing->init_plugins();

  this->mediator = new UIMediator(this, &this->ui);
  this->deviceDetectThread = new QThread(this);
  this->deviceDetectWorker = new DeviceDetectWorker();
  this->deviceDetectWorker->moveToThread(this->deviceDetectThread);
  this->deviceDetectThread->start();
}

Suscan::Object &&
Application::getConfig(void)
{
  return this->mediator->getConfig()->serialize();
}

void
Application::addComponentConfig(Suscan::Object &config)
{
  Suscan::Object object(SUSCAN_OBJECT_TYPE_OBJECT);

  this->mediator->serializeComponents(object);

  config.setField("Components", object);
}

void
Application::refreshConfig(void)
{
  this->mediator->saveUIConfig();
}

void
Application::updateRecent(void)
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();

  this->mediator->clearRecent();
  for (auto p = sing->getFirstRecent(); p != sing->getLastRecent(); ++p)
    this->mediator->addRecent(*p);
  this->mediator->finishRecent();
}

void
Application::run(Suscan::Object const &config)
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();
  this->ui.postLoadInit(this);

  // Order is important here. First deserialize component, then load
  try {
    this->mediator->deserializeComponents(config.getField("Components"));
  } catch (Suscan::Exception const &) { }

  this->mediator->loadSerializedConfig(config);

  this->mediator->setState(UIMediator::HALTED);

  // New devices may have been discovered after config deserialization
  sing->refreshDevices();
  this->mediator->refreshDevicesDone();

  this->connectUI();
  this->connectDeviceDetect();
  this->updateRecent();

  this->show();

  this->uiTimer.start(250);

  //this->mediator->notifyStartupErrors();
}

FileDataSaver *
Application::getSaver(void) const
{
  return this->dataSaver.get();
}

void
Application::connectUI(void)
{
  connect(
        this->mediator,
        SIGNAL(captureStart()),
        this,
        SLOT(onCaptureStart()));

  connect(
        this->mediator,
        SIGNAL(captureEnd()),
        this,
        SLOT(onCaptureStop()));

  connect(
        this->mediator,
        SIGNAL(profileChanged(bool)),
        this,
        SLOT(onProfileChanged(bool)));

  connect(
        this->mediator,
        SIGNAL(frequencyChanged(qint64, qint64)),
        this,
        SLOT(onFrequencyChanged(qint64, qint64)));

  connect(
        this->mediator,
        SIGNAL(seek(struct timeval)),
        this,
        SLOT(onSeek(struct timeval)));

  connect(
      this->mediator,
        SIGNAL(analyzerParamsChanged(void)),
        this,
        SLOT(onParamsChanged(void)));

  connect(
        this->mediator,
        SIGNAL(uiQuit(void)),
        this,
        SLOT(quit(void)));

  connect(
        this->mediator,
        SIGNAL(refreshDevices(void)),
        this,
        SLOT(onDeviceRefresh(void)));

  connect(
        this->mediator,
        SIGNAL(recentSelected(QString)),
        this,
        SLOT(onRecentSelected(QString)));

  connect(
        this->mediator,
        SIGNAL(recentCleared(void)),
        this,
        SLOT(onRecentCleared(void)));

  connect(
        this->mediator,
        SIGNAL(panSpectrumStart(void)),
        this,
        SLOT(onPanSpectrumStart(void)));

  connect(
        this->mediator,
        SIGNAL(panSpectrumRangeChanged(qint64, qint64, bool)),
        this,
        SLOT(onPanSpectrumRangeChanged(qint64, qint64, bool)));

  connect(
        this->mediator,
        SIGNAL(panSpectrumStop(void)),
        this,
        SLOT(onPanSpectrumStop(void)));

  connect(
        this->mediator,
        SIGNAL(panSpectrumSkipChanged(void)),
        this,
        SLOT(onPanSpectrumSkipChanged(void)));

  connect(
        this->mediator,
        SIGNAL(panSpectrumRelBwChanged(void)),
        this,
        SLOT(onPanSpectrumRelBwChanged(void)));

  connect(
        this->mediator,
        SIGNAL(panSpectrumReset(void)),
        this,
        SLOT(onPanSpectrumReset(void)));

  connect(
        this->mediator,
        SIGNAL(panSpectrumStrategyChanged(QString)),
        this,
        SLOT(onPanSpectrumStrategyChanged(QString)));

  connect(
        this->mediator,
        SIGNAL(panSpectrumPartitioningChanged(QString)),
        this,
        SLOT(onPanSpectrumPartitioningChanged(QString)));

  connect(
        this->mediator,
        SIGNAL(panSpectrumGainChanged(QString, float)),
        this,
        SLOT(onPanSpectrumGainChanged(QString, float)));

  connect(
        this->mediator,
        SIGNAL(bookmarkAdded(BookmarkInfo)),
        this,
        SLOT(onAddBookmark(BookmarkInfo)));

  connect(
        &this->uiTimer,
        SIGNAL(timeout(void)),
        this,
        SLOT(onTick(void)));
}

void
Application::connectAnalyzer(void)
{
  connect(
        this->analyzer.get(),
        SIGNAL(halted(void)),
        this,
        SLOT(onAnalyzerHalted(void)));

  connect(
        this->analyzer.get(),
        SIGNAL(eos(void)),
        this,
        SLOT(onAnalyzerEos(void)));

  connect(
        this->analyzer.get(),
        SIGNAL(read_error(void)),
        this,
        SLOT(onAnalyzerReadError(void)));

  connect(
        this->analyzer.get(),
        SIGNAL(psd_message(const Suscan::PSDMessage &)),
        this,
        SLOT(onPSDMessage(const Suscan::PSDMessage &)));

  connect(
        this->analyzer.get(),
        SIGNAL(source_info_message(const Suscan::SourceInfoMessage &)),
        this,
        SLOT(onSourceInfoMessage(const Suscan::SourceInfoMessage &)));

  connect(
        this->analyzer.get(),
        SIGNAL(status_message(const Suscan::StatusMessage &)),
        this,
        SLOT(onStatusMessage(const Suscan::StatusMessage &)));

  connect(
        this->analyzer.get(),
        SIGNAL(analyzer_params(const Suscan::AnalyzerParams &)),
        this,
        SLOT(onAnalyzerParams(const Suscan::AnalyzerParams &)));
}

void
Application::connectScanner(void)
{
  connect(
        this->scanner,
        SIGNAL(spectrumUpdated(void)),
        this,
        SLOT(onScannerUpdated(void)));

  connect(
        this->scanner,
        SIGNAL(stopped(void)),
        this,
        SLOT(onScannerStopped(void)));
}

void
Application::connectDeviceDetect(void)
{
  connect(
        this,
        SIGNAL(detectDevices()),
        this->deviceDetectWorker,
        SLOT(process()));

  connect(
        this->deviceDetectWorker,
        SIGNAL(finished()),
        this,
        SLOT(onDetectFinished()));
}

QString
Application::getLogText(void)
{
  QString text = "";
  std::lock_guard<Suscan::Logger> guard(*Suscan::Logger::getInstance());

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

    text += p.message.c_str();
  }

  return text;
}


void
Application::startCapture(void)
{
  auto iface = this->mediator->getProfile()->getInterface();

#ifdef _WIN32
  if (iface == SUSCAN_SOURCE_REMOTE_INTERFACE) {
    (void)  QMessageBox::critical(
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
    this->mediator->refreshUI();
    return;
  }
#endif // _WIN32

  try {
    this->filterInstalled = false;

    if (this->mediator->getState() == UIMediator::HALTED) {
      Suscan::AnalyzerParams params = *this->mediator->getAnalyzerParams();
      std::unique_ptr<Suscan::Analyzer> analyzer;
      Suscan::Source::Config profile = *this->mediator->getProfile();

      if (profile.getType() == SUSCAN_SOURCE_TYPE_SDR) {
        if (profile.getDecimatedSampleRate() > SIGDIGGER_MAX_SAMPLE_RATE) {
          unsigned decimate =
              static_cast<unsigned>(
                std::ceil(
                  profile.getSampleRate()
                  / static_cast<qreal>(SIGDIGGER_MAX_SAMPLE_RATE)));
          unsigned proposed =
              profile.getSampleRate() / decimate;
          QMessageBox::StandardButton reply
              = this->mediator->shouldReduceRate(
                  QString::fromStdString(profile.label()),
                  profile.getDecimatedSampleRate(),
                  proposed);

          // TODO: Maybe ask for decimation?
          if (reply == QMessageBox::Yes)
            profile.setDecimation(decimate);
          else if (reply == QMessageBox::Cancel) {
            this->mediator->setState(UIMediator::HALTED);
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

      this->sourceInfoReceived = false;

      // All set, move to application
      this->analyzer = std::move(analyzer);

      this->connectAnalyzer();

      this->mediator->setState(UIMediator::RUNNING, this->analyzer.get());
    }
  } catch (Suscan::Exception &) {
    (void)  QMessageBox::critical(
          this,
          "SigDigger error",
          "Failed to start capture due to errors:<p /><pre>"
          + getLogText().toHtmlEscaped()
          + "</pre>",
          QMessageBox::Ok);
    this->mediator->setState(UIMediator::HALTED);
  }
}

void
Application::orderedHalt(void)
{
  this->mediator->setState(UIMediator::HALTING);
  this->analyzer = nullptr;
  this->mediator->setState(UIMediator::HALTED);
}

void
Application::stopCapture(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    this->mediator->setState(UIMediator::HALTING);
    this->analyzer.get()->halt();
  }
}

void
Application::restartCapture(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    this->mediator->setState(UIMediator::RESTARTING);
    this->analyzer.get()->halt();
  }
}

void
Application::onAnalyzerHalted(void)
{
  bool restart = this->mediator->getState() == UIMediator::RESTARTING;

  this->orderedHalt();

  if (restart)
    this->startCapture();
}

void
Application::onAnalyzerEos(void)
{
  (void)  QMessageBox::information(
        this,
        "End of stream",
        "Capture interrupted due to stream end:<p /><pre>"
        + getLogText()
        + "</pre>",
        QMessageBox::Ok);

  this->orderedHalt();
}

void
Application::onPSDMessage(const Suscan::PSDMessage &msg)
{
  this->mediator->feedPSD(msg);
}

void
Application::onSourceInfoMessage(const Suscan::SourceInfoMessage &msg)
{
  this->mediator->notifySourceInfo(*msg.info());

  if (!this->sourceInfoReceived)
    this->sourceInfoReceived = true;
}

void
Application::onStatusMessage(const Suscan::StatusMessage &message)
{
  if (message.getCode() == SUSCAN_ANALYZER_INIT_FAILURE) {
    (void)  QMessageBox::critical(
          this,
          "Analyzer initialization",
          "Initialization failed: " + message.getMessage(),
          QMessageBox::Ok);
  } else {
    this->mediator->setStatusMessage(message.getMessage());
  }
}

void
Application::onAnalyzerParams(const Suscan::AnalyzerParams &params)
{
  this->mediator->setAnalyzerParams(params);
}

void
Application::onAnalyzerReadError(void)
{
  (void)  QMessageBox::critical(
        this,
        "Source error",
        "Capture stopped due to source read error. Last errors were:<p /><pre>"
        + getLogText()
        + "</pre>",
        QMessageBox::Ok);

  this->orderedHalt();
}

Application::~Application()
{
  this->uiTimer.stop();

  if (this->scanner != nullptr)
    delete this->scanner;

  this->analyzer = nullptr;

  this->deviceDetectThread->quit();
  this->deviceDetectThread->deleteLater();
  this->deviceDetectWorker->deleteLater();

  if (this->mediator != nullptr)
    delete this->mediator;
}

/////////////////////////////// Overrides //////////////////////////////////////
void
Application::closeEvent(QCloseEvent *)
{
  this->stopCapture();
}

//////////////////////////////// Slots /////////////////////////////////////////
void
Application::quit(void)
{
  this->stopCapture();
  QApplication::quit();
}

void
Application::onCaptureStart(void)
{
  this->startCapture();
}

void
Application::onCaptureStop(void)
{
  this->stopCapture();
}

void
Application::onProfileChanged(bool needsRestart)
{
  if (this->mediator->getProfile()->label() != "") {
    Suscan::Singleton *sing = Suscan::Singleton::get_instance();
    sing->notifyRecent(this->mediator->getProfile()->label());
    this->updateRecent();
  }

  if (needsRestart)
    this->restartCapture();
  else if (this->mediator->getState() == UIMediator::RUNNING)
    this->hotApplyProfile(this->mediator->getProfile());
}

void
Application::onGainChanged(QString name, float val)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    this->mediator->getProfile()->setGain(name.toStdString(), val);
    this->analyzer->setGain(name.toStdString(), val);
  }
}

void
Application::onFrequencyChanged(qint64 freq, qint64 lnb)
{
  this->mediator->getProfile()->setFreq(freq);
  this->mediator->getProfile()->setLnbFreq(lnb);

  if (this->mediator->getState() == UIMediator::RUNNING)
    this->analyzer->setFrequency(freq, lnb);
}

void
Application::onParamsChanged(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING)
    this->analyzer->setParams(*this->mediator->getAnalyzerParams());
}

void
Application::hotApplyProfile(Suscan::Source::Config const *profile)
{
  this->analyzer->setFrequency(profile->getFreq(), profile->getLnbFreq());
  this->analyzer->setBandwidth(profile->getBandwidth());
  this->analyzer->setDCRemove(profile->getDCRemove());
  this->analyzer->setAntenna(profile->getAntenna());
}

void
Application::onSeek(struct timeval tv)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    try {
      this->analyzer->seek(tv);
    } catch (Suscan::Exception &) {
      (void)  QMessageBox::critical(
            this,
            "SigDigger error",
            "Source does not allow seeking",
            QMessageBox::Ok);
    }
  }
}

void
Application::onAntennaChanged(QString name)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    try {
      this->analyzer->setAntenna(name.toStdString());
    } catch (Suscan::Exception &) {
      (void)  QMessageBox::critical(
            this,
            "SigDigger error",
            "Source does not allow changing the current RX antenna",
            QMessageBox::Ok);
    }
  }
}

void
Application::onDeviceRefresh(void)
{
  emit detectDevices();
}

void
Application::onDetectFinished(void)
{
  this->mediator->refreshDevicesDone();
}

void
Application::onBandwidthChanged(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    try {
      this->analyzer->setBandwidth(this->mediator->getProfile()->getBandwidth());
    } catch (Suscan::Exception &) {
      (void)  QMessageBox::critical(
            this,
            "SigDigger error",
            "Source does not allow setting the bandwidth",
            QMessageBox::Ok);
    }
  }
}

void
Application::onPPMChanged(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    try {
      this->analyzer->setPPM(this->mediator->getProfile()->getPPM());
    } catch (Suscan::Exception &) {
      (void)  QMessageBox::critical(
            this,
            "SigDigger error",
            "Source does not allow manual PPM adjustment",
            QMessageBox::Ok);
    }
  }
}

void
Application::onRecentSelected(QString profile)
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();
  Suscan::Source::Config *config = sing->getProfile(profile.toStdString());

  if (config != nullptr) {
    bool forceStart = this->mediator->getState() == UIMediator::HALTED;
    this->mediator->setProfile(*config);
    if (forceStart)
      this->startCapture();
  } else {
    (void) sing->removeRecent(profile.toStdString());
    QMessageBox::warning(
          this,
          "Failed to load recent profile",
          "Cannot load this profile. It was either renamed or deleted "
          "before the history was updated. The profile has been removed from history.",
              QMessageBox::Ok);
  }
}

void
Application::onRecentCleared(void)
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();

  sing->clearRecent();
}

void
Application::onPanSpectrumStart(void)
{
  if (this->scanner == nullptr) {
    qint64 freqMin;
    qint64 freqMax;
    Suscan::Source::Device device;

    if (this->mediator->getPanSpectrumRange(freqMin, freqMax)
        && this->mediator->getPanSpectrumDevice(device)) {
      Suscan::Source::Config config(
            SUSCAN_SOURCE_TYPE_SDR,
            SUSCAN_SOURCE_FORMAT_AUTO);

      this->scanMinFreq = static_cast<SUFREQ>(freqMin);
      this->scanMaxFreq = static_cast<SUFREQ>(freqMax);

      config.setDevice(device);
      config.setSampleRate(
            static_cast<unsigned int>(
              this->mediator->getPanSpectrumPreferredSampleRate()));
      config.setDCRemove(true);
      config.setBandwidth(this->mediator->getPanSpectrumPreferredSampleRate());
      config.setLnbFreq(this->mediator->getPanSpectrumLnbOffset());
      config.setFreq(.5 * (this->scanMinFreq + this->scanMaxFreq));

      try {
        Suscan::Logger::getInstance()->flush();
        this->scanner = new Scanner(this, freqMin, freqMax, config);
        this->scanner->setRelativeBw(this->mediator->getPanSpectrumRelBw());
        this->scanner->setRttMs(this->mediator->getPanSpectrumRttMs());
        this->onPanSpectrumStrategyChanged(
              this->mediator->getPanSpectrumStrategy());
        this->onPanSpectrumPartitioningChanged(
              this->mediator->getPanSpectrumPartition());

        for (auto p = device.getFirstGain();
             p != device.getLastGain();
             ++p) {
          this->scanner->setGain(
                QString::fromStdString(p->getName()),
                this->mediator->getPanSpectrumGain(
                  QString::fromStdString(p->getName())));
        }

        this->connectScanner();
        Suscan::Logger::getInstance()->flush();
      } catch (Suscan::Exception &) {
        (void)  QMessageBox::critical(
              this,
              "SigDigger error",
              "Failed to start capture due to errors:<p /><pre>"
              + getLogText()
              + "</pre>",
              QMessageBox::Ok);
      }
    }
  }

  this->mediator->setPanSpectrumRunning(this->scanner != nullptr);
}

void
Application::onPanSpectrumStop(void)
{
  if (this->scanner != nullptr) {
    delete this->scanner;
    this->scanner = nullptr;
  }

  this->mediator->setPanSpectrumRunning(this->scanner != nullptr);
}

void
Application::onPanSpectrumRangeChanged(qint64 min, qint64 max, bool noHop)
{
  if (this->scanner != nullptr)
    this->scanner->setViewRange(min, max, noHop);
}

void
Application::onPanSpectrumSkipChanged(void)
{
  if (this->scanner != nullptr)
    this->scanner->setRttMs(this->mediator->getPanSpectrumRttMs());
}

void
Application::onPanSpectrumRelBwChanged(void)
{
  if (this->scanner != nullptr)
    this->scanner->setRelativeBw(this->mediator->getPanSpectrumRelBw());
}

void
Application::onPanSpectrumReset(void)
{
  if (this->scanner != nullptr) {
    this->scanner->flip();
    this->scanner->flip();
  }
}

void
Application::onPanSpectrumStrategyChanged(QString strategy)
{
  if (this->scanner != nullptr) {
    if (strategy.toStdString() == "Stochastic")
      this->scanner->setStrategy(Suscan::Analyzer::STOCHASTIC);
    else if (strategy.toStdString() == "Progressive")
      this->scanner->setStrategy(Suscan::Analyzer::PROGRESSIVE);
  }
}

void
Application::onPanSpectrumPartitioningChanged(QString partitioning)
{
  if (this->scanner != nullptr) {
    if (partitioning.toStdString() == "Continuous")
      this->scanner->setPartitioning(Suscan::Analyzer::CONTINUOUS);
    else if (partitioning.toStdString() == "Discrete")
      this->scanner->setPartitioning(Suscan::Analyzer::DISCRETE);
  }
}

void
Application::onPanSpectrumGainChanged(QString name, float value)
{
  if (this->scanner != nullptr)
    this->scanner->setGain(name, value);
}

void
Application::onScannerStopped(void)
{
  QString messages = getLogText();

  if (this->scanner != nullptr) {
    delete this->scanner;
    this->scanner = nullptr;
  }

  if (messages.size() > 0) {
    (void)  QMessageBox::warning(
          this,
          "Scanner stopped",
          "Running scanner has stopped. The error log was:<p /><pre>"
          + getLogText()
          + "</pre>",
          QMessageBox::Ok);
  }

  this->mediator->setPanSpectrumRunning(this->scanner != nullptr);
}

void
Application::onScannerUpdated(void)
{
  SpectrumView &view = this->scanner->getSpectrumView();

  this->mediator->setMinPanSpectrumBw(this->scanner->getFs());

  this->mediator->feedPanSpectrum(
        static_cast<quint64>(view.freqMin),
        static_cast<quint64>(view.freqMax),
        view.psd,
        SIGDIGGER_SCANNER_SPECTRUM_SIZE);
}

void
Application::onAddBookmark(BookmarkInfo info)
{
  if (!Suscan::Singleton::get_instance()->registerBookmark(info)) {
    QMessageBox *mb = new QMessageBox(
          QMessageBox::Warning,
          "Cannot create bookmark",
          "A bookmark already exists for frequency "
          + SuWidgetsHelpers::formatQuantity(info.frequency, "Hz. If you wish to "
          "edit this bookmark use the bookmark manager instead."),
          QMessageBox::Ok,
          this);
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->show();
  }

  this->ui.spectrum->updateOverlay();
}

void
Application::onTick(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING)
    this->mediator->notifyTimeStamp(this->analyzer->getSourceTimeStamp());
}
