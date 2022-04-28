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
#include "BookmarkManagerDialog.h"
#include "BackgroundTasksDialog.h"
#include "AddBookmarkDialog.h"
#include "PanoramicDialog.h"
#include "LogDialog.h"
#include "ConfigDialog.h"
#include "DeviceDialog.h"
#include "AboutDialog.h"
#include "QuickConnectDialog.h"

// Tool widget controls
#include <ToolWidgetFactory.h>
#include <TabWidgetFactory.h>
#include <UIListenerFactory.h>

#if defined(_WIN32) && defined(interface)
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

  connect(action, SIGNAL(triggered(bool)), this, SLOT(onTriggerBandPlan()));
}

void
UIMediator::clearRecent()
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
UIMediator::finishRecent()
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

QMainWindow *
UIMediator::getMainWindow() const
{
  return this->owner;
}

MainSpectrum *
UIMediator::getMainSpectrum() const
{
  return this->ui->spectrum;
}

Averager *
UIMediator::getSpectrumAverager()
{
  return &this->averager;
}

AppConfig *
UIMediator::getAppConfig() const
{
  return this->appConfig;
}

void
UIMediator::configureUIComponent(UIComponent *comp)
{
  try {
    Suscan::Object config;
    config = this->appConfig->getComponentConfig(comp->factoryName());
    comp->loadSerializedConfig(config);
  } catch (Suscan::Exception &) {
    comp->assertConfig();
    comp->applyConfig();
  }
}

void
UIMediator::registerUIComponent(UIComponent *comp)
{
  assert(m_components.indexOf(comp) == -1);

  m_components.push_back(comp);
}

void
UIMediator::unregisterUIComponent(UIComponent *comp)
{
  int index = m_components.indexOf(comp);

  if (index != -1) {
    this->appConfig->setComponentConfig(
          comp->factoryName(),
          comp->getSerializedConfig());
    m_components.removeAt(index);
  }
}


bool
UIMediator::addTabWidget(TabWidget *tabWidget)
{
  // Adding a tab widget involves:
  // 1. Checking whether it exists.
  // 2. Adding it to the tab list
  // 3. Register a slot to remove it from the tab list when destroyed
  // 4. Sync state
  // 5. Open a tab with the corresponding title
  // 6. Switch focus
  int index;
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  assert(m_tabWidgets.indexOf(tabWidget) == -1);

  m_tabWidgets.push_back(tabWidget);

  index = this->ui->main->mainTab->insertTab(
        this->ui->main->mainTab->count(),
        tabWidget,
        QString::fromStdString(tabWidget->getLabel()));

  this->configureUIComponent(tabWidget);

  if (s->haveQth())
    tabWidget->setQth(s->getQth());

  tabWidget->setTimeStamp(m_lastTimeStamp);
  tabWidget->setProfile(this->appConfig->profile);
  tabWidget->setState(m_state, m_analyzer);

  this->ui->main->mainTab->setCurrentIndex(index);

  return true;
}

bool
UIMediator::addUIListener(UIListener *listener)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  if (s->haveQth())
    listener->setQth(s->getQth());

  listener->setTimeStamp(m_lastTimeStamp);
  listener->setProfile(this->appConfig->profile);
  listener->setState(m_state, m_analyzer);

  listener->setParent(this);

  return true;
}

//
// NOTE: Adding a tab widget just exposes it. In any case deletes it.
// This is something that has to be handled later.
//

bool
UIMediator::closeTabWidget(TabWidget *tabWidget)
{
  // 1. Find whether the tab exists
  // 2. Remove from the tabWidget list
  // 3. Check whether it is a tab or a floating tab, close it accordingly

  int index;

  index = m_tabWidgets.indexOf(tabWidget);
  if (index == -1)
    return false;

  m_tabWidgets.removeAt(index);

  index = this->ui->main->mainTab->indexOf(tabWidget);
  if (index != -1) {
    this->ui->main->mainTab->removeTab(index);
    tabWidget->setParent(nullptr);
  } else {
    auto p = m_floatingTabs.find(tabWidget);
    if (p != m_floatingTabs.end()) {
      QDialog *d = p.value();

      m_floatingTabs.erase(p);

      tabWidget->setParent(nullptr);

      d->setProperty("tab-ptr", QVariant::fromValue<TabWidget *>(nullptr));

      d->close();
      d->deleteLater();
    }
  }

  return true;
}

