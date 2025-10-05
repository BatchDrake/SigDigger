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
#include <sigutils/util/compat-statvfs.h>
#include <SuWidgetsHelpers.h>
#include <cassert>

#include "UIMediator.h"
#include "ProfileConfigTab.h"

#include <QGuiApplication>
#include <QDockWidget>
#include <QMessageBox>
#include <QScreen>
#include <QTimeSlider.h>
#include <FloatingTabWindow.h>

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
#include "GlobalProperty.h"
#include "RemoteControlServer.h"

// Tool widget controls
#include <ToolWidgetFactory.h>
#include <TabWidgetFactory.h>
#include <UIListenerFactory.h>
#include <ToolBarWidgetFactory.h>

#if defined(_WIN32) && defined(interface)
#  undef interface
#endif /* interface */

using namespace SigDigger;

void
UIMediator::addBandPlan(std::string const &name)
{
  QAction *action = new QAction(
        QString::fromStdString(name),
        m_ui->main->menuStart);

  action->setCheckable(true);

  m_ui->main->menuBand_plans->addAction(action);
  m_bandPlanMap[name] = action;

  connect(action, SIGNAL(triggered(bool)), this, SLOT(onTriggerBandPlan()));
}

void
UIMediator::clearRecent()
{
  m_ui->main->menuStart->clear();
  m_recentCount = 0;
}

void
UIMediator::addRecent(std::string const &name)
{
  QAction *action = new QAction(
        QString::fromStdString(name),
        m_ui->main->menuStart);

  m_ui->main->menuStart->addAction(action);
  ++m_recentCount;

  connect(action, SIGNAL(triggered(bool)), this, SLOT(onTriggerRecent(bool)));
}

void
UIMediator::finishRecent()
{
  QAction *action = new QAction("Clear", m_ui->main->menuStart);

  if (m_recentCount == 0) {
    QAction *stub = new QAction("(Empty)", m_ui->main->menuStart);
    stub->setEnabled(false);
    m_ui->main->menuStart->addAction(stub);
  }

  m_ui->main->menuStart->addSeparator();
  m_ui->main->menuStart->addAction(action);

  connect(action, SIGNAL(triggered(bool)), this, SLOT(onTriggerClear(bool)));
}

void
UIMediator::setProfile(Suscan::Source::Config const &prof, bool restart)
{
  m_appConfig->profile = prof;
  refreshProfile();
  refreshUI();
  emit profileChanged(restart);
}

QMainWindow *
UIMediator::getMainWindow() const
{
  return m_owner;
}

MainSpectrum *
UIMediator::getMainSpectrum() const
{
  return m_ui->spectrum;
}

Averager *
UIMediator::getSpectrumAverager()
{
  return &m_averager;
}

AppUI *
UIMediator::getAppUI() const
{
  return m_ui;
}

AppConfig *
UIMediator::getAppConfig() const
{
  return m_appConfig;
}

