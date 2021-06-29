//
//    UIMediator.cpp: User Interface mediator object
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

#include <Suscan/Library.h>
#include <QFileDialog>
#include <sys/statvfs.h>
#include <SuWidgetsHelpers.h>

#include "UIMediator.h"

#include <QGuiApplication>
#include <QDockWidget>
#include <QMessageBox>
#include <QScreen>

#include <fstream>

using namespace SigDigger;

void
UIMediator::addBandPlan(std::string const &name)
{
  QAction *action = new QAction(
        QString::fromStdString(name),
        this->ui->main->menuStart);

  action->setCheckable(true);

  this->ui->main->menuBand_plans->addAction(action);
  this->bandPlanMap[name] = action;

  connect(action, SIGNAL(triggered(bool)), this, SLOT(onTriggerBandPlan(void)));
}

void
UIMediator::clearRecent(void)
{
  this->ui->main->menuStart->clear();
  this->recentCount = 0;
}

void
UIMediator::addRecent(std::string const &name)
{
  QAction *action = new QAction(
        QString::fromStdString(name),
        this->ui->main->menuStart);

  this->ui->main->menuStart->addAction(action);
  ++this->recentCount;

  connect(action, SIGNAL(triggered(bool)), this, SLOT(onTriggerRecent(bool)));
}

void
UIMediator::finishRecent(void)
{
  QAction *action = new QAction("Clear", this->ui->main->menuStart);

  if (this->recentCount == 0) {
    QAction *stub = new QAction("(Empty)", this->ui->main->menuStart);
    stub->setEnabled(false);
    this->ui->main->menuStart->addAction(stub);
  }

  this->ui->main->menuStart->addSeparator();
  this->ui->main->menuStart->addAction(action);

  connect(action, SIGNAL(triggered(bool)), this, SLOT(onTriggerClear(bool)));
}

void
UIMediator::setProfile(Suscan::Source::Config const &prof)
{
  this->appConfig->profile = prof;
  this->refreshProfile();
  this->refreshUI();
  emit profileChanged();
}

void
UIMediator::refreshUI(void)
{
  QString stateString;
  QString sourceDesc;

  Suscan::Source::Config *config = this->getProfile();
  const Suscan::Source::Device &dev = config->getDevice();

  switch (this->state) {
    case HALTED:
      stateString = QString("Idle");
      this->ui->spectrum->setCaptureMode(MainSpectrum::UNAVAILABLE);
      this->setProcessRate(0);
      this->ui->main->actionRun->setEnabled(true);
      this->ui->main->actionRun->setChecked(false);
      this->ui->main->actionStart_capture->setEnabled(true);
      this->ui->main->actionStop_capture->setEnabled(false);
      this->ui->panoramicDialog->setBannedDevice("");
      this->ui->spectrum->notifyHalt();
      break;

    case HALTING:
      stateString = QString("Halting...");
      this->ui->main->actionRun->setEnabled(false);
      this->ui->main->actionStart_capture->setEnabled(false);
      this->ui->main->actionStop_capture->setEnabled(false);
      break;

    case RUNNING:
      stateString = QString("Running");

      if (this->appConfig->profile.getType() == SUSCAN_SOURCE_TYPE_SDR)
        this->ui->spectrum->setCaptureMode(MainSpectrum::CAPTURE);
      else
        this->ui->spectrum->setCaptureMode(MainSpectrum::REPLAY);

      this->ui->main->actionRun->setEnabled(true);
      this->ui->main->actionRun->setChecked(true);
      this->ui->main->actionStart_capture->setEnabled(false);
      this->ui->main->actionStop_capture->setEnabled(true);
      this->ui->panoramicDialog->setBannedDevice(
            QString::fromStdString(
              this->appConfig->profile.getDevice().getDesc()));
      break;

    case RESTARTING:
      stateString = QString("Restarting...");
      this->ui->main->actionRun->setEnabled(false);
      this->ui->main->actionStart_capture->setEnabled(false);
      this->ui->main->actionStop_capture->setEnabled(false);
      break;
  }

  this->ui->inspectorPanel->setState(
        this->state == RUNNING
        ? InspectorPanel::State::ATTACHED
        : InspectorPanel::State::DETACHED);

  if (config->getType() == SUSCAN_SOURCE_TYPE_SDR) {
    sourceDesc = QString::fromStdString(dev.getDesc());
  } else {
    QFileInfo fi = QFileInfo(QString::fromStdString(config->getPath()));
    sourceDesc = fi.fileName();
  }

  this->owner->setWindowTitle(
        "SigDigger - "
        + sourceDesc
        + " - " + stateString);
}