bool
UIMediator::floatTabWidget(TabWidget *tabWidget)
{
  int index = this->ui->main->mainTab->indexOf(tabWidget);
  auto p = m_floatingTabs.find(tabWidget);

  // Not a TabWidget of ours
  if (index == -1)
    return false;

  // Already floated
  if (p != m_floatingTabs.end())
    return true;

  QDialog *dialog = new QDialog(this->owner);
  QVBoxLayout *layout = new QVBoxLayout;

  tabWidget->floatStart();
  this->closeTabWidget(tabWidget);

  dialog->setProperty(
        "tab-ptr",
        QVariant::fromValue<TabWidget *>(tabWidget));
  dialog->setLayout(layout);
  dialog->setWindowFlags(Qt::Window);

  connect(
        dialog,
        SIGNAL(finished(int)),
        this,
        SLOT(onCloseTabWindow()));

  layout->addWidget(tabWidget);

  tabWidget->floatEnd();
  tabWidget->show();

  dialog->move(this->owner->pos());
  dialog->setWindowTitle(
        "SigDigger - " + QString::fromStdString(tabWidget->getLabel()));

  m_floatingTabs[tabWidget] = dialog;

  dialog->show();

  return true;
}

void
UIMediator::refreshUI()
{
  QString stateString;
  QString sourceDesc;
  bool    enableTimeSlider = false;

  Suscan::Source::Config *config = this->getProfile();
  const Suscan::Source::Device &dev = config->getDevice();

  switch (m_state) {
    case HALTED:
      stateString = QString("Idle");
      this->ui->spectrum->setCaptureMode(MainSpectrum::UNAVAILABLE);
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
      this->haveRtDelta = false;
      this->rtCalibrations = 0;
      this->rtDeltaReal = 0;

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

  if (config->isRemote()) {
    QString user = QString::fromStdString(config->getParam("user"));
    QString host = QString::fromStdString(config->getParam("host"));
    QString port = QString::fromStdString(config->getParam("port"));
    sourceDesc = "Remote analyzer on " + user + "@" + host + ":" + port;

    this->ui->spectrum->setGracePeriod(
          SIGDIGGER_UI_MEDIATOR_REMOTE_GRACE_PERIOD_MS);
  } else {
    if (config->getType() == SUSCAN_SOURCE_TYPE_SDR) {
      sourceDesc = QString::fromStdString(dev.getDesc());
    } else {
      QFileInfo fi = QFileInfo(QString::fromStdString(config->getPath()));
      sourceDesc = fi.fileName();
    }
    this->ui->spectrum->setGracePeriod(
          SIGDIGGER_UI_MEDIATOR_LOCAL_GRACE_PERIOD_MS);
  }

  this->ui->timeSlider->setEnabled(enableTimeSlider);

  this->owner->setWindowTitle(
        "SigDigger - "
        + sourceDesc
        + " - " + stateString);
}

void
UIMediator::connectMainWindow()
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
        this->ui->main->actionQuick_connect,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onQuickConnect()));

  connect(
        this->ui->quickConnectDialog,
        SIGNAL(accepted()),
        this,
        SLOT(onQuickConnectAccepted()));

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
        SLOT(onTabCloseRequested(int)));

  connect(
        this->ui->main->actionPanoramicSpectrum,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerPanoramicSpectrum(bool)));

  connect(
        this->ui->main->actionLogMessages,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerLogMessages()));

  connect(
        this->ui->main->action_Background_tasks,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerBackgroundTasks()));

  connect(
        this->ui->main->actionAddBookmark,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onAddBookmark()));

  connect(
        this->ui->addBookmarkDialog,
        SIGNAL(accepted()),
        this,
        SLOT(onBookmarkAccepted()));

  connect(
        this->ui->main->actionManageBookmarks,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onOpenBookmarkManager()));

  connect(
        this->ui->bookmarkManagerDialog,
        SIGNAL(bookmarkSelected(BookmarkInfo)),
        this,
        SLOT(onJumpToBookmark(BookmarkInfo)));

  connect(
        this->ui->bookmarkManagerDialog,
        SIGNAL(bookmarkChanged()),
        this,
        SLOT(onBookmarkChanged()));

  this->ui->main->mainTab->tabBar()->setContextMenuPolicy(
        Qt::CustomContextMenu);

  connect(
        this->ui->main->mainTab->tabBar(),
        SIGNAL(customContextMenuRequested(const QPoint &)),
        this,
        SLOT(onTabMenuRequested(const QPoint &)));
}

