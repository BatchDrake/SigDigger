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
#include <fcntl.h>

#include "Application.h"

#include <QMessageBox>
#include <SuWidgetsHelpers.h>

// TODO: REMOVE DEPENDS
#include "AudioPanel.h"
#include "MainSpectrum.h"
#include "SourcePanel.h"
#include "Inspector.h"
#include "InspectorPanel.h"

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

SUPRIVATE SUBOOL
onBaseBandData(
    void *privdata,
    suscan_analyzer_t *,
    const SUCOMPLEX *samples,
    SUSCOUNT length)
{
  Application *app = static_cast<Application *>(privdata);
  FileDataSaver *saver;

  if ((saver = app->getSaver()) != nullptr)
    saver->write(samples, length);

  return SU_TRUE;
}

void
Application::uninstallDataSaver()
{
  this->dataSaver = nullptr;
}

void
Application::connectDataSaver()
{
  this->connect(
        this->dataSaver.get(),
        SIGNAL(stopped()),
        this,
        SLOT(onSaveError()));

  this->connect(
        this->dataSaver.get(),
        SIGNAL(swamped()),
        this,
        SLOT(onSaveSwamped()));

  this->connect(
        this->dataSaver.get(),
        SIGNAL(dataRate(qreal)),
        this,
        SLOT(onSaveRate(qreal)));

  this->connect(
        this->dataSaver.get(),
        SIGNAL(commit()),
        this,
        SLOT(onCommit()));
}

void
Application::connectAudioFileSaver()
{
  this->connect(
        this->audioFileSaver.get(),
        SIGNAL(stopped()),
        this,
        SLOT(onAudioSaveError()));

  this->connect(
        this->audioFileSaver.get(),
        SIGNAL(swamped()),
        this,
        SLOT(onAudioSaveSwamped()));

  this->connect(
        this->audioFileSaver.get(),
        SIGNAL(dataRate(qreal)),
        this,
        SLOT(onAudioSaveRate(qreal)));

  this->connect(
        this->audioFileSaver.get(),
        SIGNAL(commit()),
        this,
        SLOT(onAudioCommit()));
}


void
Application::installDataSaver(int fd)
{
  if (this->dataSaver.get() == nullptr && this->analyzer.get() != nullptr) {
    this->dataSaver = std::make_unique<FileDataSaver>(fd, this);
    this->dataSaver->setSampleRate(
          this->mediator->getProfile()->getDecimatedSampleRate());
    if (!this->filterInstalled) {
      this->analyzer->registerBaseBandFilter(onBaseBandData, this);
      this->filterInstalled = true;
    }
    this->connectDataSaver();
  }
}

void
Application::setAudioInspectorParams(
    unsigned int rate,
    SUFLOAT cutOff,
    unsigned int demod,
    bool squelch,
    SUFLOAT squelchLevel)
{
  if (this->audioConfigured) {
    Suscan::Config cfg(this->audioCfgTemplate);
    cfg.set("audio.cutoff", cutOff);
    cfg.set("audio.volume", 1.f);
    cfg.set("audio.sample-rate", static_cast<uint64_t>(rate));
    cfg.set("audio.demodulator", static_cast<uint64_t>(demod));
    cfg.set("audio.squelch", squelch);
    cfg.set("audio.squelch-level", squelchLevel);
    this->analyzer->setInspectorConfig(this->audioInspHandle, cfg, 0);
    this->assertAudioInspectorLo();
  } else {
    this->delayedRate      = rate;
    this->delayedCutOff    = cutOff;
    this->delayedDemod     = demod;
    this->delayedEnableSql = squelch;
    this->delayedSqlLevel  = squelchLevel;
  }
}

bool
Application::openAudioFileSaver(void)
{
  bool opened = false;

  if (this->audioFileSaver == nullptr) {
    AudioFileSaver::AudioFileParams params;
    params.sampRate   = this->ui.audioPanel->getSampleRate();
    params.savePath   = this->mediator->getAudioRecordSavePath();
    params.frequency  =
        this->ui.spectrum->getCenterFreq() + this->ui.spectrum->getLoFreq();
    params.modulation = this->ui.audioPanel->getDemod();

    this->audioFileSaver = std::make_unique<AudioFileSaver>(params, nullptr);
    this->connectAudioFileSaver();
    opened = true;
  }

  return opened;
}