void
UIMediator::connectMainWindow(void)
{
  connect(
        this->ui->main->actionSetup,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerSetup(bool)));

  connect(
        this->ui->main->actionOptions,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerSetup(bool)));

  connect(
        this->ui->main->actionImport_profile,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerImport(bool)));

  connect(
        this->ui->main->actionExport_profile,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerExport(bool)));

  connect(
        this->ui->main->actionDevices,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerDevices(bool)));

  connect(
        this->ui->main->actionStart_capture,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerStart(bool)));

  connect(
        this->ui->main->actionStop_capture,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerStop(bool)));

  connect(
        this->ui->main->actionQuit,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerQuit(bool)));

  connect(
        this->ui->main->actionRun,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onToggleCapture(bool)));

#ifndef __APPLE__
  connect(
        this->ui->main->action_Full_screen,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onToggleFullScreen(bool)));
#endif // __APLE__

  connect(
        this->ui->main->actionAbout,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onToggleAbout(bool)));

  connect(
        this->ui->main->mainTab,
        SIGNAL(tabCloseRequested(int)),
        this,
        SLOT(onCloseInspectorTab(int)));

  connect(
        this->ui->main->actionPanoramicSpectrum,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerPanoramicSpectrum(bool)));

  connect(
        this->ui->main->actionLogMessages,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerLogMessages(void)));

  connect(
        this->ui->main->action_Background_tasks,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerBackgroundTasks(void)));

  connect(
        this->ui->main->actionAddBookmark,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onAddBookmark(void)));

  connect(
        this->ui->addBookmarkDialog,
        SIGNAL(accepted(void)),
        this,
        SLOT(onBookmarkAccepted(void)));

  connect(
        this->ui->main->actionManageBookmarks,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onOpenBookmarkManager(void)));

  connect(
        this->ui->bookmarkManagerDialog,
        SIGNAL(frequencySelected(qint64)),
        this,
        SLOT(onJumpToBookmark(qint64)));

  connect(
        this->ui->bookmarkManagerDialog,
        SIGNAL(bookmarkChanged(void)),
        this,
        SLOT(onBookmarkChanged(void)));
}

UIMediator::UIMediator(QMainWindow *owner, AppUI *ui)
{
  this->owner = owner;
  this->ui = ui;

  // Configure audio preview
  this->audioPanelDock = new QDockWidget("Audio preview", owner);
  this->audioPanelDock->setWidget(this->ui->audioPanel);
  owner->addDockWidget(Qt::RightDockWidgetArea, this->audioPanelDock);

  // Configure Source Panel
  this->sourcePanelDock = new QDockWidget("Source", owner);
  this->sourcePanelDock->setWidget(this->ui->sourcePanel);
  owner->addDockWidget(Qt::RightDockWidgetArea, this->sourcePanelDock);

  // Configure Inspector Panel
  this->inspectorPanelDock = new QDockWidget("Inspection", owner);
  this->inspectorPanelDock->setWidget(this->ui->inspectorPanel);
  owner->addDockWidget(Qt::RightDockWidgetArea, this->inspectorPanelDock);

  // Configure FFT
  this->fftPanelDock = new QDockWidget("FFT", owner);
  this->fftPanelDock->setWidget(this->ui->fftPanel);
  owner->addDockWidget(Qt::RightDockWidgetArea, this->fftPanelDock);

  // Add baseband analyzer tab
  this->ui->main->mainTab->addTab(this->ui->spectrum, "Radio spectrum");

  // Sort panels
  owner->tabifyDockWidget(this->sourcePanelDock, this->inspectorPanelDock);
  owner->tabifyDockWidget(this->inspectorPanelDock, this->fftPanelDock);

  this->sourcePanelDock->raise();

  // Configure main spectrum
  this->ui->spectrum->setPaletteGradient(
        this->ui->fftPanel->getPaletteGradient());

  // Create background task controller dialog
  this->ui->backgroundTasksDialog->setController(
        Suscan::Singleton::get_instance()->getBackgroundTaskController());

  this->connectMainWindow();
  this->connectSpectrum();
  this->connectSourcePanel();
  this->connectFftPanel();
  this->connectAudioPanel();
  this->connectInspectorPanel();
  this->connectDeviceDialog();
  this->connectPanoramicDialog();
}