void
UIMediator::configureUIComponent(UIComponent *comp)
{
  try {
    Suscan::Object config;
    config = m_appConfig->getComponentConfig(comp->factoryName());
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
    m_appConfig->setComponentConfig(
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

  index = m_ui->main->mainTab->insertTab(
        m_ui->main->mainTab->count(),
        tabWidget,
        tabWidget->getCurrentLabel());

  tabWidget->addActionsToParent(tabWidget);
  connect(
        tabWidget,
        SIGNAL(nameChanged(QString)),
        this,
        SLOT(onTabRename(QString)));

  configureUIComponent(tabWidget);

  if (s->haveQth())
    tabWidget->setQth(s->getQth());

  tabWidget->setTimeStamp(m_lastTimeStamp);
  tabWidget->setProfile(m_appConfig->profile);
  tabWidget->setState(m_state, m_analyzer);

  m_ui->main->mainTab->setCurrentIndex(index);

  return true;
}

bool
UIMediator::addUIListener(UIListener *listener)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  if (s->haveQth())
    listener->setQth(s->getQth());

  listener->setTimeStamp(m_lastTimeStamp);
  listener->setProfile(m_appConfig->profile);
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

  index = m_ui->main->mainTab->indexOf(tabWidget);
  if (index != -1) {
    m_ui->main->mainTab->removeTab(index);
    tabWidget->setParent(nullptr);
  } else {
    auto p = m_floatingTabs.find(tabWidget);
    if (p != m_floatingTabs.end()) {
      FloatingTabWindow *d = p.value();

      m_floatingTabs.erase(p);

      tabWidget->setParent(nullptr);

      d->close();
      d->deleteLater();
    }
  }

  return true;
}

bool
UIMediator::floatTabWidget(TabWidget *tabWidget)
{
  int index = m_ui->main->mainTab->indexOf(tabWidget);
  auto p = m_floatingTabs.find(tabWidget);

  // Not a TabWidget of ours
  if (index == -1)
    return false;

  // Already floated
  if (p != m_floatingTabs.end())
    return true;

  tabWidget->floatStart();
  closeTabWidget(tabWidget);

  FloatingTabWindow *window = new FloatingTabWindow(tabWidget, m_owner);

  connect(
        window,
        SIGNAL(finished()),
        this,
        SLOT(onCloseTabWindow()));

  connect(
        window,
        SIGNAL(reattach()),
        this,
        SLOT(onReattachTabWindow()));

  tabWidget->floatEnd();
  tabWidget->show();

  window->move(m_owner->pos());

  m_floatingTabs[tabWidget] = window;

  window->show();

  return true;
}

bool
UIMediator::unFloatTabWidget(TabWidget *tabWidget)
{
  int index = m_ui->main->mainTab->indexOf(tabWidget);
  auto p = m_floatingTabs.find(tabWidget);

  // Already a tab
  if (index != -1)
    return true;

  // Not a float of ours
  if (p == m_floatingTabs.end())
    return false;

  tabWidget->floatStart();

  FloatingTabWindow *d = p.value();

  m_floatingTabs.erase(p);

  tabWidget = d->takeTabWidget();

  d->close();
  d->hide();
  d->deleteLater();

  m_tabWidgets.push_back(tabWidget);

  index = m_ui->main->mainTab->insertTab(
        m_ui->main->mainTab->count(),
        tabWidget,
        tabWidget->getCurrentLabel());

  tabWidget->floatEnd();
  tabWidget->show();

  m_ui->main->mainTab->setCurrentIndex(index);

  return true;
}

void
UIMediator::registerComponentActions(UIComponent *comp)
{
  auto &actions = comp->actions();

  if (!actions.isEmpty()) {
    QToolBar *toolBar = new QToolBar(comp->factory()->desc());

    getMainWindow()->insertToolBar(m_lastToolBar, toolBar);
    m_lastToolBar = toolBar;

    toolBar->setObjectName(comp->factory()->name() + QString("::Actions"));
    toolBar->setIconSize(QSize(24, 24));
    toolBar->setFloatable(true);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    for (auto &p : actions)
      toolBar->addAction(p);
  }
}

void
UIMediator::setUIBusy(bool busy)
{
  m_owner->setCursor(busy ? Qt::WaitCursor : Qt::ArrowCursor);
}

//
// The toolbar is visible only under the following conditions:
// If RUNNING:
//   if the analyzer has exposed a SEEK permission
// If NOT RUNNING:
//    if the source is local and it is not realtime
//
// Additionally, the toolbar is only enabled in RUNNING state
//

void
UIMediator::refreshTimeToolbarState()
{
  bool haveToolbar = false;

  if (m_state == RUNNING) {
    haveToolbar =
        m_analyzer->getSourceInfo().testPermission(SUSCAN_ANALYZER_PERM_SEEK);
  } else {
    Suscan::Source::Config *config = getProfile();
    haveToolbar = config->getDeviceSpec().analyzer() != "remote"
        && config->isSeekable();
  }

  m_ui->timeToolbar->setVisible(haveToolbar);
  m_ui->timeSlider->setEnabled(m_state == RUNNING);
}

void
UIMediator::refreshUI()
{
  QString stateString;
  QString sourceDesc;
  bool runButtonPressed = false;

  Suscan::Source::Config *config = getProfile();
  auto spec = config->getDeviceSpec();
  auto prop = spec.properties();

  switch (m_state) {
    case INVALID:
      break;

    case HALTED:
      stateString = QString("Idle");
      m_ui->spectrum->setCaptureMode(MainSpectrum::UNAVAILABLE);
      m_ui->main->actionRun->setEnabled(true);
      m_ui->main->actionRun->setChecked(false);
      m_ui->main->actionStart_capture->setEnabled(true);
      m_ui->main->actionStop_capture->setEnabled(false);
      m_ui->panoramicDialog->setBannedDevice("");
      m_ui->spectrum->notifyHalt();
      break;

    case HALTING:
      stateString = QString("Halting...");
      m_ui->main->actionRun->setEnabled(false);
      m_ui->main->actionStart_capture->setEnabled(false);
      m_ui->main->actionStop_capture->setEnabled(false);
      break;

    case RUNNING:
      m_haveRtDelta = false;
      m_rtCalibrations = 0;
      m_rtDeltaReal = 0;
      runButtonPressed = true;

      stateString = QString("Running");

      if (m_appConfig->profile.isRealTime())
        m_ui->spectrum->setCaptureMode(MainSpectrum::CAPTURE);
      else
        m_ui->spectrum->setCaptureMode(MainSpectrum::REPLAY);

      m_ui->main->actionRun->setEnabled(true);
      m_ui->main->actionRun->setChecked(true);
      m_ui->main->actionStart_capture->setEnabled(false);
      m_ui->main->actionStop_capture->setEnabled(true);
      m_ui->panoramicDialog->setBannedDevice(QString::fromStdString(spec.uri()));
      break;

    case RESTARTING:
      stateString = QString("Restarting...");
      runButtonPressed = true;
      m_ui->main->actionRun->setEnabled(false);
      m_ui->main->actionStart_capture->setEnabled(false);
      m_ui->main->actionStop_capture->setEnabled(false);
      break;
  }

  if (spec.analyzer() == "remote") {
    QString user = QString::fromStdString(config->getParam("user"));
    QString host = QString::fromStdString(config->getParam("host"));
    QString port = QString::fromStdString(config->getParam("port"));
    sourceDesc = "Remote analyzer on " + user + "@" + host + ":" + port;

    m_ui->spectrum->setGracePeriod(
          SIGDIGGER_UI_MEDIATOR_REMOTE_GRACE_PERIOD_MS);
  } else {
    if (config->getType() == "file") {
      QFileInfo fi = QFileInfo(QString::fromStdString(config->getPath()));
      sourceDesc = fi.fileName();
    } else {
      sourceDesc = QString::fromStdString(
            prop == nullptr
            ? spec.uri()
            : prop->label());
    }

    m_ui->spectrum->setGracePeriod(
          SIGDIGGER_UI_MEDIATOR_LOCAL_GRACE_PERIOD_MS);
  }

  // Time toolbar is visible always, only if a file is selected
  refreshTimeToolbarState();

  m_owner->setWindowTitle(
        "SigDigger - "
        + sourceDesc
        + " - " + stateString);

  m_ui->spectrum->setFreqs(
        static_cast<qint64>(m_appConfig->profile.getFreq()),
        static_cast<qint64>(m_appConfig->profile.getLnbFreq()));
  setSampleRate(m_appConfig->profile.getDecimatedSampleRate());

  BLOCKSIG(m_ui->main->actionRun, setChecked(runButtonPressed));
}

void
UIMediator::connectMainWindow()
{
  connect(
        m_ui->main->actionReplayFile,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerReplayFile(bool)));

  connect(
        m_ui->main->actionSetup,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerSetup(bool)));

  connect(
        m_ui->main->actionOptions,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerSetup(bool)));

  connect(
        m_ui->main->actionImport_profile,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerImport(bool)));

  connect(
        m_ui->main->actionExport_profile,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerExport(bool)));

  connect(
        m_ui->main->actionDevices,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerDevices(bool)));

  connect(
        m_ui->main->actionQuick_connect,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onQuickConnect()));

  connect(
        m_ui->quickConnectDialog,
        SIGNAL(accepted()),
        this,
        SLOT(onQuickConnectAccepted()));

  connect(
        m_ui->main->actionStart_capture,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerStart(bool)));

  connect(
        m_ui->main->actionStop_capture,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerStop(bool)));

  connect(
        m_ui->main->actionQuit,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerQuit(bool)));

  connect(
        m_ui->main->actionRun,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onToggleCapture(bool)));

