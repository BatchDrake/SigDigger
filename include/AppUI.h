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

#include "AboutDialog.h"
#include "SourcePanel.h"
#include "InspectorPanel.h"
#include "FftPanel.h"
#include "AudioPanel.h"
#include "MainSpectrum.h"
#include "ConfigDialog.h"
#include "Palette.h"
#include "AutoGain.h"
#include "Averager.h"
#include "DeviceGain.h"
#include "Inspector.h"
#include "ui_MainWindow.h"
#include "ConfigDialog.h"
#include "DeviceDialog.h"
#include "PanoramicDialog.h"

namespace SigDigger {
  struct AppUI {
    Ui_MainWindow *main = nullptr;
    ConfigDialog *configDialog = nullptr;
    DeviceDialog *deviceDialog = nullptr;
    PanoramicDialog *panoramicDialog = nullptr;
    MainSpectrum *spectrum = nullptr;
    SourcePanel *sourcePanel = nullptr;
    InspectorPanel *inspectorPanel = nullptr;
    FftPanel *fftPanel = nullptr;
    AudioPanel *audioPanel = nullptr;
    AboutDialog *aboutDialog = nullptr;
    DataSaverUI * dataSaverUI = nullptr;

    std::map<Suscan::InspectorId, Inspector *> inspectorTable;
    Suscan::InspectorId lastId = 0;

    AppUI(QMainWindow *);
    void postLoadInit(QMainWindow *owner);
    ~AppUI();
  };
}

#endif // APPUI_H