void
UIMediator::setBandwidth(unsigned int bw)
{
  this->ui->spectrum->setFilterBandwidth(bw);
  this->ui->inspectorPanel->setBandwidth(bw);
}

void
UIMediator::setProcessRate(unsigned int rate)
{
  this->ui->sourcePanel->setProcessRate(rate);
}

void
UIMediator::setSampleRate(unsigned int rate)
{
  if (this->rate != rate) {
    unsigned int bw = rate / 30;

    SUFREQ audioBw = SIGDIGGER_AUDIO_INSPECTOR_BANDWIDTH;

    if (audioBw > rate / 2)
      audioBw = rate / 2;

    this->ui->fftPanel->setSampleRate(rate);
    this->ui->inspectorPanel->setBandwidthLimits(0, rate);
    this->ui->spectrum->setSampleRate(rate);
    this->ui->sourcePanel->setSampleRate(rate);
    this->ui->inspectorPanel->setBandwidth(bw);
    this->ui->spectrum->setFilterBandwidth(bw);

    this->ui->audioPanel->setBandwidth(static_cast<float>(audioBw));
    this->rate = rate;
  }
}

void
UIMediator::setAudioRecordState(bool state)
{
  this->ui->audioPanel->setRecordState(state);
}

void
UIMediator::setAudioRecordSize(quint64 size)
{
  this->ui->audioPanel->setCaptureSize(size);
}

void
UIMediator::setAudioRecordIORate(qreal rate)
{
  this->ui->audioPanel->setIORate(rate);
}

void
UIMediator::setState(State state)
{
  this->state = state;
  this->refreshUI();
}

UIMediator::State
UIMediator::getState(void) const
{
  return this->state;
}

void
UIMediator::notifySourceInfo(Suscan::AnalyzerSourceInfo const &info)
{
  this->ui->spectrum->setFrequencyLimits(
        static_cast<qint64>(info.getMinFrequency()),
        static_cast<qint64>(info.getMaxFrequency()));

  this->ui->spectrum->setFreqs(
        static_cast<qint64>(info.getFrequency()),
        static_cast<qint64>(info.getLnbFrequency()),
        true); // Silent update (important!)

  this->ui->sourcePanel->applySourceInfo(info);
}

void
UIMediator::setPanSpectrumRunning(bool running)
{
  this->ui->panoramicDialog->setRunning(running);
}

void
UIMediator::resetRawInspector(qreal fs)
{
  this->ui->inspectorPanel->resetRawInspector(fs);
}

void
UIMediator::feedRawInspector(const SUCOMPLEX *data, size_t size)
{
  this->ui->inspectorPanel->feedRawInspector(data, size);
}


void
UIMediator::setMinPanSpectrumBw(quint64 bw)
{
  this->ui->panoramicDialog->setMinBwForZoom(bw);
}

void
UIMediator::feedPanSpectrum(
    quint64 minFreq,
    quint64 maxFreq,
    float *data,
    size_t size)
{
  this->ui->panoramicDialog->feed(minFreq, maxFreq, data, size);
}

void
UIMediator::refreshDevicesDone(void)
{
  this->ui->deviceDialog->refreshDone();
  this->ui->configDialog->notifySingletonChanges();
}