#ifndef __APPLE__
  connect(
        m_ui->main->action_Full_screen,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onToggleFullScreen(bool)));
#endif // __APLE__

  connect(
        m_ui->main->actionAbout,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onToggleAbout(bool)));

  connect(
        m_ui->main->mainTab,
        SIGNAL(tabCloseRequested(int)),
        this,
        SLOT(onTabCloseRequested(int)));

  connect(
        m_ui->main->actionPanoramicSpectrum,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerPanoramicSpectrum(bool)));

  connect(
        m_ui->main->actionLogMessages,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerLogMessages()));

  connect(
        m_ui->main->action_Background_tasks,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerBackgroundTasks()));

  connect(
        m_ui->main->actionAddBookmark,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onAddBookmark()));

  connect(
        m_ui->addBookmarkDialog,
        SIGNAL(accepted()),
        this,
        SLOT(onBookmarkAccepted()));

  connect(
        m_ui->main->actionManageBookmarks,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onOpenBookmarkManager()));

  connect(
        m_ui->bookmarkManagerDialog,
        SIGNAL(bookmarkSelected(BookmarkInfo)),
        this,
        SLOT(onJumpToBookmark(BookmarkInfo)));

  connect(
        m_ui->bookmarkManagerDialog,
        SIGNAL(bookmarkChanged()),
        this,
        SLOT(onBookmarkChanged()));

  m_ui->main->mainTab->tabBar()->setContextMenuPolicy(
        Qt::CustomContextMenu);

  connect(
        m_ui->main->mainTab->tabBar(),
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
    m_ui->spectrum->addToolWidget(
          widget,
          f->getTitle().c_str());

    registerComponentActions(widget);
  }
}

void
UIMediator::initToolBarWidgets()
{
  auto s = Suscan::Singleton::get_instance();

  for (auto p = s->getFirstToolBarWidgetFactory();
       p != s->getLastToolBarWidgetFactory();
       ++p) {
    ToolBarWidgetFactory *f = *p;
    ToolBarWidget *widget = f->make(this);

    m_ui->addToolBarWidget(widget);
    registerComponentActions(widget);
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
    addUIListener(listener);
  }
}

UIMediator::UIMediator(QMainWindow *owner, AppUI *ui)
{
  m_owner = owner;
  m_ui = ui;

  m_lastPsd.tv_sec = m_lastPsd.tv_usec = 0;

  m_requestTracker = new Suscan::AnalyzerRequestTracker(this);
  m_lastToolBar    = m_ui->main->helpToolBar;

  assertConfig();

  // Add baseband analyzer tab
  m_ui->main->mainTab->addTab(m_ui->spectrum, "Radio spectrum");

  // Create background task controller dialog
  m_ui->backgroundTasksDialog->setController(
        Suscan::Singleton::get_instance()->getBackgroundTaskController());


  connectRequestTracker();
  connectMainWindow();
  connectSpectrum();

  connectDeviceDialog();
  connectPanoramicDialog();
  connectTimeSlider();

  m_propFrequency = GlobalProperty::registerProperty("frequency", "Spectrum frequency", 0);
  m_propLNB       = GlobalProperty::registerProperty("lnb", "LNB frequency", 0);
  m_propSampRate  = GlobalProperty::registerProperty("samp_rate", "Sample rate", "N/A");
  m_propFftSize   = GlobalProperty::registerProperty("fft_size", "Size of the FFT", 0);
  m_propRBW       = GlobalProperty::registerProperty("rbw", "Resolution bandwidth", "N/A");
  m_propDate      = GlobalProperty::registerProperty("date", "Source date (UTC)", "N/A");
  m_propTime      = GlobalProperty::registerProperty("time", "Source time (UTC)", "N/A");
  m_propDateTime  = GlobalProperty::registerProperty("datetime", "Source date and time (UTC)", "N/A");
  m_propCity      = GlobalProperty::registerProperty("city", "City", "None");
  m_propLat       = GlobalProperty::registerProperty("lat", "Receiver latitude", 0.0);
  m_propLon       = GlobalProperty::registerProperty("lon", "Receiver longitude", 0.0);
  m_propLocator   = GlobalProperty::registerProperty("locator", "Grid locator", "");

  m_propFrequency->setAdjustable(true);
  connect(
        m_propFrequency,
        SIGNAL(changed()),
        this,
        SLOT(onPropFrequencyChanged()));

  m_propLNB->setAdjustable(true);
  connect(
        m_propLNB,
        SIGNAL(changed()),
        this,
        SLOT(onPropLNBChanged()));

  m_remoteControl = new RemoteControlServer(this);
  m_remoteControl->setEnabled(true);

}