void
UIMediator::initSidePanel()
{
  auto s = Suscan::Singleton::get_instance();

  for (auto p = s->getFirstToolWidgetFactory();
       p != s->getLastToolWidgetFactory();
       ++p) {
    ToolWidgetFactory *f = *p;
    ToolWidget *widget = f->make(this);
    this->ui->spectrum->addToolWidget(
          widget,
          f->getTitle().c_str());
  }
}

void
UIMediator::initUIListeners()
{
  auto s = Suscan::Singleton::get_instance();

  for (auto p = s->getFirstUIListenerFactory();
       p != s->getLastUIListenerFactory();
       ++p) {
    UIListenerFactory *f = *p;
    UIListener *listener = f->make(this);
    this->addUIListener(listener);
  }
}

UIMediator::UIMediator(QMainWindow *owner, AppUI *ui)
{
  this->owner = owner;
  this->ui = ui;

  this->lastPsd.tv_sec = this->lastPsd.tv_usec = 0;

  m_requestTracker = new Suscan::AnalyzerRequestTracker(this);

  this->remoteDevice = Suscan::Source::Device(
            "Remote device",
            "localhost",
            28001,
            "anonymous",
            "");

  this->assertConfig();

  // Now we can create UI components
  this->initSidePanel();

  // Add baseband analyzer tab
  this->ui->main->mainTab->addTab(this->ui->spectrum, "Radio spectrum");

  // Create background task controller dialog
  this->ui->backgroundTasksDialog->setController(
        Suscan::Singleton::get_instance()->getBackgroundTaskController());


  this->connectRequestTracker();
  this->connectMainWindow();
  this->connectSpectrum();

  this->connectDeviceDialog();
  this->connectPanoramicDialog();
  this->connectTimeSlider();
}

void
UIMediator::setBandwidth(unsigned int bw)
{
  this->ui->spectrum->setFilterBandwidth(bw);
}

void
UIMediator::setSampleRate(unsigned int rate)
{
  if (this->rate != rate) {
    unsigned int bw = rate / 30;

    this->ui->spectrum->setSampleRate(rate);
    this->setBandwidth(bw);

    this->rate = rate;
  }
}

void
UIMediator::setState(State state, Suscan::Analyzer *analyzer)
{
  if (m_state != state) {
    // Sanity check
    switch (state) {
      case HALTED:
      case HALTING:
      case RESTARTING:
        assert(analyzer == nullptr);
        break;

      case RUNNING:
        assert(analyzer != nullptr);
    }

    m_state = state;
    m_analyzer = analyzer;

    if (m_analyzer != nullptr)
      this->connectAnalyzer();

    m_requestTracker->setAnalyzer(m_analyzer);

    // Propagate state
    for (auto p : m_components)
      p->setState(state, analyzer);

    this->refreshUI();
  }
}

UIMediator::State
UIMediator::getState() const
{
  return m_state;
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

  this->ui->spectrum->setLocked(
        !info.testPermission(SUSCAN_ANALYZER_PERM_SET_FREQ));

  if (info.isSeekable()) {
    this->setSourceTimeStart(info.getSourceStartTime());
    this->setSourceTimeEnd(info.getSourceEndTime());

    this->ui->timeSlider->setEnabled(
          info.testPermission(
            SUSCAN_ANALYZER_PERM_SEEK));
  }
}

void
UIMediator::notifyTimeStamp(struct timeval const &timestamp)
{
  this->setTimeStamp(timestamp);

  for (auto p : m_components)
    p->setTimeStamp(timestamp);
}

void
UIMediator::refreshDevicesDone()
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
UIMediator::notifyStartupErrors()
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
UIMediator::setAnalyzerParams(Suscan::AnalyzerParams const &params)
{
  this->appConfig->analyzerParams = params;
  this->ui->spectrum->setExpectedRate(
        static_cast<int>(1.f / params.psdUpdateInterval));
}

void
UIMediator::setStatusMessage(QString const &message)
{
  this->ui->main->statusBar->showMessage(message);
}