QMessageBox::StandardButton
UIMediator::shouldReduceRate(
    QString const &label,
    unsigned int requested,
    unsigned int proposed)
{
  QMessageBox::StandardButton reply = QMessageBox::No;

  if (!this->appConfig->disableHighRateWarning) {
      QCheckBox *cb = new QCheckBox("Don't ask again");
      QMessageBox msgbox;
      msgbox.setText(
            "The sample rate of profile \""
            + label
            + "\" is unusually big ("
            + QString::number(requested)
            + "). Decimate it down to "
            + QString::number(proposed)
            + "?");
      msgbox.setWindowTitle("Sample rate too high");
      msgbox.setIcon(QMessageBox::Icon::Question);
      msgbox.addButton(QMessageBox::Yes);
      msgbox.addButton(QMessageBox::No);
      msgbox.addButton(QMessageBox::Cancel);
      msgbox.setDefaultButton(QMessageBox::Cancel);
      msgbox.setCheckBox(cb);

      QObject::connect(
            cb,
            &QCheckBox::stateChanged,
            [this](int state) {
        if (static_cast<Qt::CheckState>(state) == Qt::CheckState::Checked) {
              this->appConfig->disableHighRateWarning = true;
          }
      });

      reply = static_cast<QMessageBox::StandardButton>(msgbox.exec());
  }

  return reply;
}

void
UIMediator::notifyStartupErrors(void)
{
  if (this->ui->logDialog->haveErrorMessages()) {
      QMessageBox msgbox(this->owner);
      msgbox.setTextFormat(Qt::RichText);
      msgbox.setText(
            "Errors occurred during startup:" +
            this->ui->logDialog->getErrorHtml());
      msgbox.setWindowTitle("SigDigger startup");
      msgbox.setIcon(QMessageBox::Icon::Warning);
      msgbox.addButton(QMessageBox::Ok);
      msgbox.setDefaultButton(QMessageBox::Ok);

      msgbox.exec();
  }
}

void
UIMediator::feedPSD(const Suscan::PSDMessage &msg)
{
  this->setSampleRate(msg.getSampleRate());
  this->setProcessRate(msg.getMeasuredSampleRate());
  this->averager.feed(msg);
  this->ui->spectrum->feed(
        this->averager.get(),
        static_cast<int>(this->averager.size()));
}

void
UIMediator::setCaptureSize(quint64 size)
{
  this->ui->sourcePanel->setCaptureSize(size);
}

Inspector *
UIMediator::lookupInspector(Suscan::InspectorId handle) const
{
  Inspector *entry = nullptr;

  try {
    entry = this->ui->inspectorTable.at(handle);
  } catch (std::out_of_range &) { }

  return entry;
}

bool
UIMediator::getAudioRecordState(void) const
{
  return this->ui->audioPanel->getRecordState();
}

std::string
UIMediator::getAudioRecordSavePath(void) const
{
  return this->ui->audioPanel->getRecordSavePath();
}

bool
UIMediator::getPanSpectrumDevice(Suscan::Source::Device &dev) const
{
  return this->ui->panoramicDialog->getSelectedDevice(dev);
}

bool
UIMediator::getPanSpectrumRange(qint64 &min, qint64 &max) const
{
  if (!this->ui->panoramicDialog->invalidRange()) {
    min = static_cast<qint64>(this->ui->panoramicDialog->getMinFreq());
    max = static_cast<qint64>(this->ui->panoramicDialog->getMaxFreq());
    return true;
  }

  return false;
}

unsigned int
UIMediator::getPanSpectrumRttMs(void) const
{
  return this->ui->panoramicDialog->getRttMs();
}

float
UIMediator::getPanSpectrumRelBw(void) const
{
  return this->ui->panoramicDialog->getRelBw();
}

float
UIMediator::getPanSpectrumGain(QString const &name) const
{
  return this->ui->panoramicDialog->getGain(name);
}

SUFREQ
UIMediator::getPanSpectrumLnbOffset(void) const
{
  return this->ui->panoramicDialog->getLnbOffset();
}

float
UIMediator::getPanSpectrumPreferredSampleRate(void) const
{
  return this->ui->panoramicDialog->getPreferredSampleRate();
}

QString
UIMediator::getPanSpectrumStrategy(void) const
{
  return this->ui->panoramicDialog->getStrategy();
}

QString
UIMediator::getPanSpectrumPartition(void) const
{
  return this->ui->panoramicDialog->getPartitioning();
}