void
UIMediator::setBandwidth(unsigned int bw)
{
  m_ui->spectrum->setFilterBandwidth(bw);
}

void
UIMediator::setSampleRate(unsigned int rate)
{
  if (m_rate != rate) {
    unsigned int bw = rate / 30;

    m_propSampRate->setValue<QString>(
          rate > 0
          ? QString::number(rate)
          : "N/A");

    m_ui->spectrum->setSampleRate(rate);
    setBandwidth(bw);

    m_rate = rate;
  }
}

void
UIMediator::setState(State state, Suscan::Analyzer *analyzer)
{
  if (m_state != state) {
    // Reset to previous state
    setUIBusy(false);

    // Sanity check
    switch (state) {
      case INVALID:
        QMessageBox::critical(
              this,
              "Internal error",
              "A component requested the UI to enter an invalid state. "
              "This is most certaintly a bug from a plugin. Check if new plugins "
              "have been installed and contact the developer.\n\n"
              "SigDigger cannot run in this state and will be closed."
              );
        abort();

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
      connectAnalyzer();

    m_requestTracker->setAnalyzer(m_analyzer);

    // Propagate state
    for (auto p : m_components)
      p->setState(state, analyzer);
  }

  refreshUI();
}

UIMediator::State
UIMediator::getState() const
{
  return m_state;
}

void
UIMediator::notifySourceInfo(Suscan::AnalyzerSourceInfo const &info)
{
  if (!info.isSeekable()) {
    if (info.replayMode())
      m_ui->spectrum->setCaptureMode(MainSpectrum::HISTORY);
    else
      m_ui->spectrum->setCaptureMode(MainSpectrum::CAPTURE);
  } else {
    m_ui->spectrum->setCaptureMode(MainSpectrum::REPLAY);
  }

  m_ui->spectrum->setFrequencyLimits(
        static_cast<qint64>(info.getMinFrequency()),
        static_cast<qint64>(info.getMaxFrequency()));

  m_ui->spectrum->setDisplayFreqs(
        static_cast<qint64>(info.getFrequency()),
        static_cast<qint64>(info.getLnbFrequency()));

  m_ui->spectrum->setLocked(
        !info.testPermission(SUSCAN_ANALYZER_PERM_SET_FREQ));

  if (info.testPermission(SUSCAN_ANALYZER_PERM_SEEK)) {
    setSourceTimeStart(info.getSourceStartTime());
    setSourceTimeEnd(info.getSourceEndTime());
  }

  refreshTimeToolbarState();
  setSampleRate(static_cast<unsigned>(info.getSampleRate()));
}

void
UIMediator::notifyTimeStamp(struct timeval const &timestamp)
{
  QDateTime dateTime;
  dateTime.setSecsSinceEpoch(timestamp.tv_sec);
  dateTime = dateTime.toUTC();
  QString strDate, strTime;

  strDate = dateTime.toString("yyyy-MM-dd");
  strTime = dateTime.toString("hh:mm:ss");
  m_propDate->setValue(strDate);
  m_propTime->setValue(strTime);
  m_propDateTime->setValue(strDate + "T" + strTime + "Z");

  setTimeStamp(timestamp);

  for (auto p : m_components)
    p->setTimeStamp(timestamp);

  // TODO: These things should be a component at some point
  m_ui->spectrum->setTimeStamp(timestamp);
}

void
UIMediator::refreshDevicesDone()
{
  m_ui->deviceDialog->refreshDone();
  m_ui->configDialog->notifySingletonChanges();
  refreshUI();
}

QMessageBox::StandardButton
UIMediator::shouldReduceRate(
    QString const &label,
    unsigned int requested,
    unsigned int proposed)
{
  QMessageBox::StandardButton reply = QMessageBox::No;

  if (!m_appConfig->disableHighRateWarning) {
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
              m_appConfig->disableHighRateWarning = true;
          }
      });

      reply = static_cast<QMessageBox::StandardButton>(msgbox.exec());
  }

  return reply;
}

