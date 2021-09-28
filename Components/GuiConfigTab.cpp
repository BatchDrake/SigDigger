//
//    GuiConfigTab.cpp: GUI tweaks
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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
#include "GuiConfigTab.h"
#include "ui_GuiConfigTab.h"

using namespace SigDigger;

void
GuiConfigTab::saveGuiConfigUi()
{
  this->guiConfig.useLMBdrag = this->ui->reverseDragBehaviorCheck->isChecked();
}

void
GuiConfigTab::refreshGuiConfigUi()
{
  this->ui->reverseDragBehaviorCheck->setChecked(this->guiConfig.useLMBdrag);
}

void
GuiConfigTab::connectAll(void)
{
  // None, but it will be necessary
}

GuiConfigTab::GuiConfigTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::GuiConfigTab)
{
  ui->setupUi(this);

  this->connectAll();
}

GuiConfigTab::~GuiConfigTab()
{
  delete ui;
}
