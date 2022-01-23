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
#include <util/compat-statvfs.h>
#include <SuWidgetsHelpers.h>

#include "UIMediator.h"
#include "ProfileConfigTab.h"

#include <QGuiApplication>
#include <QDockWidget>
#include <QMessageBox>
#include <QScreen>
#include <QTimeSlider.h>

#include <fstream>

// Ui control dependencies
#include <QAction>
#include "ui_MainWindow.h"
#include "MainWindow.h"
#include "MainSpectrum.h"
#include "AudioPanel.h"
#include "FftPanel.h"
#include "InspectorPanel.h"
#include "SourcePanel.h"
#include "BookmarkManagerDialog.h"
#include "BackgroundTasksDialog.h"
#include "AddBookmarkDialog.h"
#include "PanoramicDialog.h"
#include "Inspector.h"
#include "LogDialog.h"
#include "ConfigDialog.h"
#include "DeviceDialog.h"
#include "AboutDialog.h"

#ifdef interface
#  undef interface
#endif /* interface */

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
UIMediator::setProfile(Suscan::Source::Config const &prof, bool restart)
{
  this->appConfig->profile = prof;
  this->refreshProfile();
  this->refreshUI();
  emit profileChanged(restart);
}

void
UIMediator::refreshUI(void)
{
  QString stateString;
  QString sourceDesc;
  bool    enableTimeSlider = false;
  bool    isRealTime = false;

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

      enableTimeSlider =
          this->ui->spectrum->getCaptureMode() == MainSpectrum::REPLAY;
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

  if (config->getInterface() == SUSCAN_SOURCE_REMOTE_INTERFACE) {
    QString user = QString::fromStdString(config->getParam("user"));
    QString host = QString::fromStdString(config->getParam("host"));
    QString port = QString::fromStdString(config->getParam("port"));
    sourceDesc = "Remote analyzer on " + user + "@" + host + ":" + port;

    this->ui->spectrum->setGracePeriod(
          SIGDIGGER_UI_MEDIATOR_REMOTE_GRACE_PERIOD_MS);
  } else {
    if (config->getType() == SUSCAN_SOURCE_TYPE_SDR) {
      sourceDesc = QString::fromStdString(dev.getDesc());
      isRealTime = true;
    } else {
      QFileInfo fi = QFileInfo(QString::fromStdString(config->getPath()));
      sourceDesc = fi.fileName();
    }
    this->ui->spectrum->setGracePeriod(
          SIGDIGGER_UI_MEDIATOR_LOCAL_GRACE_PERIOD_MS);
  }

  this->ui->timeSlider->setEnabled(enableTimeSlider);
  this->ui->timeToolbar->setVisible(!isRealTime);

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
        SIGNAL(bookmarkSelected(BookmarkInfo)),
        this,
        SLOT(onJumpToBookmark(BookmarkInfo)));

  connect(
        this->ui->bookmarkManagerDialog,
        SIGNAL(bookmarkChanged(void)),
        this,
        SLOT(onBookmarkChanged(void)));

  connect(
        this->ui->spectrum,
        SIGNAL(modulationChanged(QString)),
        this,
        SLOT(onModulationChanged(QString)));

  this->ui->main->mainTab->tabBar()->setContextMenuPolicy(
        Qt::CustomContextMenu);

  connect(
        this->ui->main->mainTab->tabBar(),
        SIGNAL(customContextMenuRequested(const QPoint &)),
        this,
        SLOT(onInspectorMenuRequested(const QPoint &)));

}

UIMediator::UIMediator(QMainWindow *owner, AppUI *ui)
{
  this->owner = owner;
  this->ui = ui;

  this->ui->spectrum->addToolWidget(this->ui->audioPanel, "Audio preview");
  this->ui->spectrum->addToolWidget(this->ui->sourcePanel, "Signal source");
  this->ui->spectrum->addToolWidget(this->ui->inspectorPanel, "Inspection");
  this->ui->spectrum->addToolWidget(this->ui->fftPanel, "FFT");

  // Add baseband analyzer tab
  this->ui->main->mainTab->addTab(this->ui->spectrum, "Radio spectrum");

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
  this->connectTimeSlider();
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

  if (info.isSeekable()) {
    this->setSourceTimeStart(info.getSourceStartTime());
    this->setSourceTimeEnd(info.getSourceEndTime());
  }

  this->ui->sourcePanel->applySourceInfo(info);
}

void
UIMediator::notifyTimeStamp(struct timeval const &timestamp)
{
  this->ui->audioPanel->setTimeStamp(timestamp);
  this->setTimeStamp(timestamp);

  for (auto i : this->ui->inspectorTable)
    i.second->setTimeStamp(timestamp);
}

