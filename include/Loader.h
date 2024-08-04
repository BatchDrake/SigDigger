//
//    Loader.h: Load Suscan from library
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
#ifndef LOADER_H
#define LOADER_H

#include <QApplication>
#include <QMainWindow>
#include <QThread>
#include <QSplashScreen>

#include <Suscan/Library.h>

#include "Application.h"

namespace SigDigger {
  class InitThread: public QThread {
    Q_OBJECT

    void run() override;

  public:
    InitThread(QObject *parent);

  signals:
    void done();
    void change(const QString &state);
    void failure(const QString &reason);
  };

  class Loader: public QSplashScreen {
    Q_OBJECT

    unsigned int m_confIndex = 0;
    void flushLog();
    QString getLogText();

    // Owned pointers
    std::unique_ptr<InitThread> m_initThread; // QT wants this to be a pointer

    // Borrowed pointers
    Application *m_app;
    Suscan::Singleton *m_suscan;
    void showMessage(const QString &message);

  public:
    Loader(Application *app);
    ~Loader();
    void load();

  public slots:
    void handleChange(const QString &state);
    void handleFailure(const QString &state);
    void handleDone();

    void saveConfig();
  };
};

#endif // LOADER_H