QString
UIMediator::getInspectorTabTitle(Suscan::InspectorMessage const &msg)
{
  QString result = " in "
      + SuWidgetsHelpers::formatQuantity(
        msg.getChannel().fc + msg.getChannel().ft,
        "Hz");

  if (msg.getClass() == "psk")
    return "PSK inspector" + result;
  else if (msg.getClass() == "fsk")
    return "FSK inspector" + result;
  else if (msg.getClass() == "ask")
    return "ASK inspector" + result;

  return "Generic inspector" + result;
}

Inspector *
UIMediator::addInspectorTab(
    Suscan::InspectorMessage const &msg,
    Suscan::InspectorId &oId)
{

  int index;
  Inspector *insp = new Inspector(
        this->ui->main->mainTab,
        msg,
        *this->appConfig);

  oId = this->ui->lastId++;

  insp->setId(oId);

  index = this->ui->main->mainTab->addTab(
        insp,
        UIMediator::getInspectorTabTitle(msg));

  this->ui->inspectorTable[oId] = insp;
  this->ui->main->mainTab->setCurrentIndex(index);

  return insp;
}

void
UIMediator::detachAllInspectors()
{
  for (auto p = this->ui->inspectorTable.begin();
       p != this->ui->inspectorTable.end();
       ++p) {
    p->second->setAnalyzer(nullptr);
    p->second = nullptr;
  }
}

void
UIMediator::setStatusMessage(QString const &message)
{
  this->ui->main->statusBar->showMessage(message);
}

void
UIMediator::setRecordState(bool state)
{
  this->ui->sourcePanel->setRecordState(state);
}

void
UIMediator::setIORate(qreal rate)
{
  this->ui->sourcePanel->setIORate(rate);
}

void
UIMediator::refreshProfile(void)
{
  qint64 min = 0, max = 0;
  this->ui->sourcePanel->setProfile(&this->appConfig->profile);
  this->ui->configDialog->setProfile(this->appConfig->profile);

  if (this->appConfig->profile.getType() == SUSCAN_SOURCE_TYPE_SDR) {
    min = static_cast<qint64>(
          this->appConfig->profile.getDevice().getMinFreq());
    max = static_cast<qint64>(
          this->appConfig->profile.getDevice().getMaxFreq());
  } else {
    min = SIGDIGGER_MIN_RADIO_FREQ;
    max = SIGDIGGER_MAX_RADIO_FREQ;
  }

  // Dummy device should not accept modifications if we don't accept
  // them in the ConfigDialog.
  //
  // if (max - min < 1000) {
  //   min = SIGDIGGER_UI_MEDIATOR_DEFAULT_MIN_FREQ;
  //   max = SIGDIGGER_UI_MEDIATOR_DEFAULT_MAX_FREQ;
  // }

  this->ui->spectrum->setFrequencyLimits(min, max);
  this->ui->spectrum->setFreqs(
        static_cast<qint64>(this->appConfig->profile.getFreq()),
        static_cast<qint64>(this->appConfig->profile.getLnbFreq()));
  this->setSampleRate(this->appConfig->profile.getDecimatedSampleRate());
}

Suscan::Source::Config *
UIMediator::getProfile(void) const
{
  return &this->appConfig->profile;
}

Suscan::AnalyzerParams *
UIMediator::getAnalyzerParams(void) const
{
  return &this->appConfig->analyzerParams;
}

unsigned int
UIMediator::getFftSize(void) const
{
  return this->appConfig->analyzerParams.windowSize;
}

Suscan::Serializable *
UIMediator::allocConfig(void)
{
  return this->appConfig = new AppConfig(this->ui);
}

void
UIMediator::saveUIConfig(void)
{
  this->appConfig->x = this->owner->geometry().x();
  this->appConfig->y = this->owner->geometry().y();
  this->appConfig->width  = this->owner->geometry().width();
  this->appConfig->height = this->owner->geometry().height();

  this->appConfig->enabledBandPlans.clear();

  for (auto p : this->bandPlanMap)
    if (p.second->isChecked())
      this->appConfig->enabledBandPlans.push_back(p.first);
}