void
Application::closeAudioFileSaver(void)
{
  if (this->audioFileSaver != nullptr)
    this->audioFileSaver = nullptr;

  this->mediator->setAudioRecordSize(0);
  this->mediator->setAudioRecordState(false);
}

bool
Application::openAudio(unsigned int rate)
{
  bool opened = false;

  if (this->mediator->getState() == UIMediator::RUNNING) {
    if (this->playBack == nullptr) {
      try {
        Suscan::Channel ch;
        SUFREQ maxFc = this->analyzer->getSampleRate() / 2;
        SUFREQ bw = SIGDIGGER_AUDIO_INSPECTOR_BANDWIDTH;

        if (rate > bw)
          rate = static_cast<unsigned int>(floor(bw));

        this->playBack = std::make_unique<AudioPlayback>("default", rate);
        this->audioSampleRate = this->playBack->getSampleRate();
        this->lastAudioLo = this->getAudioInspectorLo();

        if (bw > this->analyzer->getSampleRate() / 2)
          bw = this->analyzer->getSampleRate() / 2;

        ch.bw    = bw;
        ch.ft    = 0;
        ch.fc    = this->getAudioInspectorLo();
        ch.fLow  = -.5 * bw;
        ch.fHigh = .5 * bw;

        if (ch.fc > maxFc || ch.fc < -maxFc)
          ch.fc = 0;

        this->maxAudioBw = bw;

        this->analyzer->openPrecise(
              "audio",
              ch,
              SIGDIGGER_AUDIO_INSPECTOR_REQID);

        this->playBack->setVolume(this->ui.audioPanel->getMuteableVolume());

        this->setAudioInspectorParams(
              this->audioSampleRate,
              this->ui.audioPanel->getCutOff(),
              this->ui.audioPanel->getDemod() + 1,
              this->ui.audioPanel->getSquelchEnabled(),
              this->ui.audioPanel->getSquelchLevel());
        opened = true;
      } catch (Suscan::Exception const &e) {
        QMessageBox::critical(
                  this,
                  "Internal Suscan exception",
                  "Failed to open inspector. Error was:<p /><pre>"
                  + QString(e.what()) + "</pre>",
                  QMessageBox::Ok);
        this->playBack = nullptr;
      } catch (std::runtime_error const &e) {
        QMessageBox::warning(
                  this,
                  "Failed to open soundcard device",
                  "Cannot open audio device. Error was:<p /><pre>"
                  + QString(e.what()) + "</pre>",
                  QMessageBox::Ok);
      }
    }
  }

  return opened;
}

void
Application::closeAudio(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING
      && this->audioInspectorOpened)
    this->analyzer->closeInspector(this->audioInspHandle, 0);
  this->closeAudioFileSaver();
  this->audioInspectorOpened = false;
  this->audioSampleRate = 0;
  this->audioInspHandle = 0;
  this->playBack = nullptr;
  this->audioConfigured = false;
}

SUFREQ
Application::getAudioInspectorBandwidth(void) const
{
  SUFREQ bw = this->ui.spectrum->getBandwidth();

  if (this->ui.audioPanel->getDemod() > 1)
    bw *= .5;

  if (bw > this->maxAudioBw)
    bw = this->maxAudioBw;
  else if (bw < 1)
    bw = 1;

  return bw;
}

