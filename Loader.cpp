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

using namespace QStones;

//////////////////////////////// Loader thread ///////////////////////////////
InitThread::InitThread(QObject *parent) : QThread(parent) { }

void
InitThread::run()
{
  Suscan::Singleton *sing = Suscan::Singleton::get_instance();

  try {
    emit change("Loading spectrum sources");
    sing->init_sources();
  } catch (Suscan::Exception &e) {
    emit failure(QString(e.what()));
  }

  emit done();
}

///////////////////////////////// Loader UI //////////////////////////////////
Loader::Loader(Application *app)
{
  QFont font;
  static QPixmap background(QString(":/splash.png"));

  this->suscan = Suscan::Singleton::get_instance();
  this->app = app;

  // Allocate resources
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
        "Failed to initialize Suscan's utility library: " + state,
        QMessageBox::Close);

  QApplication::quit();
}

void
Loader::handleDone(void)
{
  this->app->run();
  this->close();
}

// Public methods
void
Loader::showMessage(const QString &message)
{
  QSplashScreen::showMessage(message, Qt::AlignLeft, Qt::white);
}


void
Loader::load(void)
{
  this->show();
  this->initThread->start();
}
