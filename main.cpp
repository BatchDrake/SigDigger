//
//    main.cpp: Entry point
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

#include <QApplication>
#include <QTranslator>
#include <RMSViewer.h>
#include <iostream>
#include <QFont>
#include <QSurface>
#include "Loader.h"
#include <QtGlobal>

#include <sigutils/version.h>
#include <analyzer/version.h>
#include <FileViewer.h>

#include <cstring>
#include <getopt.h>

#define MAX_LOG_MESSAGES 20

using namespace SigDigger;

static int
runRMSViewer(QApplication &app)
{
  int ret;
  RMSViewer viewer;

  viewer.show();
  ret = app.exec();

  return ret;
}

static int
runFileViewer(QApplication &app)
{
  int ret;
  FileViewer viewer;

  ret = app.exec();

  return ret;
}

static QString
getLogText(void)
{
  QString text = "";
  std::lock_guard<Suscan::Logger> guard(*Suscan::Logger::getInstance());

  auto begin = Suscan::Logger::getInstance()->begin();
  auto end   = Suscan::Logger::getInstance()->end();

  if (end - begin > MAX_LOG_MESSAGES) {
    auto removed = end - begin - MAX_LOG_MESSAGES;
    begin = end - MAX_LOG_MESSAGES;

    text += "(" + QString::number(removed) + " previous messages omitted)\n";
  }

  for (auto p = begin; p != end; ++p) {
    switch (p->severity) {
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

    text += p->message.c_str();
  }

  return text;
}

static int
runSigDigger(QApplication &app)
{
  int ret = 1;

  try {
    Application main_app;
    Loader loader(&main_app);

    QSurfaceFormat fmt;
    fmt.setSamples(16);
    QSurfaceFormat::setDefaultFormat(fmt);

    loader.load();

    ret = app.exec();

    Suscan::Singleton::get_instance()->killBackgroundTaskController();

    std::cout << "Saving config..." << std::endl;

    loader.saveConfig();
  } catch (Suscan::Exception const &e) {
    (void) QMessageBox::critical(
          nullptr,
          "SigDigger internal error",
          QString(e.what())
          + "<pre>" + getLogText() + "</pre>",
          QMessageBox::Close);
    app.quit();
  }

  return ret;
}


static void
help(const char *argv0)
{
  fprintf(stderr, "%s: SigDigger launcher binary\n", argv0);
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s [options] \n\n", argv0);

  fprintf(stderr, "Options:\n\n");
  fprintf(stderr, "     -t, --tool=\"tool name\"  Tool to launch\n");
  fprintf(stderr, "     -h, --help              This help\n\n");
  fprintf(
        stderr,
        "Tool name can be either one of SigDigger (default) and RMSViewer\n\n");

  fprintf(
      stderr,
      "Using suscan version %s (%s)\n",
      suscan_api_version(),
      suscan_pkgversion());

  fprintf(
      stderr,
      "Using sigutils version %s (%s)\n\n",
      sigutils_api_version(),
      sigutils_pkgversion());

  fprintf(stderr, "Copyright © 2020 Gonzalo José Carracedo Carballal\n");
  fprintf(
      stderr,
      "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
}

static struct option long_options[] = {
  {"tool",  required_argument, nullptr, 't' },
  {"help",  no_argument,       nullptr, 'h' },
  {nullptr, 0,                 nullptr, 0 }
};


int
main(int argc, char *argv[])
{
  // This is a workaround for the poor backwards compatibility of macOS
  // with software that used to work in previous releases. macOS is far
  // from being a serious piece of engineering, and developing stuff
  // in macOS implies handling all the stuff that Apple engineers forgot
  // to do right.
  
#ifdef Q_OS_MACOS
  qputenv("QT_MAC_WANTS_LAYER", "1");
#endif // Q_OS_MACOS
  
  QApplication app(argc, argv);
  QString appName = "SigDigger";
  int ret = EXIT_FAILURE;
  int c;

  QTranslator translator;
  if (!translator.load(QLocale(), "SigDigger", "_", ":/i18n"))
    fprintf(stderr, "%s: locale %s not found, using English as default\n", argv[0], QLocale().name().toStdString().c_str());
  else
    app.installTranslator(&translator);

#ifdef __APPLE__
  QFont::insertSubstitution("Monospace", "Monaco");
#endif // __APPLE__

  while (true) {
    int option_index = 0;

    c = getopt_long(argc, argv, "t:h", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
      case 't':
        appName = optarg;
        break;

      case 'h':
        help(argv[0]);
        exit(EXIT_SUCCESS);

      case '?':
        fprintf(stderr, "%s: invalid option `-%c'.\n", argv[0], optopt);
        help(argv[0]);
        break;

      default:
        fprintf(stderr, "%s: unexpected getopt_long retcode %d\n", argv[0], c);
    }
  }

  if (appName == "SigDigger") {
    ret = runSigDigger(app);
  } else if (appName == "RMSViewer") {
    ret = runRMSViewer(app);
  } else if (appName == "FileViewer") {
    ret = runFileViewer(app);
  } else {
    fprintf(
          stderr,
          "%s: unknown tool `%s'\n",
          argv[0],
          appName.toStdString().c_str());
  }

  exit(ret);
}