SUFREQ
Application::getAudioInspectorLo(void) const
{
  SUFREQ lo = this->ui.spectrum->getLoFreq();
  SUFREQ bw = this->getAudioInspectorBandwidth();
  SUFREQ delta = 0;

  if (this->ui.audioPanel->getDemod() == AudioDemod::USB)
    delta += .5 * bw;
  else if (this->ui.audioPanel->getDemod() == AudioDemod::LSB)
    delta -= .5 * bw;

  return lo + delta;
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
        SIGNAL(toggleRecord(void)),
        this,
        SLOT(onToggleRecord(void)));

  connect(
        this->mediator,
        SIGNAL(throttleConfigChanged(void)),
        this,
        SLOT(onThrottleConfigChanged(void)));

  connect(
        this->mediator,
        SIGNAL(seek(struct timeval)),
        this,
        SLOT(onSeek(struct timeval)));

  connect(
        this->mediator,
        SIGNAL(gainChanged(QString, float)),
        this,
        SLOT(onGainChanged(QString, float)));

  connect(
        this->mediator,
        SIGNAL(requestOpenInspector(void)),
        this,
        SLOT(onOpenInspector(void)));

  connect(
        this->mediator,
        SIGNAL(requestOpenRawInspector(void)),
        this,
        SLOT(onOpenRawInspector(void)));

  connect(
        this->mediator,
        SIGNAL(requestCloseRawInspector(void)),
        this,
        SLOT(onCloseRawInspector(void)));

  connect(
        this->mediator,
        SIGNAL(toggleDCRemove(void)),
        this,
        SLOT(onToggleDCRemove(void)));

  connect(
        this->mediator,
        SIGNAL(toggleIQReverse(void)),
        this,
        SLOT(onToggleIQReverse(void)));

  connect(
        this->mediator,
        SIGNAL(toggleAGCEnabled(void)),
        this,
        SLOT(onToggleAGCEnabled(void)));

  connect(
      this->mediator,
        SIGNAL(analyzerParamsChanged(void)),
        this,
        SLOT(onParamsChanged(void)));

  connect(
        this->mediator,
        SIGNAL(loChanged(qint64)),
        this,
        SLOT(onLoChanged(qint64)));

  connect(
        this->mediator,
        SIGNAL(channelBandwidthChanged(qreal)),
        this,
        SLOT(onChannelBandwidthChanged(qreal)));

  connect(
        this->mediator,
        SIGNAL(audioChanged(void)),
        this,
        SLOT(onAudioChanged(void)));

  connect(
        this->mediator,
        SIGNAL(audioVolumeChanged(float)),
        this,
        SLOT(onAudioVolumeChanged(float)));

  connect(
        this->mediator,
        SIGNAL(audioRecordStateChanged(void)),
        this,
        SLOT(onAudioRecordStateChanged(void)));

  connect(
        this->mediator,
        SIGNAL(bandwidthChanged(void)),
        this,
        SLOT(onBandwidthChanged(void)));

  connect(
        this->mediator,
        SIGNAL(ppmChanged(void)),
        this,
        SLOT(onPPMChanged(void)));

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
        this->mediator,
        SIGNAL(audioSetCorrection(Suscan::Orbit)),
        this,
        SLOT(onAudioSetCorrection(Suscan::Orbit)));

  connect(
        this->mediator,
        SIGNAL(audioDisableCorrection(void)),
        this,
        SLOT(onAudioDisableCorrection(void)));

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

  connect(
        this->analyzer.get(),
        SIGNAL(inspector_message(const Suscan::InspectorMessage &)),
        this,
        SLOT(onInspectorMessage(const Suscan::InspectorMessage &)));

  connect(
        this->analyzer.get(),
        SIGNAL(samples_message(const Suscan::SamplesMessage &)),
        this,
        SLOT(onInspectorSamples(const Suscan::SamplesMessage &)));
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

      // Enable throttling, if requested
      if (this->ui.sourcePanel->isThrottleEnabled())
        analyzer->setThrottle(this->ui.sourcePanel->getThrottleRate());

      analyzer->setDCRemove(this->ui.sourcePanel->getDCremove());
      analyzer->setIQReverse(this->ui.sourcePanel->getIQReverse());

      if (this->ui.sourcePanel->getAGCEnabled())
        analyzer->setAGC(true);

      // All set, move to application
      this->analyzer = std::move(analyzer);

      // If there is a capture file configured, install data saver
      if (this->ui.sourcePanel->getRecordState()) {
        int fd = this->openCaptureFile();
        if (fd != -1)
          this->installDataSaver(fd);
      }

      this->connectAnalyzer();

      this->mediator->setState(UIMediator::RUNNING);

      if (this->ui.audioPanel->getEnabled())
        this->openAudio(this->ui.audioPanel->getSampleRate());
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
  this->uninstallDataSaver();
  this->mediator->setRecordState(false);
  this->mediator->detachAllInspectors();
  this->closeAudio();
  this->rawInspectorOpened = false;
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
  else
    this->closeAudioFileSaver();
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

  // It may have notified a change in current frequency.
  this->assertAudioInspectorLo();
}