void
UIMediator::notifyStartupErrors()
{
  if (m_ui->logDialog->haveErrorMessages()) {
      QMessageBox msgbox(m_owner);
      msgbox.setTextFormat(Qt::RichText);
      msgbox.setText(
            "Errors occurred during startup:" +
            m_ui->logDialog->getErrorHtml());
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
  m_appConfig->analyzerParams = params;
  m_ui->spectrum->setExpectedRate(
        static_cast<int>(1.f / params.psdUpdateInterval));
}

void
UIMediator::setStatusMessage(QString const &message)
{
  m_ui->main->statusBar->showMessage(message);
}

void
UIMediator::refreshProfile(bool updateFreqs)
{
  qint64 min = SIGDIGGER_MIN_RADIO_FREQ, max = SIGDIGGER_MAX_RADIO_FREQ;
  bool isRealTime = false;
  struct timeval tv, start, end;

  std::string user, pass, interface;

  user = getProfile()->getParam("user");
  pass = getProfile()->getParam("password");
  interface = getProfile()->getDeviceSpec().analyzer();

  m_ui->configDialog->setProfile(m_appConfig->profile);

  // Local sources, we may know the limits beforehand
  if (interface != "remote") {
    SUFREQ fMin, fMax;
    isRealTime = m_appConfig->profile.isRealTime();
    if (m_appConfig->profile.getFreqLimits(fMin, fMax)) {
      min = SCAST(qint64, fMin);
      max = SCAST(qint64, fMax);
    }
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
    if (m_appConfig->profile.fileIsValid()) {
      start = m_appConfig->profile.getStartTime();
      end   = m_appConfig->profile.getEndTime();
    } else {
      start = m_appConfig->profile.getStartTime();
      end   = start;
      end.tv_sec += 1;
    }
    tv = start;
  }

  // Set cached members
  m_profileStart = start;
  m_profileEnd   = end;

  // Configure timeslider
  m_ui->timeSlider->setStartTime(start);
  m_ui->timeSlider->setEndTime(end);
  m_ui->timeSlider->setTimeStamp(tv);
  refreshTimeToolbarState();

  // Configure spectrum
  m_ui->spectrum->setFrequencyLimits(min, max);

  if (updateFreqs) {
    m_ui->spectrum->setFreqs(
          static_cast<qint64>(m_appConfig->profile.getFreq()),
          static_cast<qint64>(m_appConfig->profile.getLnbFreq()));
    setSampleRate(m_appConfig->profile.getDecimatedSampleRate());
  }

  // Apply profile to all UI components
  for (auto p : m_components)
    p->setProfile(m_appConfig->profile);
}

Suscan::Source::Config *
UIMediator::getProfile() const
{
  return &m_appConfig->profile;
}

Suscan::AnalyzerParams *
UIMediator::getAnalyzerParams() const
{
  return &m_appConfig->analyzerParams;
}

Suscan::Serializable *
UIMediator::allocConfig()
{
  return m_appConfig = new AppConfig(m_ui);
}

void
UIMediator::saveUIConfig()
{
  m_appConfig->x = m_owner->geometry().x();
  m_appConfig->y = m_owner->geometry().y();
  m_appConfig->width  = m_owner->geometry().width();
  m_appConfig->height = m_owner->geometry().height();
  m_appConfig->sidePanelRatio = m_ui->spectrum->sidePanelRatio();
  m_appConfig->mainWindowState = m_owner->saveState();
  m_appConfig->enabledBandPlans.clear();

  for (auto p : m_bandPlanMap)
    if (p.second->isChecked())
      m_appConfig->enabledBandPlans.push_back(p.first);

  for (auto p : m_components)
    m_appConfig->setComponentConfig(
          p->factoryName(),
          p->getSerializedConfig());
}

void
UIMediator::refreshQthProperties()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  QString cachedCity, cachedLat, cachedLon, cachedLocator;

  if (sus->haveQth()) {
    auto loc = sus->getQth();
    xyz_t qth = loc.getQth();
    qreal lat = SCAST(qreal, SU_RAD2DEG(qth.lat));
    qreal lon = SCAST(qreal, SU_RAD2DEG(qth.lon));

    cachedCity    = loc.getLocationName();
    cachedLat     = SuWidgetsHelpers::formatQuantity(lat, 0, "deg", true);
    cachedLon     = SuWidgetsHelpers::formatQuantity(lon, 0, "deg", true);
    cachedLocator = loc.getGridLocator();
  } else {
    cachedCity    = "(no city defined)";
    cachedLat     = "(no latitude defined)";
    cachedLon     = "(no longitude defined)";
    cachedLocator = "(no locator defined)";
  }

  m_propCity->setValue(cachedCity);
  m_propLat->setValue(cachedLat);
  m_propLon->setValue(cachedLon);
  m_propLocator->setValue(cachedLocator);
}