void
UIMediator::notifyOrbitReport(
    Suscan::InspectorId id,
    Suscan::OrbitReport const &report)
{
  if (id == SIGDIGGER_AUDIO_INSPECTOR_MAGIC_ID) {
    this->ui->audioPanel->notifyOrbitReport(report);
  } else {
    Inspector *insp;
    if ((insp = this->lookupInspector(id)) != nullptr)
      insp->notifyOrbitReport(report);
  }
}

void
UIMediator::notifyDisableCorrection(Suscan::InspectorId id)
{
  if (id == SIGDIGGER_AUDIO_INSPECTOR_MAGIC_ID) {
    this->ui->audioPanel->notifyDisableCorrection();
  } else {
    Inspector *insp;
    if ((insp = this->lookupInspector(id)) != nullptr)
      insp->disableCorrection();
  }
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
UIMediator::setCaptureSize(quint64 size)
{
  this->ui->sourcePanel->setCaptureSize(size);
}

void
UIMediator::setAnalyzerParams(Suscan::AnalyzerParams const &params)
{
  this->ui->spectrum->setExpectedRate(
        static_cast<int>(1.f / params.psdUpdateInterval));
  this->ui->fftPanel->setWindowFunction(params.windowFunction);
  this->ui->fftPanel->setFftSize(params.windowSize);
  this->ui->fftPanel->setRefreshRate(
        static_cast<unsigned int>(1.f / params.psdUpdateInterval));
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
  bool isRealTime = false;
  struct timeval tv, start, end;

  this->ui->sourcePanel->setProfile(&this->appConfig->profile);

  std::string user, pass, interface;

  user = this->getProfile()->getParam("user");
  pass = this->getProfile()->getParam("password");
  interface = this->getProfile()->getInterface();

  this->ui->configDialog->setProfile(this->appConfig->profile);

  if (this->appConfig->profile.getInterface() == SUSCAN_SOURCE_LOCAL_INTERFACE) {
    if (this->appConfig->profile.getType() == SUSCAN_SOURCE_TYPE_SDR) {
      min = static_cast<qint64>(
            this->appConfig->profile.getDevice().getMinFreq());
      max = static_cast<qint64>(
            this->appConfig->profile.getDevice().getMaxFreq());
        isRealTime = true;
    } else {
      min = SIGDIGGER_MIN_RADIO_FREQ;
      max = SIGDIGGER_MAX_RADIO_FREQ;

      this->ui->audioPanel->resetTimeStamp(
            this->appConfig->profile.getStartTime());
    }
  } else {
    struct timeval tv;
    // Remote sources receive time from the server
    min = SIGDIGGER_MIN_RADIO_FREQ;
    max = SIGDIGGER_MAX_RADIO_FREQ;

    gettimeofday(&tv, nullptr);
    this->ui->audioPanel->resetTimeStamp(tv);
  }

  // Dummy device should not accept modifications if we don't accept
  // them in the ConfigDialog.
  //
  // if (max - min < 1000) {
  //   min = SIGDIGGER_UI_MEDIATOR_DEFAULT_MIN_FREQ;
  //   max = SIGDIGGER_UI_MEDIATOR_DEFAULT_MAX_FREQ;
  // }

  if (isRealTime) {
    gettimeofday(&tv, nullptr);

    start = tv;
    start.tv_sec -= 1;
    start.tv_usec = 0;

    end = tv;
    end.tv_sec += 1;
    end.tv_usec = 0;
  } else {
    if (this->appConfig->profile.fileIsValid()) {
      start = this->appConfig->profile.getStartTime();
      end   = this->appConfig->profile.getEndTime();
    } else {
      start = this->appConfig->profile.getStartTime();
      end   = start;
      end.tv_sec += 1;
    }
    tv = start;
  }

  // Set cached members
  this->profileStart = start;
  this->profileEnd   = end;
  this->isRealTime   = isRealTime;

  // Configure timeslider
  this->ui->timeSlider->setEnabled(!isRealTime);
  this->ui->timeSlider->setStartTime(start);
  this->ui->timeSlider->setEndTime(end);
  this->ui->timeSlider->setTimeStamp(tv);

  // Configure audio panel
  this->ui->audioPanel->setRealTime(isRealTime);
  this->ui->audioPanel->setTimeLimits(start, end);

  // Configure spectrum
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
  this->appConfig->sidePanelRatio = this->ui->spectrum->sidePanelRatio();

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

  if (this->appConfig->sidePanelRatio >= 0)
    this->ui->spectrum->setSidePanelRatio(this->appConfig->sidePanelRatio);

  // The following controls reflect elements of the configuration that are
  // not owned by them. We need to set them manually.
  this->ui->configDialog->setColors(this->appConfig->colors);
  this->ui->configDialog->setGuiConfig(this->appConfig->guiConfig);
  this->ui->configDialog->setTleSourceConfig(this->appConfig->tleSourceConfig);
  this->ui->panoramicDialog->setColors(this->appConfig->colors);
  this->ui->spectrum->setColorConfig(this->appConfig->colors);
  this->ui->audioPanel->setColorConfig(this->appConfig->colors);
  this->ui->inspectorPanel->setColorConfig(this->appConfig->colors);
  this->ui->spectrum->setGuiConfig(this->appConfig->guiConfig);

  this->setAnalyzerParams(this->appConfig->analyzerParams);

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

  // Configure spectrum
  this->ui->spectrum->setUnits(
        this->ui->fftPanel->getUnitName(),
        this->ui->fftPanel->getdBPerUnit(),
        this->ui->fftPanel->getCompleteZeroPoint());
  this->ui->spectrum->setGain(this->ui->fftPanel->getGain());

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
  auto sus = Suscan::Singleton::get_instance();

  this->ui->configDialog->setProfile(*this->getProfile());
  this->ui->configDialog->setAnalyzerParams(*this->getAnalyzerParams());
  this->ui->configDialog->setColors(this->appConfig->colors);
  this->ui->configDialog->setTleSourceConfig(this->appConfig->tleSourceConfig);

  if (sus->haveQth())
    this->ui->configDialog->setLocation(sus->getQth());

  if (this->ui->configDialog->run()) {
    this->appConfig->analyzerParams = this->ui->configDialog->getAnalyzerParams();
    this->ui->fftPanel->setFftSize(this->getFftSize());

    if (this->ui->configDialog->profileChanged())
      this->setProfile(
          this->ui->configDialog->getProfile(),
          this->ui->configDialog->sourceNeedsRestart());

    if (this->ui->configDialog->colorsChanged()) {
      this->appConfig->colors = this->ui->configDialog->getColors();
      this->ui->spectrum->setColorConfig(this->appConfig->colors);
      this->ui->inspectorPanel->setColorConfig(this->appConfig->colors);
      this->ui->audioPanel->setColorConfig(this->appConfig->colors);
    }

    if (this->ui->configDialog->guiChanged()) {
      this->appConfig->guiConfig = this->ui->configDialog->getGuiConfig();
      this->ui->spectrum->setGuiConfig(this->appConfig->guiConfig);
    }

    if (this->ui->configDialog->tleSourceConfigChanged()) {
      this->appConfig->tleSourceConfig =
          this->ui->configDialog->getTleSourceConfig();
    }
    if (this->ui->configDialog->locationChanged()) {
      Suscan::Location loc = this->ui->configDialog->getLocation();
      sus->setQth(loc);
      this->ui->audioPanel->setQth(loc.getQth());
    }
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

  this->ui->addBookmarkDialog->setBandwidthHint(this->ui->spectrum->getBandwidth());
  this->ui->addBookmarkDialog->setModulationHint(QString::fromStdString(AudioPanel::demodToStr(this->ui->audioPanel->getDemod())));

  this->ui->addBookmarkDialog->show();
}

void
UIMediator::onBookmarkAccepted(void)
{
  BookmarkInfo info;
  info.name = this->ui->addBookmarkDialog->name();
  info.frequency = this->ui->addBookmarkDialog->frequency();
  info.color = this->ui->addBookmarkDialog->color();
  info.lowFreqCut = this->ui->spectrum->computeLowCutFreq(this->ui->addBookmarkDialog->bandwidth());
  info.highFreqCut = this->ui->spectrum->computeHighCutFreq(this->ui->addBookmarkDialog->bandwidth());
  info.modulation = this->ui->addBookmarkDialog->modulation();

  emit bookmarkAdded(info);
}

void
UIMediator::onOpenBookmarkManager(void)
{
  this->ui->bookmarkManagerDialog->show();
}

void
UIMediator::onJumpToBookmark(BookmarkInfo info)
{
  this->ui->spectrum->setCenterFreq(info.frequency);
  this->ui->spectrum->setLoFreq(0);

  if (!info.modulation.isEmpty()) {
    this->ui->audioPanel->setDemod(AudioPanel::strToDemod(info.modulation.toStdString()));
  }

  if (info.bandwidth() != 0) {
    this->setBandwidth(info.bandwidth());
  }

  this->onFrequencyChanged(info.frequency);
}

void
UIMediator::onBookmarkChanged(void)
{
  this->ui->spectrum->updateOverlay();
}

void
UIMediator::onModulationChanged(QString newModulation)
{
  this->ui->audioPanel->setDemod(AudioPanel::strToDemod(newModulation.toStdString()));
}
