//
//    AppUI.cpp: Initialize UI controls
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

#include "AppUI.h"

using namespace SigDigger;

AppUI::AppUI(QMainWindow *owner)
{
  this->main = new Ui_MainWindow();

  this->main->setupUi(owner);

  this->spectrum = new MainSpectrum(owner);
  this->sourcePanel = new SourcePanel(owner);
  this->inspectorPanel = new InspectorPanel(owner);
  this->fftPanel = new FftPanel(owner);
  this->audioPanel = new AudioPanel(owner);
  this->aboutDialog = new AboutDialog(owner);
  this->deviceDialog = new DeviceDialog(owner);
}

void
AppUI::postLoadInit(QMainWindow *owner)
{
  this->configDialog = new ConfigDialog(owner);
  this->fftPanel->deserializePalettes();
  this->sourcePanel->deserializeAutoGains();
}

AppUI::~AppUI(void)
{
  delete this->main;
}