void
UIMediator::applyConfig()
{
  // Check if all plugin widgets have been loaded.
  if (!m_uiInitialized) {
    initSidePanel();
    initToolBarWidgets();
    m_uiInitialized = true;
  }

  // Apply window config
  QRect rec = QGuiApplication::primaryScreen()->geometry();
  unsigned int savedBw = m_appConfig->bandwidth;
  int savedLoFreq = m_appConfig->loFreq;
  auto sus = Suscan::Singleton::get_instance();

  if (m_appConfig->x == -1)
    m_appConfig->x = (rec.width() - m_appConfig->width) / 2;
  if (m_appConfig->y == -1)
    m_appConfig->y = (rec.height() - m_appConfig->height) / 2;

  m_owner->setGeometry(
        m_appConfig->x,
        m_appConfig->y,
        m_appConfig->width,
        m_appConfig->height);

  if (m_appConfig->fullScreen)
    m_owner->setWindowState(
        m_owner->windowState() | Qt::WindowFullScreen);

  m_owner->restoreState(m_appConfig->mainWindowState);

  if (m_appConfig->sidePanelRatio >= 0)
    m_ui->spectrum->setSidePanelRatio(m_appConfig->sidePanelRatio);

  // The following controls reflect elements of the configuration that are
  // not owned by them. We need to set them manually.
  m_ui->configDialog->setColors(m_appConfig->colors);
  m_ui->configDialog->setGuiConfig(m_appConfig->guiConfig);
  m_ui->configDialog->setTleSourceConfig(m_appConfig->tleSourceConfig);
  m_ui->configDialog->setAudioConfig(m_appConfig->audioConfig);
  m_ui->configDialog->setRemoteControlConfig(m_appConfig->rcConfig);
  m_ui->panoramicDialog->setGuiConfig(m_appConfig->guiConfig);
  m_ui->panoramicDialog->setColors(m_appConfig->colors);
  m_ui->spectrum->setColorConfig(m_appConfig->colors);

  // Apply color config to all UI components
  for (auto p : m_components)
    p->setColorConfig(m_appConfig->colors);

  // Apply QTH to all UI components
  if (sus->haveQth())
    for (auto p : m_components)
      p->setQth(sus->getQth());

  refreshQthProperties();
  m_ui->spectrum->setGuiConfig(m_appConfig->guiConfig);

  setAnalyzerParams(m_appConfig->analyzerParams);

  // Apply enabled bandplans
  for (auto p : m_appConfig->enabledBandPlans)
    if (m_bandPlanMap.find(p) != m_bandPlanMap.cend()) {
      FrequencyAllocationTable *table =
          m_ui->spectrum->getFAT(QString::fromStdString(p));

      if (table != nullptr) {
        m_bandPlanMap[p]->setChecked(true);
        m_ui->spectrum->pushFAT(table);
      }
    }

  // The rest of them are automatically deserialized
  m_ui->panoramicDialog->applyConfig();

  // Component config in each component is kept in serialized format,
  // so we have to instruct each component to parse it every time
  for (auto p : m_components)
    configureUIComponent(p);

  refreshProfile();
  refreshUI();

  // Apply loFreq and bandwidth config AFTER profile has been set.
  m_ui->spectrum->setLoFreq(savedLoFreq);
  if (savedBw > 0)
    setBandwidth(savedBw);

  // Enable remote control
  m_remoteControl->setHost(QString::fromStdString(m_appConfig->rcConfig.host));
  m_remoteControl->setPort(SCAST(uint16_t, m_appConfig->rcConfig.port));
  m_remoteControl->setEnabled(m_appConfig->rcConfig.enabled);
}

bool
UIMediator::openSettingsDialog(Suscan::Source::Config *prof)
{
  auto sus = Suscan::Singleton::get_instance();
  bool accepted = false;

  // Make sure configDialog is up to date
  m_ui->configDialog->setProfile(*prof);
  m_ui->configDialog->setAnalyzerParams(*getAnalyzerParams());
  m_ui->configDialog->setColors(m_appConfig->colors);
  m_ui->configDialog->setTleSourceConfig(m_appConfig->tleSourceConfig);
  m_ui->configDialog->setGuiConfig(m_appConfig->guiConfig);
  m_ui->configDialog->setAudioConfig(m_appConfig->audioConfig);

  if (sus->haveQth())
    m_ui->configDialog->setLocation(sus->getQth());

  // Run config dialog

  accepted = m_ui->configDialog->run();

  if (!accepted)
    return false;

  m_appConfig->analyzerParams = m_ui->configDialog->getAnalyzerParams();

  if (m_ui->configDialog->profileChanged())
    setProfile(
          m_ui->configDialog->getProfile(),
          m_ui->configDialog->sourceNeedsRestart());

  if (m_ui->configDialog->colorsChanged()) {
    m_appConfig->colors = m_ui->configDialog->getColors();
    m_ui->spectrum->setColorConfig(m_appConfig->colors);

    // Apply color config to all UI components
    for (auto p : m_components)
      p->setColorConfig(m_appConfig->colors);
  }

  if (m_ui->configDialog->guiChanged()) {
    m_appConfig->guiConfig = m_ui->configDialog->getGuiConfig();
    m_ui->spectrum->setGuiConfig(m_appConfig->guiConfig);
    m_ui->panoramicDialog->setGuiConfig(m_appConfig->guiConfig);
  }

  if (m_ui->configDialog->audioChanged()) {
    m_appConfig->audioConfig = m_ui->configDialog->getAudioConfig();
  }

  if (m_ui->configDialog->remoteControlChanged()) {
    m_appConfig->rcConfig = m_ui->configDialog->getRemoteControlConfig();

    m_remoteControl->setHost(QString::fromStdString(m_appConfig->rcConfig.host));
    m_remoteControl->setPort(SCAST(uint16_t, m_appConfig->rcConfig.port));

    if (m_remoteControl->isEnabled())
      m_remoteControl->setEnabled(false);

    m_remoteControl->setEnabled(m_appConfig->rcConfig.enabled);
  }

  if (m_ui->configDialog->tleSourceConfigChanged()) {
    m_appConfig->tleSourceConfig =
        m_ui->configDialog->getTleSourceConfig();
  }

  if (m_ui->configDialog->locationChanged()) {
    Suscan::Location loc = m_ui->configDialog->getLocation();
    sus->setQth(loc);

    refreshQthProperties();

    // This triggers the update of the infotext
    m_ui->spectrum->setGuiConfig(m_appConfig->guiConfig);

    // Set QTH of all UI components
    for (auto p : m_components)
      p->setQth(loc);
  }
  return true;
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
  openSettingsDialog(getProfile());
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
  m_owner->setWindowState(m_owner->windowState() ^ Qt::WindowFullScreen);
  m_appConfig->fullScreen =
      (m_owner->windowState() & Qt::WindowFullScreen) != 0;
}

void
UIMediator::onToggleAbout(bool)
{
  m_ui->aboutDialog->exec();
}

void
UIMediator::onQuickConnect()
{
  m_ui->quickConnectDialog->setProfile(m_appConfig->profile);
  m_ui->quickConnectDialog->exec();
}