void
UIMediator::applyConfig(void)
{
  // Apply window config
  QRect rec = QGuiApplication::primaryScreen()->geometry();
  unsigned int savedBw = this->appConfig->bandwidth;
  int savedLoFreq = this->appConfig->loFreq;

  if (this->appConfig->x == -1)
    this->appConfig->x = (rec.width() - this->appConfig->width) / 2;
  if (this->appConfig->y == -1)
    this->appConfig->y = (rec.height() - this->appConfig->height) / 2;

  this->owner->setGeometry(
        this->appConfig->x,
        this->appConfig->y,
        this->appConfig->width,
        this->appConfig->height);

  if (this->appConfig->fullScreen)
    this->owner->setWindowState(
        this->owner->windowState() | Qt::WindowFullScreen);

  // The following controls reflect elements of the configuration that are
  // not owned by them. We need to set them manually.
  this->ui->configDialog->setColors(this->appConfig->colors);
  this->ui->configDialog->setGuiConfig(this->appConfig->guiConfig);
  this->ui->panoramicDialog->setColors(this->appConfig->colors);
  this->ui->spectrum->setColorConfig(this->appConfig->colors);
  this->ui->spectrum->setGuiConfig(this->appConfig->guiConfig);
  this->ui->spectrum->setExpectedRate(
        static_cast<int>(1.f / this->appConfig->analyzerParams.psdUpdateInterval));
  this->ui->inspectorPanel->setColorConfig(this->appConfig->colors);
  this->ui->fftPanel->setWindowFunction(this->appConfig->analyzerParams.windowFunction);
  this->ui->fftPanel->setFftSize(this->appConfig->analyzerParams.windowSize);
  this->ui->fftPanel->setRefreshRate(
        static_cast<unsigned int>(1.f / this->appConfig->analyzerParams.psdUpdateInterval));
  this->ui->fftPanel->setDefaultFftSize(SIGDIGGER_FFT_WINDOW_SIZE);
  this->ui->fftPanel->setDefaultRefreshRate(SIGDIGGER_FFT_REFRESH_RATE);

  // Apply enabled bandplans
  for (auto p : this->appConfig->enabledBandPlans)
    if (this->bandPlanMap.find(p) != this->bandPlanMap.cend()) {
      FrequencyAllocationTable *table =
          this->ui->spectrum->getFAT(QString::fromStdString(p));

      if (table != nullptr) {
        this->bandPlanMap[p]->setChecked(true);
        this->ui->spectrum->pushFAT(table);
      }
    }
  // The rest of them are automatically deserialized
  this->ui->sourcePanel->applyConfig();
  this->ui->fftPanel->applyConfig();
  this->ui->inspectorPanel->applyConfig();
  this->ui->audioPanel->applyConfig();
  this->ui->panoramicDialog->applyConfig();

  this->refreshProfile();

  // Apply loFreq and bandwidth config AFTER profile has been set.
  this->ui->spectrum->setLoFreq(savedLoFreq);
  if (savedBw > 0)
    this->setBandwidth(savedBw);

  // Artificially trigger slots to synchronize UI
  this->onPaletteChanged();
  this->onRangesChanged();
  this->onAveragerChanged();
  this->onThrottleConfigChanged();
  this->onTimeSpanChanged();
  this->onTimeStampsChanged();
  this->onBookmarksButtonChanged();
}

UIMediator::~UIMediator()
{

}

/////////////////////////////// Slots //////////////////////////////////////////
void
UIMediator::onTriggerSetup(bool)
{
  this->ui->configDialog->setProfile(*this->getProfile());
  this->ui->configDialog->setAnalyzerParams(*this->getAnalyzerParams());

  if (this->ui->configDialog->run()) {
    this->appConfig->analyzerParams = this->ui->configDialog->getAnalyzerParams();
    this->ui->fftPanel->setFftSize(this->getFftSize());
    this->appConfig->colors = this->ui->configDialog->getColors();
    this->appConfig->guiConfig = this->ui->configDialog->getGuiConfig();
    this->ui->spectrum->setColorConfig(this->appConfig->colors);
    this->ui->spectrum->setGuiConfig(this->appConfig->guiConfig);
    this->ui->inspectorPanel->setColorConfig(this->appConfig->colors);
    this->setProfile(this->ui->configDialog->getProfile());
  }
}