void
Application::onInspectorSamples(const Suscan::SamplesMessage &msg)
{
  Inspector *insp;

  if (msg.getInspectorId() == SIGDIGGER_AUDIO_INSPECTOR_MAGIC_ID) {
    if (this->playBack != nullptr)
      this->playBack->write(msg.getSamples(), msg.getCount());
    if (this->audioFileSaver != nullptr)
      this->audioFileSaver->write(msg.getSamples(), msg.getCount());
  } else {
    switch (msg.getInspectorId()) {
      case SIGDIGGER_RAW_INSPECTOR_MAGIC_ID:
        this->mediator->feedRawInspector(msg.getSamples(), msg.getCount());
        break;

      default:
        if ((insp = this->mediator->lookupInspector(msg.getInspectorId()))
                     != nullptr)
            insp->feed(msg.getSamples(), msg.getCount());
    }
  }
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
Application::onInspectorMessage(const Suscan::InspectorMessage &msg)
{
  Inspector *insp = nullptr;
  SUFLOAT *data;
  SUSCOUNT len, p;
  Suscan::InspectorId oId;
  float x;

  switch (msg.getKind()) {
    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_OPEN:
      // Audio path: set inspector Id

      if (msg.getClass() == "audio") {
        this->audioInspHandle = msg.getHandle();
        this->audioInspectorOpened = true;
        this->analyzer->setInspectorId(
              msg.getHandle(),
              SIGDIGGER_AUDIO_INSPECTOR_MAGIC_ID,
              0);
        this->analyzer->setInspectorWatermark(
              msg.getHandle(),
              SIGDIGGER_AUDIO_BUFFER_SIZE / 2,
              0);
        this->analyzer->setInspectorBandwidth(
              msg.getHandle(),
              this->getAudioInspectorBandwidth(),
              0);
        if (this->audioCfgTemplate == nullptr)
          SU_ATTEMPT(this->audioCfgTemplate = suscan_config_dup(msg.getCConfig()));

        this->audioConfigured = true;

        /* Set params for good */
        this->setAudioInspectorParams(
              this->audioSampleRate,
              this->delayedCutOff,
              this->delayedDemod,
              this->delayedEnableSql,
              this->delayedSqlLevel);

        /* Enable Doppler correction */
        if (this->mediator->isAudioDopplerCorrectionEnabled())
          this->analyzer->setInspectorDopplerCorrection(
              this->audioInspHandle,
              this->mediator->getAudioOrbit(),
              0);
      } else if (msg.getClass() == "raw") {
          this->rawInspHandle = msg.getHandle();
          this->rawInspectorOpened = true;

          this->analyzer->setInspectorId(
                msg.getHandle(),
                SIGDIGGER_RAW_INSPECTOR_MAGIC_ID,
                0);

          this->mediator->resetRawInspector(
                static_cast<qreal>(msg.getEquivSampleRate()));
      } else {
          insp = this->mediator->addInspectorTab(msg, oId);
          insp->setAnalyzer(this->analyzer.get());
          this->analyzer->setInspectorId(msg.getHandle(), oId, 0);
      }
      break;

    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_SPECTRUM:
       if ((insp = this->mediator->lookupInspector(msg.getInspectorId())) != nullptr) {
         data = msg.getSpectrumData();
         len = msg.getSpectrumLength();
         p = len / 2;

         for (auto i = 0u; i < len; ++i)
           data[i] = SU_POWER_DB(data[i]);

         for (auto i = 0u; i < len / 2; ++i) {
           x = data[i];
           data[i] = data[p];
           data[p] = x;

           if (++p == len)
             p = 0;
         }
         insp->feedSpectrum(
               data,
               len,
               msg.getSpectrumRate(),
               msg.getSpectrumSourceId());
       }
      break;

    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_ESTIMATOR:
      if ((insp = this->mediator->lookupInspector(msg.getInspectorId())) != nullptr)
        insp->updateEstimator(msg.getEstimatorId(), msg.getEstimation());

      break;

    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_CLOSE:
      if (this->audioConfigured && this->audioInspHandle == msg.getHandle()) {
        // Do nothing (yet).
      } else if (this->rawInspectorOpened && this->rawInspHandle == msg.getHandle()) {
        // Do nothing either (yet).
      } else if ((insp = this->mediator->lookupInspector(msg.getInspectorId())) != nullptr) {
        insp->setAnalyzer(nullptr);
        this->mediator->closeInspectorTab(insp);
      }

      break;

    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_SET_TLE:
      if (!msg.isTLEEnabled())
        this->mediator->notifyDisableCorrection(msg.getInspectorId());
      break;

    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_ORBIT_REPORT:
      this->mediator->notifyOrbitReport(
            msg.getInspectorId(),
            msg.getOrbitReport());
      break;

    default:
      // printf("Ignored inspector message of type %d\n", msg.getKind());
      break;
  }
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

  if (this->audioCfgTemplate != nullptr)
    suscan_config_destroy(this->audioCfgTemplate);

  if (this->scanner != nullptr)
    delete this->scanner;

  this->playBack = nullptr;
  this->analyzer = nullptr;
  this->uninstallDataSaver();
  this->audioFileSaver = nullptr;

  this->deviceDetectThread->quit();
  this->deviceDetectThread->deleteLater();
  this->deviceDetectWorker->deleteLater();
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
Application::onToggleIQReverse(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING)
    this->analyzer->setIQReverse(this->ui.sourcePanel->getIQReverse());
}

void
Application::onToggleDCRemove(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING)
    this->analyzer->setDCRemove(this->ui.sourcePanel->getDCremove());
}