void
UIMediator::refreshProfile(bool updateFreqs)
{
  qint64 min = 0, max = 0;
  bool isRealTime = false;
  struct timeval tv, start, end;

  std::string user, pass, interface;

  user = this->getProfile()->getParam("user");
  pass = this->getProfile()->getParam("password");
  interface = this->getProfile()->getInterface();

  this->ui->configDialog->setProfile(this->appConfig->profile);

  if (!this->appConfig->profile.isRemote()) {
    if (this->appConfig->profile.getType() == SUSCAN_SOURCE_TYPE_SDR) {
      min = static_cast<qint64>(
            this->appConfig->profile.getDevice().getMinFreq());
      max = static_cast<qint64>(
            this->appConfig->profile.getDevice().getMaxFreq());
      isRealTime = true;
    } else {
      min = SIGDIGGER_MIN_RADIO_FREQ;
      max = SIGDIGGER_MAX_RADIO_FREQ;
    }
  } else {
    // Remote sources receive time from the server
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

  // Configure timeslider
  this->ui->timeSlider->setStartTime(start);
  this->ui->timeSlider->setEndTime(end);
  this->ui->timeSlider->setTimeStamp(tv);
  this->ui->timeToolbar->setVisible(!isRealTime);

  // Configure spectrum
  this->ui->spectrum->setFrequencyLimits(min, max);

  if (updateFreqs) {
    this->ui->spectrum->setFreqs(
          static_cast<qint64>(this->appConfig->profile.getFreq()),
          static_cast<qint64>(this->appConfig->profile.getLnbFreq()));
    this->setSampleRate(this->appConfig->profile.getDecimatedSampleRate());
  }

  // Apply profile to all UI components
  for (auto p : m_components)
    p->setProfile(this->appConfig->profile);
}

Suscan::Source::Config *
UIMediator::getProfile() const
{
  return &this->appConfig->profile;
}

Suscan::AnalyzerParams *
UIMediator::getAnalyzerParams() const
{
  return &this->appConfig->analyzerParams;
}

Suscan::Serializable *
UIMediator::allocConfig()
{
  return this->appConfig = new AppConfig(this->ui);
}

void
UIMediator::saveUIConfig()
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

  for (auto p : m_components)
    this->appConfig->setComponentConfig(
          p->factoryName(),
          p->getSerializedConfig());
}

void
UIMediator::applyConfig()
{
  // Apply window config
  QRect rec = QGuiApplication::primaryScreen()->geometry();
  unsigned int savedBw = this->appConfig->bandwidth;
  int savedLoFreq = this->appConfig->loFreq;
  auto sus = Suscan::Singleton::get_instance();

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

  // Apply color config to all UI components
  for (auto p : m_components)
    p->setColorConfig(this->appConfig->colors);

  // Apply QTH to all UI components
  if (sus->haveQth())
    for (auto p : m_components)
      p->setQth(sus->getQth());

  this->ui->spectrum->setGuiConfig(this->appConfig->guiConfig);

  this->setAnalyzerParams(this->appConfig->analyzerParams);

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
  this->ui->panoramicDialog->applyConfig();

  // Component config in each component is kept in serialized format,
  // so we have to instruct each component to parse it every time
  for (auto p : m_components)
    this->configureUIComponent(p);

  this->refreshProfile();
  this->refreshUI();

  // Apply loFreq and bandwidth config AFTER profile has been set.
  this->ui->spectrum->setLoFreq(savedLoFreq);
  if (savedBw > 0)
    this->setBandwidth(savedBw);
}