void
UIMediator::onToggleCapture(bool state)
{
  if (state) {
    emit captureStart();
  } else {
    emit captureEnd();
  }
}

void
UIMediator::onToggleFullScreen(bool)
{
  this->owner->setWindowState(this->owner->windowState() ^ Qt::WindowFullScreen);
  this->appConfig->fullScreen =
      (this->owner->windowState() & Qt::WindowFullScreen) != 0;
}

void
UIMediator::onToggleAbout(bool)
{
  this->ui->aboutDialog->exec();
}

void
UIMediator::closeInspectorTab(Inspector *insp)
{
  if (insp != nullptr) {
    Suscan::Analyzer *analyzer = insp->getAnalyzer();
    if (analyzer != nullptr) {
      analyzer->closeInspector(insp->getHandle(), 0);
    } else {
      this->ui->main->mainTab->removeTab(
            this->ui->main->mainTab->indexOf(insp));
      this->ui->inspectorTable.erase(insp->getId());
      delete insp;
    }
  }
}

void
UIMediator::onCloseInspectorTab(int ndx)
{
  QWidget *widget = this->ui->main->mainTab->widget(ndx);

  if (widget != nullptr && widget != this->ui->spectrum) {
    Inspector *insp = static_cast<Inspector *>(widget);
    this->closeInspectorTab(insp);
  }
}

void
UIMediator::onTriggerStart(bool)
{
  emit captureStart();
}

void
UIMediator::onTriggerStop(bool)
{
  emit captureEnd();
}

void
UIMediator::onTriggerImport(bool)
{
  QString path = QFileDialog::getOpenFileName(
        this,
        "Import SigDigger profile",
        QString(),
        "SigDigger profile files (*.xml)");

  if (!path.isEmpty()) {
    std::ifstream in(
          path.toStdString().c_str(),
          std::ifstream::ate | std::ifstream::binary);

    if (!in.is_open()) {
      QMessageBox::warning(
            this->ui->main->centralWidget,
            "Cannot open file",
            "Selected profile file could not be opened. Please check its "
            "access rights or whether the file still exists and try again.",
            QMessageBox::Ok);
    } else if (in.tellg() > SIGDIGGER_PROFILE_FILE_MAX_SIZE) {
        QMessageBox::critical(
              this->ui->main->centralWidget,
              "Cannot open file",
              "Invalid profile file",
              QMessageBox::Ok);
    } else {
      std::vector<char> data;
      data.resize(static_cast<size_t>(in.tellg()));
      in.seekg(0);
      in.read(&data[0], static_cast<int>(data.size()));

      if (in.eof()) {
        QMessageBox::critical(
              this->ui->main->centralWidget,
              "Cannot open file",
              "Unexpected end of file",
              QMessageBox::Ok);
      } else {
        try {
          Suscan::Object envelope(
                path.toStdString(),
                reinterpret_cast<const uint8_t *>(data.data()),
                data.size());
          Suscan::Object profObj = envelope[0];
          QString className = QString::fromStdString(profObj.getClass());

          if (className.isEmpty()) {
            QMessageBox::warning(
                  this->ui->main->centralWidget,
                  "Cannot open file",
                  "Invalid file: serialized object class is undefined",
                  QMessageBox::Ok);
          } else if (className != "source_config") {
            QMessageBox::warning(
                  this->ui->main->centralWidget,
                  "Cannot open file",
                  "Invalid file: objects of class `" + className + "' are "
                  "not profiles.",
                  QMessageBox::Ok);
          } else {
            try {
              Suscan::Source::Config config(profObj);
              this->setProfile(config);
            } catch (Suscan::Exception &e) {
              QMessageBox::critical(
                    this->ui->main->centralWidget,
                    "Cannot open file",
                    "Failed to create source configuration from profile object: "
                    + QString(e.what()),
                    QMessageBox::Ok);
            }
          }
        } catch (Suscan::Exception &e) {
          QMessageBox::critical(
                this->ui->main->centralWidget,
                "Cannot open file",
                "Failed to load profile object from file: " + QString(e.what()),
                QMessageBox::Ok);
        }
      }
    }
  }
}

