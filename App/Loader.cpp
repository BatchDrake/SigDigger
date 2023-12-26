//
//    Loader.cpp: GUI-Level Suscan initialization
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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

#include <iostream>

#include <QThread>
#include <QMessageBox>
#include <SigDiggerHelpers.h>

#include <Loader.h>

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif // HAVE_CURL

using namespace SigDigger;

//////////////////////////////// Loader thread ///////////////////////////////
InitThread::InitThread(QObject *parent) : QThread(parent) { }

void
InitThread::run()
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();
  QString verString;

  try {
    emit change("Generating FFT wisdom (this may take a while)");
    su_lib_gen_wisdom();
    emit change("Loading signal sources");
    sing->init_sources();
    emit change("Loading spectrum sources");
    sing->init_spectrum_sources();
    emit change("Loading estimators");
    sing->init_estimators();
    emit change("Loading inspectors");
    sing->init_inspectors();
    emit change("Loading palettes");
    sing->init_palettes();
    emit change("Loading frequency tables");
    sing->init_fats();
    emit change("Loading bookmarks");
    sing->init_bookmarks();
    emit change("Loading locations");
    sing->init_locations();
    emit change("Loading TLE sources");
    sing->init_tle_sources();
    emit change("Loading satellites from TLE");
    sing->init_tle();
    emit change("Loading auto gains");
    sing->init_autogains();
    emit change("Loading UI config");
    sing->init_ui_config();
    emit change("Loading profile history");
    sing->init_recent_list();
    emit change("Init done, triggering delayed plugin tasks...");
    sing->trigger_delayed();
  } catch (Suscan::Exception const &e) {
    emit failure(QString(e.what()));
  }

  verString =
      "SigDigger "
      + SigDiggerHelpers::version()
      + " loaded.";

  SU_INFO(
        "%s\n",
        verString.toStdString().c_str());

  emit done();
}

///////////////////////////////// Loader UI //////////////////////////////////
Loader::Loader(Application *app)
{
  QFont font;
  static QPixmap background(QString(":/icons/splash.png"));

  m_suscan = Suscan::Singleton::get_instance();
  m_app = app;

  // Init CURL
#ifdef HAVE_CURL
  if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
    fprintf(stderr, "*** CURL Initialization failed ***\n");
    exit(EXIT_FAILURE);
  }
#endif // HAVE_CURL

  // Allocate resources
  flushLog();
  m_initThread = std::make_unique<InitThread>(this);

  font.setBold(true);
  font.setPixelSize(12);
  font.setStretch(125);

  setPixmap(background);
  setFont(font);
  setWindowFlags(Qt::WindowStaysOnTopHint | Qt::SplashScreen);
  finish(m_app);

  // Connect thread to this object
  connect(
        m_initThread.get(),
        SIGNAL(done()),
        this,
        SLOT(handleDone()),
        Qt::QueuedConnection);

  connect(
        m_initThread.get(),
        SIGNAL(change(const QString &)),
        this,
        SLOT(handleChange(const QString &)),
        Qt::QueuedConnection);

  connect(
        m_initThread.get(),
        SIGNAL(failure(const QString &)),
        this,
        SLOT(handleFailure(const QString &)),
        Qt::QueuedConnection);
}

void
Loader::flushLog()
{
  Suscan::Logger::getInstance()->flush();
}

QString
Loader::getLogText()
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

Loader::~Loader()
{
}

// Signal handlers
void
Loader::handleChange(const QString &state)
{
  showMessage(state + "...");
}

void
Loader::handleFailure(const QString &state)
{
  QMessageBox::critical(
        this,
        "Suscan initialization",
        "Failed to initialize Suscan's utility library: " + state +
        "<pre>" + getLogText() + "</pre>",
        QMessageBox::Close);

  QApplication::quit();
}

void
Loader::handleDone()
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();
  Suscan::Object objConfig;

  for (auto p = sing->getFirstUIConfig(); p != sing->getLastUIConfig(); p++) {
    if (p->getClass() == "qtui") {
      m_confIndex = static_cast<unsigned>(p - sing->getFirstUIConfig());
      objConfig = *p;
      break;
    }
  }

  m_app->run(objConfig);

  connect(
        m_app,
        SIGNAL(triggerSaveConfig()),
        this,
        SLOT(saveConfig()));

  close();
}

// Public methods
void
Loader::saveConfig()
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();

  m_app->refreshConfig();

  try {
    Suscan::Object obj = m_app->getConfig();

    sing->putUIConfig(m_confIndex, std::move(obj));

    sing->sync();

    Suscan::ConfigContext::saveAll();
  } catch (Suscan::Exception const &e) {
    QWidget *parent = isVisible()
        ? SCAST(QWidget *, this)
        : SCAST(QWidget *, m_app);

    QMessageBox::critical(
          parent,
          "Save configuration",
          "Failed to save SigDigger's configuration: <pre>"
          + QString::fromStdString(e.what()) + "</pre>",
          QMessageBox::Close);
  }
}

void
Loader::showMessage(const QString &message)
{
  QSplashScreen::showMessage(message, Qt::AlignLeft, Qt::white);
}


void
Loader::load()
{
  setlocale(LC_NUMERIC,"C");

  show();
  m_initThread->start();
}
