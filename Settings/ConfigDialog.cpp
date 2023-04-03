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

#include <SuWidgetsHelpers.h>
#include <ProfileConfigTab.h>
#include <ColorConfigTab.h>
#include <AudioConfigTab.h>
#include <GuiConfigTab.h>
#include <TLESourceTab.h>
#include <LocationConfigTab.h>
#include <time.h>
#include "ConfigDialog.h"

using namespace SigDigger;

Q_DECLARE_METATYPE(Suscan::Source::Config); // Unicorns
Q_DECLARE_METATYPE(Suscan::Source::Device); // More unicorns

void
ConfigDialog::connectAll()
{
  connect(
         this,
         SIGNAL(accepted()),
         this,
         SLOT(onAccepted()));
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
ConfigDialog::notifySingletonChanges()
{
  this->profileTab->notifySingletonChanges();
}

bool
ConfigDialog::remoteSelected() const
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
ConfigDialog::getAnalyzerParams() const
{
  return this->analyzerParams;
}

Suscan::Source::Config
ConfigDialog::getProfile() const
{
  return this->profileTab->getProfile();
}

void
ConfigDialog::setColors(ColorConfig const &config)
{
  this->colorTab->setColorConfig(config);
}

void
ConfigDialog::setTleSourceConfig(const TLESourceConfig &config)
{
  return this->tleSourceTab->setTleSourceConfig(config);
}


ColorConfig
ConfigDialog::getColors() const
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

TLESourceConfig
ConfigDialog::getTleSourceConfig() const
{
  return this->tleSourceTab->getTleSourceConfig();
}

bool
ConfigDialog::profileChanged() const
{
  return this->profileTab->hasChanged();
}

bool
ConfigDialog::colorsChanged() const
{
  return this->colorTab->hasChanged();
}

bool
ConfigDialog::guiChanged() const
{
  return this->guiTab->hasChanged();
}

bool
ConfigDialog::tleSourceConfigChanged() const
{
  return this->tleSourceTab->hasChanged();
}

bool
ConfigDialog::locationChanged() const
{
  return this->locationTab->hasChanged();
}

Suscan::Location
ConfigDialog::getLocation() const
{
  return this->locationTab->getLocation();
}

void
ConfigDialog::setLocation(Suscan::Location const &loc)
{
  this->locationTab->setLocation(loc);
}

void
ConfigDialog::setAudioConfig(AudioConfig const &config)
{
  this->audioTab->setAudioConfig(config);
}

AudioConfig
ConfigDialog::getAudioConfig() const
{
  return this->audioTab->getAudioConfig();
}

bool
ConfigDialog::sourceNeedsRestart() const
{
  return this->profileTab->shouldRestart();
}

bool
ConfigDialog::run()
{
  this->accepted = false;
  this->setWindowTitle("Settings");

  this->exec();

  return this->accepted;
}

void
ConfigDialog::appendConfigTab(ConfigTab *tab)
{
  this->ui->tabWidget->addTab(tab, tab->getName());

  connect(
        tab,
        SIGNAL(changed()),
        this,
        SLOT(onTabConfigChanged()));
}

ConfigDialog::ConfigDialog(QWidget *parent) :
  QDialog(parent)
{
  this->ui = new Ui_Config();
  this->ui->setupUi(this);
  this->setWindowFlags(
    this->windowFlags() & ~Qt::WindowMaximizeButtonHint);
  //this->layout()->setSizeConstraint(QLayout::SetFixedSize);

  this->profileTab   = new ProfileConfigTab(this);
  this->colorTab     = new ColorConfigTab(this);
  this->audioTab     = new AudioConfigTab(this);
  this->guiTab       = new GuiConfigTab(this);
  this->locationTab  = new LocationConfigTab(this);
  this->tleSourceTab = new TLESourceTab(this);

  this->appendConfigTab(this->profileTab);
  this->appendConfigTab(this->colorTab);
  this->appendConfigTab(this->audioTab);
  this->appendConfigTab(this->guiTab);
  this->appendConfigTab(this->tleSourceTab);
  this->appendConfigTab(this->locationTab);

  this->connectAll();
}

ConfigDialog::~ConfigDialog()
{
  delete this->ui;
}


void
ConfigDialog::onAccepted()
{
  int i;

  for (i = 0; i < this->ui->tabWidget->count(); ++i) {
    ConfigTab *tab =
        static_cast<ConfigTab *>(this->ui->tabWidget->widget(i));
    tab->save();
  }

  this->accepted = true;
}

void
ConfigDialog::onTabConfigChanged()
{
  this->setWindowTitle("Settings [changed]");
}