void
UIMediator::onTriggerExport(bool)
{
  bool done = false;

  do {
    QFileDialog dialog(this);

    dialog.setFileMode(QFileDialog::FileMode::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setWindowTitle(QString("Save current profile"));
    dialog.setNameFilter(QString("SigDigger profile files (*.xml)"));

    if (dialog.exec()) {
      QString path = dialog.selectedFiles().first();
      try {
        Suscan::Object profileObj = this->getProfile()->serialize();
        Suscan::Object envelope(SUSCAN_OBJECT_TYPE_SET);

        envelope.append(profileObj);

        std::vector<char> data = envelope.serialize();
        std::ofstream of(path.toStdString().c_str(), std::ofstream::binary);

        if (!of.is_open()) {
          QMessageBox::warning(
                this->ui->main->centralWidget,
                "Cannot open file",
                "Cannote save file in the specified location. Please choose "
                "a different location and try again.",
                QMessageBox::Ok);
        } else {
          of.write(&data[0], static_cast<int>(data.size()));

          if (of.fail()) {
            QMessageBox::critical(
                  this->ui->main->centralWidget,
                  "Cannot serialize profile",
                  "Write error. Profile has not been saved. Please try again.",
                  QMessageBox::Ok);
          } else {
            done = true;
          }
        }
      } catch (Suscan::Exception &e) {
        QMessageBox::critical(
              this->ui->main->centralWidget,
              "Cannot serialize profile",
              "Serialization of profile data failed: " + QString(e.what()),
              QMessageBox::Ok);
      }
    } else {
      done = true;
    }
  } while (!done);
}

void
UIMediator::onTriggerDevices(bool)
{
  this->ui->deviceDialog->run();
}

void
UIMediator::onTriggerClear(bool)
{
  this->clearRecent();
  this->finishRecent();

  emit recentCleared();
}

void
UIMediator::onTriggerRecent(bool)
{
  QAction *sender = qobject_cast<QAction *>(QObject::sender());

  emit recentSelected(sender->text());
}

void
UIMediator::onTriggerQuit(bool)
{
  emit uiQuit();
}

void
UIMediator::onTriggerPanoramicSpectrum(bool)
{
  this->ui->panoramicDialog->run();
}

void
UIMediator::onTriggerBandPlan(void)
{
  QObject* obj = sender();
  QAction *asAction = qobject_cast<QAction *>(obj);

  if (asAction->isChecked()) {
    FrequencyAllocationTable *fat = this->ui->spectrum->getFAT(asAction->text());
    if (fat != nullptr)
      this->ui->spectrum->pushFAT(fat);
  } else {
    this->ui->spectrum->removeFAT(asAction->text());
  }
}

void
UIMediator::onTriggerLogMessages(void)
{
  this->ui->logDialog->show();
}

void
UIMediator::onTriggerBackgroundTasks(void)
{
  this->ui->backgroundTasksDialog->show();
}

void
UIMediator::onAddBookmark(void)
{
  this->ui->addBookmarkDialog->setFrequencyHint(
        this->ui->spectrum->getLoFreq() + this->ui->spectrum->getCenterFreq());

  this->ui->addBookmarkDialog->setNameHint(
        QString::asprintf(
          "%s signal @ %s",
          AudioPanel::demodToStr(this->ui->audioPanel->getDemod()).c_str(),
          SuWidgetsHelpers::formatQuantity(
            this->ui->spectrum->getLoFreq() + this->ui->spectrum->getCenterFreq(),
            4,
            "Hz").toStdString().c_str()));

  this->ui->addBookmarkDialog->show();
}

void
UIMediator::onBookmarkAccepted(void)
{
  emit bookmarkAdded(
        this->ui->addBookmarkDialog->name(),
        this->ui->addBookmarkDialog->frequency(),
        this->ui->addBookmarkDialog->color());
}

void
UIMediator::onOpenBookmarkManager(void)
{
  this->ui->bookmarkManagerDialog->show();
}

void
UIMediator::onJumpToBookmark(qint64 frequency)
{
  this->ui->spectrum->setCenterFreq(frequency);
  this->ui->spectrum->setLoFreq(0);

  this->onFrequencyChanged(frequency);
}

void
UIMediator::onBookmarkChanged(void)
{
  this->ui->spectrum->updateOverlay();
}