void
UIMediator::onQuickConnectAccepted()
{
  Suscan::DeviceSpec spec;
  std::map<std::string, std::string> traits;

  auto host = m_ui->quickConnectDialog->getHost().toStdString();
  auto port = std::to_string(m_ui->quickConnectDialog->getPort());
  auto user = m_ui->quickConnectDialog->getUser().toStdString();
  auto pass = m_ui->quickConnectDialog->getPassword().toStdString();

  traits["host"] = host;
  traits["port"] = port;

  spec.setAnalyzer("remote");
  spec.setSource(traits["host"] + ":" + traits["port"]);
  spec.setTraits(traits);
  spec.set("user", user);
  spec.set("password", pass);

  m_appConfig->profile.setDeviceSpec(spec);

  refreshProfile(false);
  refreshUI();
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
            m_ui->main->centralWidget,
            "Cannot open file",
            "Selected profile file could not be opened. Please check its "
            "access rights or whether the file still exists and try again.",
            QMessageBox::Ok);
    } else if (in.tellg() > SIGDIGGER_PROFILE_FILE_MAX_SIZE) {
        QMessageBox::critical(
              m_ui->main->centralWidget,
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
              m_ui->main->centralWidget,
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
                  m_ui->main->centralWidget,
                  "Cannot open file",
                  "Invalid file: serialized object class is undefined",
                  QMessageBox::Ok);
          } else if (className != "source_config") {
            QMessageBox::warning(
                  m_ui->main->centralWidget,
                  "Cannot open file",
                  "Invalid file: objects of class `" + className + "' are "
                  "not profiles.",
                  QMessageBox::Ok);
          } else {
            try {
              Suscan::Source::Config config(profObj);
              setProfile(config);
            } catch (Suscan::Exception &e) {
              QMessageBox::critical(
                    m_ui->main->centralWidget,
                    "Cannot open file",
                    "Failed to create source configuration from profile object: "
                    + QString(e.what()),
                    QMessageBox::Ok);
            }
          }
        } catch (Suscan::Exception &e) {
          QMessageBox::critical(
                m_ui->main->centralWidget,
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
        Suscan::Object profileObj = getProfile()->serialize();
        Suscan::Object envelope(SUSCAN_OBJECT_TYPE_SET);

        envelope.append(profileObj);

        std::vector<char> data = envelope.serialize();
        std::ofstream of(path.toStdString().c_str(), std::ofstream::binary);

        if (!of.is_open()) {
          QMessageBox::warning(
                m_ui->main->centralWidget,
                "Cannot open file",
                "Cannote save file in the specified location. Please choose "
                "a different location and try again.",
                QMessageBox::Ok);
        } else {
          of.write(&data[0], static_cast<int>(data.size()));

          if (of.fail()) {
            QMessageBox::critical(
                  m_ui->main->centralWidget,
                  "Cannot serialize profile",
                  "Write error. Profile has not been saved. Please try again.",
                  QMessageBox::Ok);
          } else {
            done = true;
          }
        }
      } catch (Suscan::Exception &e) {
        QMessageBox::critical(
              m_ui->main->centralWidget,
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
  m_ui->deviceDialog->run();
}

void
UIMediator::onTriggerClear(bool)
{
  clearRecent();
  finishRecent();

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
  m_ui->panoramicDialog->run();
}

void
UIMediator::onTriggerBandPlan()
{
  QObject* obj = sender();
  QAction *asAction = qobject_cast<QAction *>(obj);

  if (asAction->isChecked()) {
    FrequencyAllocationTable *fat = m_ui->spectrum->getFAT(asAction->text());
    if (fat != nullptr)
      m_ui->spectrum->pushFAT(fat);
  } else {
    m_ui->spectrum->removeFAT(asAction->text());
  }
}

void
UIMediator::onTriggerLogMessages()
{
  m_ui->logDialog->show();
}

void
UIMediator::onTriggerBackgroundTasks()
{
  m_ui->backgroundTasksDialog->show();
}

void
UIMediator::onAddBookmark()
{
  m_ui->addBookmarkDialog->setFrequencyHint(
        m_ui->spectrum->getLoFreq() + m_ui->spectrum->getCenterFreq());

  m_ui->addBookmarkDialog->setNameHint(
        QString::asprintf(
          "Signal @ %s",
          SuWidgetsHelpers::formatQuantity(
            m_ui->spectrum->getLoFreq() + m_ui->spectrum->getCenterFreq(),
            4,
            "Hz").toStdString().c_str()));

  m_ui->addBookmarkDialog->setBandwidthHint(m_ui->spectrum->getBandwidth());

  m_ui->addBookmarkDialog->show();
}

void
UIMediator::onBookmarkAccepted()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  BookmarkInfo info;

  info.name = m_ui->addBookmarkDialog->name();
  info.frequency = m_ui->addBookmarkDialog->frequency();
  info.color = m_ui->addBookmarkDialog->color();
  info.lowFreqCut = m_ui->spectrum->computeLowCutFreq(
        m_ui->addBookmarkDialog->bandwidth());
  info.highFreqCut = m_ui->spectrum->computeHighCutFreq(
        m_ui->addBookmarkDialog->bandwidth());
  info.modulation = m_ui->addBookmarkDialog->modulation();

  if (!sus->registerBookmark(info)) {
    QMessageBox *mb = new QMessageBox(
          QMessageBox::Warning,
          "Cannot create bookmark",
          "A bookmark already exists for frequency "
          + SuWidgetsHelpers::formatQuantity(info.frequency, "Hz. If you wish to "
          "edit this bookmark use the bookmark manager instead."),
          QMessageBox::Ok,
          m_owner);
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->show();
  }

  m_ui->spectrum->updateOverlay();

  emit triggerSaveConfig();
}

void
UIMediator::onOpenBookmarkManager()
{
  m_ui->bookmarkManagerDialog->show();
}

void
UIMediator::onJumpToBookmark(BookmarkInfo info)
{
  m_ui->spectrum->setCenterFreq(info.frequency);
  m_ui->spectrum->setLoFreq(0);

  if (info.bandwidth() != 0)
    setBandwidth(info.bandwidth());

  onFrequencyChanged(info.frequency);
}

void
UIMediator::onCloseTabWindow()
{
  FloatingTabWindow *sender = qobject_cast<FloatingTabWindow *>(QObject::sender());
  TabWidget *tabWidget;

  tabWidget = sender->getTabWidget();

  // We simply tell the tab widget that someone requested its closure
  if (tabWidget != nullptr)
    tabWidget->closeRequested();
}

void
UIMediator::onReattachTabWindow()
{
  FloatingTabWindow *sender = qobject_cast<FloatingTabWindow *>(QObject::sender());
  TabWidget *tabWidget;

  tabWidget = sender->getTabWidget();

  unFloatTabWidget(tabWidget);
}

void
UIMediator::onTabCloseRequested(int i)
{
  QWidget *widget = m_ui->main->mainTab->widget(i);

  if (widget != nullptr) {
    TabWidget *asTabWidget = SCAST(TabWidget *, widget);
    int index = m_tabWidgets.indexOf(asTabWidget);

    // If it is an opened tab, just notify the closure request
    if (index != -1)
      asTabWidget->closeRequested();
  }
}

void
UIMediator::onTabRename(QString newName)
{
  QWidget *sender = qobject_cast<QWidget *>(QObject::sender());
  int index;

  index = m_ui->main->mainTab->indexOf(sender);

  if (index != -1)
    m_ui->main->mainTab->setTabText(index, newName);
}

void
UIMediator::onTabMenuRequested(const QPoint &point)
{
  int index;

  if (point.isNull())
    return;

  index = m_ui->main->mainTab->tabBar()->tabAt(point);

  QWidget *widget = m_ui->main->mainTab->widget(index);

  if (widget != nullptr) {
    TabWidget *asTabWidget = SCAST(TabWidget *, widget);
    int index = m_tabWidgets.indexOf(asTabWidget);

    // If it is an opened tab, just notify the closure request
    if (index != -1)
      asTabWidget->popupMenu();
  }
}

bool
UIMediator::attemptReplayFile(QString const &path)
{
  suscan_source_metadata meta;
  Suscan::Source::Config prof = *getProfile();
  bool haveRate = false;

  prof.setType("file");
  prof.setFormat(SUSCAN_SOURCE_FORMAT_AUTO);
  prof.setPath(path.toStdString());

  if (!prof.guessMetadata(meta)) {
    QMessageBox::critical(
          m_owner,
          "Replay file",
          "Cannot replay file: failed to extract meaningful metadata from it.\n\n"
          "If this is a raw IQ file, please use the source settings dialog instead.");
    return false;
  }

  auto st = prof.getStartTime();
  if ((meta.guessed & SUSCAN_SOURCE_CONFIG_GUESS_START_TIME)
      && (st.tv_sec != meta.start_time.tv_sec || st.tv_usec != meta.start_time.tv_usec)) {
    prof.setStartTime(meta.start_time);
  }

  haveRate = meta.guessed & SUSCAN_SOURCE_CONFIG_GUESS_SAMP_RATE;
  if (haveRate && prof.getSampleRate() != meta.sample_rate)
    prof.setSampleRate(meta.sample_rate);

  if (meta.guessed & SUSCAN_SOURCE_CONFIG_GUESS_FREQ) {
    if (!sufeq(prof.getFreq(), meta.frequency, 1)) {
      prof.setLnbFreq(0);
      prof.setFreq(meta.frequency);
    }
  }

  if ((meta.guessed & SUSCAN_SOURCE_CONFIG_GUESS_FORMAT)
      && meta.format != prof.getFormat()) {
    prof.setFormat(meta.format);
  }

  if (!haveRate) {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
          m_owner,
          "Replay file",
          "The file was recognized, but the sample rate could not be guessed "
          "from metadata. Do you want to open the settings dialog to manually "
          "define the sample rate?",
          QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
      if (!openSettingsDialog(&prof))
        return true;
  } else {
    setProfile(prof, true);

    if (m_state == HALTED)
      emit captureStart();
  }

  m_appConfig->lastLoadedFile = path.toStdString();

  return true;
}

void
UIMediator::onTriggerReplayFile(bool)
{
  QString title, path;
  QString prevPath = QString::fromStdString(m_appConfig->lastLoadedFile);
  QFileInfo fi(prevPath);
  QStringList formats;
  QString selected = "All files (*)";


  title = "Open capture file";
  formats
      << "Raw complex 32-bit float (*.raw *.cf32)"
      << "Raw complex 8-bit unsigned (*.u8 *.cu8)"
      << "Raw complex 8-bit signed (*.s8 *.cs8)"
      << "Raw complex 16-bit signed (*.s16 *.cs16)"
      << "WAV files (*.wav)"
      << "SigMF signal recordings (*.sigmf-data *.sigmf-meta)"
      << "All files (*)";

  if (!prevPath.isEmpty())
    for (auto p : formats)
      if (p.contains("*." + fi.suffix()))
        selected = p;

  do {
    path = QFileDialog::getOpenFileName(
           m_owner,
           title,
           fi.absolutePath(),
           formats.join(";;"),
           &selected);

    if (path.isEmpty())
      break;
  } while (!attemptReplayFile(path));
}