void
Application::onToggleAGCEnabled(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING)
    this->analyzer->setAGC(this->ui.sourcePanel->getAGCEnabled());
}

void
Application::onParamsChanged(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING)
    this->analyzer->setParams(*this->mediator->getAnalyzerParams());
}

void
Application::onOpenInspector(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    Suscan::Channel ch;

    ch.bw    = this->ui.inspectorPanel->getBandwidth();
    ch.ft    = 0;
    ch.fc    = this->ui.spectrum->getLoFreq();
    ch.fLow  = - .5 * ch.bw;
    ch.fHigh = + .5 * ch.bw;

    if (this->ui.inspectorPanel->getPrecise())
      this->analyzer->openPrecise(
          this->ui.inspectorPanel->getInspectorClass(),
          ch,
          0);
    else
      this->analyzer->open(
          this->ui.inspectorPanel->getInspectorClass(),
          ch,
          0);
  }
}

void
Application::onOpenRawInspector(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING
      && !this->rawInspectorOpened) {
    Suscan::Channel ch;

    ch.bw    = this->ui.inspectorPanel->getBandwidth();
    ch.ft    = 0;
    ch.fc    = this->ui.spectrum->getLoFreq();
    ch.fLow  = - .5 * ch.bw;
    ch.fHigh = + .5 * ch.bw;

    this->analyzer->openPrecise("raw", ch, SIGDIGGER_RAW_INSPECTOR_REQID);
  }
}

void
Application::onCloseRawInspector(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    if (this->rawInspectorOpened) {
      this->analyzer->closeInspector(this->rawInspHandle, 0);
      this->rawInspectorOpened = false;
    }
  }
}

