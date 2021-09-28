//
//    ConfigDialog.cpp: Configuration dialog window
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

#include <QFileDialog>
#include <QMessageBox>

#include <SuWidgetsHelpers.h>
#include <ProfileConfigTab.h>
#include <ColorConfigTab.h>
#include <GuiConfigTab.h>
#include <LocationConfigTab.h>
#include <time.h>
#include "ConfigDialog.h"

using namespace SigDigger;

Q_DECLARE_METATYPE(Suscan::Source::Config); // Unicorns
Q_DECLARE_METATYPE(Suscan::Source::Device); // More unicorns

void
ConfigDialog::connectAll(void)
{
  connect(
         this,
         SIGNAL(accepted(void)),
         this,
         SLOT(onAccepted(void)));
}

void
ConfigDialog::setAnalyzerParams(const Suscan::AnalyzerParams &params)
{
  this->analyzerParams = params;
}

void
ConfigDialog::setProfile(const Suscan::Source::Config &profile)
{
  this->profileTab->setProfile(profile);
}

void
ConfigDialog::setFrequency(qint64 val)
{
  this->profileTab->setFrequency(val);
}

void
ConfigDialog::notifySingletonChanges(void)
{
  this->profileTab->notifySingletonChanges();
}

bool
ConfigDialog::remoteSelected(void) const
{
  return this->profileTab->remoteSelected();
}

void
ConfigDialog::setGain(std::string const &name, float value)
{
  this->profileTab->setGain(name, value);
}

float
ConfigDialog::getGain(std::string const &name) const
{
  return this->profileTab->getGain(name);
}

Suscan::AnalyzerParams
ConfigDialog::getAnalyzerParams(void) const
{
  return this->analyzerParams;
}

Suscan::Source::Config
ConfigDialog::getProfile(void) const
{
  return this->profileTab->getProfile();
}

void
ConfigDialog::setColors(ColorConfig const &config)
{
  this->colorTab->setColorConfig(config);
}

ColorConfig
ConfigDialog::getColors(void) const
{
  return this->colorTab->getColorConfig();
}

void
ConfigDialog::setGuiConfig(GuiConfig const &config)
{
  this->guiTab->setGuiConfig(config);
}

GuiConfig
ConfigDialog::getGuiConfig() const
{
  return this->guiTab->getGuiConfig();
}

bool
ConfigDialog::run(void)
{
  this->accepted = false;

  this->exec();

  return this->accepted;
}

ConfigDialog::ConfigDialog(QWidget *parent) :
  QDialog(parent)
{
  this->ui = new Ui_Config();
  this->ui->setupUi(this);
  this->setWindowFlags(
    this->windowFlags() & ~Qt::WindowMaximizeButtonHint);
  this->layout()->setSizeConstraint(QLayout::SetFixedSize);

  this->profileTab  = new ProfileConfigTab(this);
  this->colorTab    = new ColorConfigTab(this);
  this->guiTab      = new GuiConfigTab(this);
  this->locationTab = new LocationConfigTab(this);

  this->ui->tabWidget->addTab(this->profileTab,  "Source");
  this->ui->tabWidget->addTab(this->colorTab,    "Colors");
  this->ui->tabWidget->addTab(this->guiTab,      "GUI behavior");
  this->ui->tabWidget->addTab(this->locationTab, "Location");

  this->connectAll();
}

ConfigDialog::~ConfigDialog()
{
  delete this->ui;
}


void
ConfigDialog::onAccepted(void)
{
  this->guiTab->save();
  this->colorTab->save();
  this->locationTab->save();

  // warning: it will trigger reconfiguring device
  // and gui refresh from stored variables
  this->profileTab->save();
  this->accepted = true;
}

