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
GuiConfigTab::save()
{
  this->guiConfig.useLMBdrag     = this->ui->reverseDragBehaviorCheck->isChecked();
  this->guiConfig.noLimits       = this->ui->noLimitsCheck->isChecked();
  this->guiConfig.useGLWaterfall = this->ui->useGLWaterfallCheck->isChecked();
  this->guiConfig.useMaxBlending = this->ui->useMaxBlendingCheck->isChecked();
  this->guiConfig.useGlInWindows = this->ui->useGlWfInWindowsCheck->isChecked();
  this->guiConfig.enableMsgTTL   = this->ui->ttlCheck->isChecked();
  this->guiConfig.msgTTL         = static_cast<unsigned>(
        this->ui->ttlSpin->value());
}

void
GuiConfigTab::refreshUi()
{
  this->ui->reverseDragBehaviorCheck->setChecked(this->guiConfig.useLMBdrag);
  this->ui->noLimitsCheck->setChecked(this->guiConfig.noLimits);
  this->ui->useGLWaterfallCheck->setChecked(this->guiConfig.useGLWaterfall);
  this->ui->useMaxBlendingCheck->setEnabled(
        this->ui->useGLWaterfallCheck->isChecked());
  this->ui->useGlWfInWindowsCheck->setEnabled(
        this->ui->useGLWaterfallCheck->isChecked());
  this->ui->useMaxBlendingCheck->setChecked(this->guiConfig.useMaxBlending);
  this->ui->ttlCheck->setChecked(this->guiConfig.enableMsgTTL);
  this->ui->ttlLabel->setEnabled(this->ui->ttlCheck->isChecked());
  this->ui->ttlSpin->setEnabled(this->ui->ttlCheck->isChecked());
  this->ui->ttlSpin->setValue(static_cast<int>(this->guiConfig.msgTTL));

}

void
GuiConfigTab::setGuiConfig(GuiConfig const &config)
{
  this->guiConfig = config;
  this->refreshUi();
  this->modified = false;
}

GuiConfig
GuiConfigTab::getGuiConfig() const
{
  return this->guiConfig;
}

bool
GuiConfigTab::hasChanged() const
{
  return this->modified;
}

void
GuiConfigTab::connectAll()
{
  connect(
        this->ui->reverseDragBehaviorCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onConfigChanged()));

  connect(
        this->ui->noLimitsCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onConfigChanged()));

  connect(
        this->ui->useGLWaterfallCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onConfigChanged()));

  connect(
        this->ui->useMaxBlendingCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onConfigChanged()));

  connect(
        this->ui->useGlWfInWindowsCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onConfigChanged()));

  connect(
        this->ui->ttlCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onConfigChanged()));

  connect(
        this->ui->ttlSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onConfigChanged()));
}

GuiConfigTab::GuiConfigTab(QWidget *parent) :
  ConfigTab(parent, "GUI behavior"),
  ui(new Ui::GuiConfigTab)
{
  ui->setupUi(this);

  this->connectAll();
}

GuiConfigTab::~GuiConfigTab()
{
  delete ui;
}

////////////////////////////////// Slots ///////////////////////////////////////
void
GuiConfigTab::onConfigChanged()
{
  this->ui->useMaxBlendingCheck->setEnabled(
        this->ui->useGLWaterfallCheck->isChecked());
  this->ui->useGlWfInWindowsCheck->setEnabled(
        this->ui->useGLWaterfallCheck->isChecked());

  this->ui->ttlLabel->setEnabled(this->ui->ttlCheck->isChecked());
  this->ui->ttlSpin->setEnabled(this->ui->ttlCheck->isChecked());

  this->modified = true;
  emit changed();
}
