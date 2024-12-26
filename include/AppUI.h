//
//    filename: description
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
#ifndef APPUI_H
#define APPUI_H

#include <Suscan/Analyzer.h>
#include <QHash>

class QMainWindow;
class Ui_MainWindow;
class QToolBar;
class QDialog;

namespace SigDigger {
  class ConfigDialog;
  class DeviceDialog;
  class PanoramicDialog;
  class MainSpectrum;
  class AboutDialog;
  class DataSaverUI;
  class LogDialog;
  class BackgroundTasksDialog;
  class AddBookmarkDialog;
  class BookmarkManagerDialog;
  class QTimeSlider;
  class Inspector;
  class QuickConnectDialog;
  class ToolBarWidgetFactory;
  class ToolBarWidget;
  class UIMediator;

  struct AppUI {
    QMainWindow *owner;
    QHash<QString, ToolBarWidget *> toolBarWidgets;

    Ui_MainWindow *main = nullptr;
    UIMediator *uiMediator = nullptr;
    ConfigDialog *configDialog = nullptr;
    DeviceDialog *deviceDialog = nullptr;
    PanoramicDialog *panoramicDialog = nullptr;
    MainSpectrum *spectrum = nullptr;
    AboutDialog *aboutDialog = nullptr;
    DataSaverUI *dataSaverUI = nullptr;
    LogDialog *logDialog = nullptr;
    QuickConnectDialog *quickConnectDialog = nullptr;
    BackgroundTasksDialog *backgroundTasksDialog = nullptr;
    AddBookmarkDialog *addBookmarkDialog = nullptr;
    BookmarkManagerDialog *bookmarkManagerDialog = nullptr;
    QToolBar *timeToolbar;
    QTimeSlider *timeSlider = nullptr;

    AppUI(QMainWindow *);
    void addToolBarWidget(ToolBarWidget *);
    void postLoadInit(UIMediator *, QMainWindow *owner);
    ~AppUI();
  };
}

#endif // APPUI_H
