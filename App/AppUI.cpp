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


#include "AboutDialog.h"
#include "MainSpectrum.h"
#include "ConfigDialog.h"
#include "Palette.h"
#include "AutoGain.h"
#include "Averager.h"
#include "DeviceGain.h"
#include "ui_MainWindow.h"
#include "ConfigDialog.h"
#include "DeviceDialog.h"
#include "PanoramicDialog.h"
#include "LogDialog.h"
#include "BackgroundTasksDialog.h"
#include "AddBookmarkDialog.h"
#include "BookmarkManagerDialog.h"
#include <QToolBar>
#include "QTimeSlider.h"
#include "AppUI.h"
#include "QuickConnectDialog.h"

#include "SigDiggerHelpers.h"
#include <QToolBar>
#include <QTimeSlider.h>
#include <ToolBarWidgetFactory.h>

using namespace SigDigger;

AppUI::AppUI(QMainWindow *owner)
{
  this->main = new Ui_MainWindow();
  this->main->setupUi(owner);

  this->owner = owner;

  this->timeToolbar = new QToolBar("Time controls");
  this->timeSlider = new QTimeSlider(nullptr);

  this->timeToolbar->setAllowedAreas(Qt::ToolBarArea::AllToolBarAreas);
  this->timeToolbar->setMovable(false);

  owner->addToolBarBreak(Qt::TopToolBarArea);
  owner->addToolBar(Qt::TopToolBarArea, timeToolbar);

  this->timeToolbar->addWidget(this->timeSlider);

  // In MacOS X there is already a system feature to show windows in full
  // screen, and therefore this option is not needed.
#ifdef __APPLE__
  delete this->main->action_Full_screen;
  this->main->action_Full_screen = nullptr;
#endif // __APPLE__
  
  this->spectrum = new MainSpectrum(owner);
  this->quickConnectDialog = new QuickConnectDialog(owner);
  this->aboutDialog = new AboutDialog(owner);
  this->deviceDialog = new DeviceDialog(owner);
  this->panoramicDialog = new PanoramicDialog(owner);
  this->logDialog = new LogDialog(owner);
  this->backgroundTasksDialog = new BackgroundTasksDialog(owner);
  this->addBookmarkDialog = new AddBookmarkDialog(owner);
  this->bookmarkManagerDialog = new BookmarkManagerDialog(owner);
}

void
AppUI::addToolBarWidget(ToolBarWidget *widget)
{
  ToolBarWidgetFactory *factory = widget->factory();
  QString name = factory->name();

  if (toolBarWidgets.contains(name)) {
    SU_ERROR("Attempting to add `%s' to toolbar twice!\n", factory->name());
    return;
  }

  QToolBar *toolBar = new QToolBar(factory->desc());

  toolBar->setAllowedAreas(Qt::ToolBarArea::AllToolBarAreas);
  toolBar->setMovable(true);

  this->owner->insertToolBar(this->timeToolbar, toolBar);

  toolBar->addWidget(widget);

  QObject::connect(
        widget,
        SIGNAL(destroyed(QObject*)),
        toolBar,
        SLOT(deleteLater()));
}

void
AppUI::postLoadInit(UIMediator *mediator, QMainWindow *owner)
{
  this->uiMediator = mediator;

  // Singleton config has been deserialized. Refresh UI with these changes.
  SigDiggerHelpers::instance()->deserializePalettes();

  this->configDialog = new ConfigDialog(owner);
  this->spectrum->deserializeFATs();

  this->spectrum->adjustSizes();
}

AppUI::~AppUI(void)
{
  delete this->main;
}