void
Application::onThrottleConfigChanged(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    if (this->ui.sourcePanel->isThrottleEnabled()) {
      // TODO: Modify dataSaver
      this->analyzer->setThrottle(this->ui.sourcePanel->getThrottleRate());
    } else {
      this->analyzer->setThrottle(0);
    }
  }
}

void
Application::hotApplyProfile(Suscan::Source::Config const *profile)
{
  this->analyzer->setFrequency(profile->getFreq(), profile->getLnbFreq());
  this->analyzer->setBandwidth(profile->getBandwidth());
  this->analyzer->setDCRemove(profile->getDCRemove());
  this->analyzer->setAntenna(profile->getAntenna());
}

//
// sigdigger_XXXXXXXX_XXXXXXZ_XXXXXXXXXX_XXXXXXXXXXXXXXXXXXXX_float32_iq.raw
//
int
Application::openCaptureFile(void)
{
  int fd = -1;
  char baseName[80];
  char datetime[17];
  time_t unixtime;
  struct tm tm;

  unixtime = time(NULL);
  gmtime_r(&unixtime, &tm);
  strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M%SZ", &tm);

  snprintf(
        baseName,
        sizeof(baseName),
        "sigdigger_%s_%d_%.0lf_float32_iq.raw",
        datetime,
        this->mediator->getProfile()->getDecimatedSampleRate(),
        this->mediator->getProfile()->getFreq());

  std::string fullPath =
      this->ui.sourcePanel->getRecordSavePath() + "/" + baseName;

  if ((fd = creat(fullPath.c_str(), 0600)) == -1) {
    QMessageBox::warning(
              this,
              "SigDigger error",
              "Failed to open capture file for writing: " +
              QString(strerror(errno)),
              QMessageBox::Ok);
  }

  return fd;
}

void
Application::onToggleRecord(void)
{
  if (this->ui.sourcePanel->getRecordState()) {
    if (this->mediator->getState() == UIMediator::RUNNING) {
      int fd = this->openCaptureFile();
      if (fd != -1)
        this->installDataSaver(fd);

      this->ui.sourcePanel->setRecordState(fd != -1);
    }
  } else {
    this->uninstallDataSaver();
    this->mediator->setCaptureSize(0);
    this->ui.sourcePanel->setRecordState(false);
  }
}

void
Application::onSeek(struct timeval tv)
{
  if (this->mediator->getState() == UIMediator::RUNNING)
    this->analyzer->seek(tv);
}

void
Application::onSaveError(void)
{
  if (this->dataSaver.get() != nullptr) {
    this->uninstallDataSaver();

    QMessageBox::warning(
              this,
              "SigDigger error",
              "Capture file write error. Disk full?",
              QMessageBox::Ok);

    this->mediator->setRecordState(false);
  }
}

void
Application::onSaveSwamped(void)
{
  if (this->dataSaver.get() != nullptr) {
    this->uninstallDataSaver();

    QMessageBox::warning(
          this,
          "SigDigger error",
          "Capture thread swamped. Maybe your storage device is too slow",
          QMessageBox::Ok);

    this->mediator->setRecordState(false);
  }
}

void
Application::onSaveRate(qreal rate)
{
  this->mediator->setIORate(rate);
}

void
Application::onCommit(void)
{
  this->mediator->setCaptureSize(this->dataSaver->getSize());
}

void
Application::onAudioSaveError(void)
{
  if (this->audioFileSaver != nullptr) {
    this->closeAudioFileSaver();

    QMessageBox::warning(
              this,
              "SigDigger error",
              "Audio saver stopped unexpectedly. Check disk usage and directory permissions and try again.",
              QMessageBox::Ok);
  }
}

void
Application::onAudioSaveSwamped(void)
{
  if (this->audioFileSaver != nullptr) {
    this->closeAudioFileSaver();

    QMessageBox::warning(
          this,
          "SigDigger error",
          "Audiofile thread swamped. Maybe your storage device is too slow",
          QMessageBox::Ok);
  }
}

void
Application::onAudioSaveRate(qreal rate)
{
  this->mediator->setAudioRecordIORate(rate);
}