UIMediator::~UIMediator()
{
  // Delete UI components in an ordered way
  for (auto p : m_components)
    delete p;
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

    if (this->ui->configDialog->profileChanged())
      this->setProfile(
          this->ui->configDialog->getProfile(),
          this->ui->configDialog->sourceNeedsRestart());

    if (this->ui->configDialog->colorsChanged()) {
      this->appConfig->colors = this->ui->configDialog->getColors();
      this->ui->spectrum->setColorConfig(this->appConfig->colors);

      // Apply color config to all UI components
      for (auto p : m_components)
        p->setColorConfig(this->appConfig->colors);
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

      // Set QTH of all UI components
      for (auto p : m_components)
        p->setQth(loc);
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
UIMediator::onQuickConnect()
{
  this->ui->quickConnectDialog->setProfile(this->appConfig->profile);
  this->ui->quickConnectDialog->exec();
}

void
UIMediator::onQuickConnectAccepted()
{
  this->appConfig->profile.setInterface(SUSCAN_SOURCE_REMOTE_INTERFACE);
  this->appConfig->profile.setDevice(this->remoteDevice);

  this->appConfig->profile.clearParams();

  this->appConfig->profile.setParam(
        "host",
        this->ui->quickConnectDialog->getHost().toStdString());
  this->appConfig->profile.setParam(
        "port",
        std::to_string(this->ui->quickConnectDialog->getPort()));
  this->appConfig->profile.setParam(
        "user",
        this->ui->quickConnectDialog->getUser().toStdString());
  this->appConfig->profile.setParam(
        "password",
        this->ui->quickConnectDialog->getPassword().toStdString());

  this->refreshProfile(false);
  this->refreshUI();
  emit profileChanged(true);

  if (m_state == HALTED)
    emit captureStart();
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
UIMediator::onTriggerBandPlan()
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
UIMediator::onTriggerLogMessages()
{
  this->ui->logDialog->show();
}

void
UIMediator::onTriggerBackgroundTasks()
{
  this->ui->backgroundTasksDialog->show();
}

void
UIMediator::onAddBookmark()
{
  this->ui->addBookmarkDialog->setFrequencyHint(
        this->ui->spectrum->getLoFreq() + this->ui->spectrum->getCenterFreq());

  this->ui->addBookmarkDialog->setNameHint(
        QString::asprintf(
          "Signal @ %s",
          SuWidgetsHelpers::formatQuantity(
            this->ui->spectrum->getLoFreq() + this->ui->spectrum->getCenterFreq(),
            4,
            "Hz").toStdString().c_str()));

  this->ui->addBookmarkDialog->setBandwidthHint(this->ui->spectrum->getBandwidth());

  this->ui->addBookmarkDialog->show();
}

void
UIMediator::onBookmarkAccepted()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  BookmarkInfo info;

  info.name = this->ui->addBookmarkDialog->name();
  info.frequency = this->ui->addBookmarkDialog->frequency();
  info.color = this->ui->addBookmarkDialog->color();
  info.lowFreqCut = this->ui->spectrum->computeLowCutFreq(
        this->ui->addBookmarkDialog->bandwidth());
  info.highFreqCut = this->ui->spectrum->computeHighCutFreq(
        this->ui->addBookmarkDialog->bandwidth());
  info.modulation = this->ui->addBookmarkDialog->modulation();

  if (!sus->registerBookmark(info)) {
    QMessageBox *mb = new QMessageBox(
          QMessageBox::Warning,
          "Cannot create bookmark",
          "A bookmark already exists for frequency "
          + SuWidgetsHelpers::formatQuantity(info.frequency, "Hz. If you wish to "
          "edit this bookmark use the bookmark manager instead."),
          QMessageBox::Ok,
          this->owner);
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->show();
  }

  this->ui->spectrum->updateOverlay();
}

void
UIMediator::onOpenBookmarkManager()
{
  this->ui->bookmarkManagerDialog->show();
}

void
UIMediator::onJumpToBookmark(BookmarkInfo info)
{
  this->ui->spectrum->setCenterFreq(info.frequency);
  this->ui->spectrum->setLoFreq(0);

  if (info.bandwidth() != 0)
    this->setBandwidth(info.bandwidth());

  this->onFrequencyChanged(info.frequency);
}

void
UIMediator::onCloseTabWindow()
{
  QDialog *sender = qobject_cast<QDialog *>(QObject::sender());
  TabWidget *tabWidget;

  tabWidget = sender->property("tab-ptr").value<TabWidget *>();

  // We simply tell the tab widget that someone requested its closure
  tabWidget->closeRequested();
}

void
UIMediator::onTabCloseRequested(int i)
{
  QWidget *widget = this->ui->main->mainTab->widget(i);

  if (widget != nullptr) {
    TabWidget *asTabWidget = SCAST(TabWidget *, widget);
    int index = m_tabWidgets.indexOf(asTabWidget);

    // If it is an opened tab, just notify the closure request
    if (index != -1)
      asTabWidget->closeRequested();
  }
}

void
UIMediator::onTabMenuRequested(const QPoint &point)
{
  int index;

  if (point.isNull())
    return;

  index = this->ui->main->mainTab->tabBar()->tabAt(point);

  QWidget *widget = this->ui->main->mainTab->widget(index);

  if (widget != nullptr) {
    TabWidget *asTabWidget = SCAST(TabWidget *, widget);
    int index = m_tabWidgets.indexOf(asTabWidget);

    // If it is an opened tab, just notify the closure request
    if (index != -1)
      asTabWidget->popupMenu();
  }
}
