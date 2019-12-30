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

#include <Loader.h>

using namespace SigDigger;

//////////////////////////////// Loader thread ///////////////////////////////
InitThread::InitThread(QObject *parent) : QThread(parent) { }

void
InitThread::run()
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();

  try {
    emit change("Loading signal sources");
    sing->init_sources();
    emit change("Loading spectrum sources");
    sing->init_spectrum_sources();
    emit change("Loading estimators");
    sing->init_estimators();
    emit change("Loading inspectors");
    sing->init_inspectors();
    emit change("Loading codecs");
    sing->init_codecs();
    emit change("Loading palettes");
    sing->init_palettes();
    emit change("Loading auto gains");
    sing->init_autogains();
    emit change("Loading UI config");
    sing->init_ui_config();
    emit change("Loading profile history");
    sing->init_recent_list();
  } catch (Suscan::Exception const &e) {
    emit failure(QString(e.what()));
  }

  emit done();
}

///////////////////////////////// Loader UI //////////////////////////////////
Loader::Loader(Application *app)
{
  QFont font;
  static QPixmap background(QString(":/icons/splash.png"));

  this->suscan = Suscan::Singleton::get_instance();
  this->app = app;

  // Allocate resources
  this->flushLog();
  this->initThread = std::make_unique<InitThread>(this);

  font.setBold(true);
  font.setPixelSize(12);
  font.setStretch(125);

  this->setPixmap(background);
  this->setFont(font);
  this->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::SplashScreen);
  this->finish(this->app);

  // Connect thread to this object
  connect(
        this->initThread.get(),
        SIGNAL(done(void)),
        this,
        SLOT(handleDone(void)),
        Qt::QueuedConnection);

  connect(
        this->initThread.get(),
        SIGNAL(change(const QString &)),
        this,
        SLOT(handleChange(const QString &)),
        Qt::QueuedConnection);

  connect(
        this->initThread.get(),
        SIGNAL(failure(const QString &)),
        this,
        SLOT(handleFailure(const QString &)),
        Qt::QueuedConnection);
}

void
Loader::flushLog(void)
{
  Suscan::Logger::getInstance()->flush();
}

QString
Loader::getLogText(void)
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
  this->showMessage(state + "...");
}

void
Loader::handleFailure(const QString &state)
{
  (void) QMessageBox::critical(
        this,
        "Suscan initialization",
        "Failed to initialize Suscan's utility library: " + state +
        "<pre>" + this->getLogText() + "</pre>",
        QMessageBox::Close);

  QApplication::quit();
}

void
Loader::handleDone(void)
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();
  Suscan::Object objConfig;

  for (auto p = sing->getFirstUIConfig(); p != sing->getLastUIConfig(); p++) {
    if (p->getClass() == "qtui") {
      this->confIndex = static_cast<unsigned>(p - sing->getFirstUIConfig());
      objConfig = *p;
      break;
    }
  }

  this->app->run(objConfig);
  this->close();
}

// Public methods
void
Loader::saveConfig(void)
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();

  this->app->refreshConfig();

  try {
    Suscan::Object obj = std::move(this->app->getConfig());

    sing->putUIConfig(this->confIndex, std::move(obj));

    sing->sync();

    Suscan::ConfigContext::saveAll();
  } catch (Suscan::Exception const &e) {
    (void) QMessageBox::critical(
          this,
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
Loader::load(void)
{
  setlocale(LC_NUMERIC,"C");

  this->show();
  this->initThread->start();
}