void
Application::onAudioCommit(void)
{
  this->mediator->setAudioRecordSize(
        this->audioFileSaver->getSize() * sizeof(uint16_t) / sizeof(SUCOMPLEX));
}


void
Application::onLoChanged(qint64)
{
  if (this->audioConfigured)
    this->assertAudioInspectorLo();
}

void
Application::assertAudioInspectorLo(void)
{
  SUFREQ lo = this->getAudioInspectorLo();

  if (!sufeq(lo, this->lastAudioLo, 1e-8)) {
    if (this->audioConfigured)
      this->analyzer->setInspectorFreq(this->audioInspHandle, lo, 0);
    this->lastAudioLo = lo;
  }
}

void
Application::onChannelBandwidthChanged(qreal)
{
  if (this->audioConfigured) {
    SUFREQ bw;
    bw = this->getAudioInspectorBandwidth();

    this->analyzer->setInspectorBandwidth(this->audioInspHandle, bw, 0);
    this->assertAudioInspectorLo();
  }
}

void
Application::onAudioChanged(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    if (this->audioFileSaver != nullptr) {
      // If any parameter affecting the sample rate or even the name of
      // the file has changed, we must stop the audio file saver and restart
      // it again.

      if (this->ui.audioPanel->getDemod()
            != this->audioFileSaver->params.modulation
          || this->ui.audioPanel->getSampleRate()
            != this->audioFileSaver->params.sampRate) {
        this->closeAudioFileSaver();
        this->openAudioFileSaver();
      }
    }

    if (this->playBack == nullptr) {
      if (this->ui.audioPanel->getEnabled()) {
        // Audio enabled, open it.
        (void) this->openAudio(this->ui.audioPanel->getSampleRate());
      }
    } else {
     if (this->ui.audioPanel->getEnabled()) {
       // Audio enabled, update parameters

       if (this->ui.audioPanel->getSampleRate() != this->audioSampleRate) {
         this->closeAudio();
         this->openAudio(this->ui.audioPanel->getSampleRate());

         // XXX: MEDIATE!!
         if (this->ui.audioPanel->getSampleRate() != this->audioSampleRate)
           this->ui.audioPanel->setSampleRate(this->audioSampleRate);
       }

       this->setAudioInspectorParams(
             this->audioSampleRate,
             this->ui.audioPanel->getCutOff(),
             this->ui.audioPanel->getDemod() + 1,
             this->ui.audioPanel->getSquelchEnabled(),
             this->ui.audioPanel->getSquelchLevel());
     } else {
       // Disable audio
       closeAudio();
     }
    }
  }
}

void
Application::onAudioVolumeChanged(float)
{
  if (this->playBack != nullptr)
    this->playBack->setVolume(this->ui.audioPanel->getMuteableVolume());
}

void
Application::onAudioSetCorrection(Suscan::Orbit orbit)
{
  if (this->mediator->getState() == UIMediator::RUNNING
      && this->audioInspectorOpened)
    this->analyzer->setInspectorDopplerCorrection(
          this->audioInspHandle,
          orbit,
          0);
}

void
Application::onAudioDisableCorrection(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING
      && this->audioInspectorOpened)
  this->analyzer->disableDopplerCorrection(this->audioInspHandle, 0);
}

void
Application::onAudioRecordStateChanged(void)
{
  if (this->mediator->getAudioRecordState()) {
    this->openAudioFileSaver();
  } else {
    this->closeAudioFileSaver();
  }
}

void
Application::onAntennaChanged(QString name)
{
  if (this->mediator->getState() == UIMediator::RUNNING)
    this->analyzer->setAntenna(name.toStdString());
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
    this->analyzer->setBandwidth(this->mediator->getProfile()->getBandwidth());
    this->assertAudioInspectorLo();
  }
}

void
Application::onPPMChanged(void)
{
  if (this->mediator->getState() == UIMediator::RUNNING) {
    this->analyzer->setPPM(this->mediator->getProfile()->getPPM());
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
