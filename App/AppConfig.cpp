//
//    AppConfig.h: Application configuration object
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

#include "AppConfig.h"
#include "PanoramicDialog.h"

#include <Suscan/Library.h>

using namespace SigDigger;

AppConfig::AppConfig(AppUI *ui)
  : profile{},
  analyzerParams{},
  colors{},
  guiConfig{},
  cachedComponentConfig(SUSCAN_OBJECT_TYPE_OBJECT),
  enabledBandPlans{}
{
  this->panSpectrumConfig = ui->panoramicDialog->getConfig();
}

Suscan::Object &&
AppConfig::serialize(void)
{
  Suscan::Object profileObj = this->profile.serialize();
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);
  Suscan::Object bandPlans(SUSCAN_OBJECT_TYPE_SET);
  Suscan::Object componentConfigCopy;

  obj.setClass("qtui");

  obj.set("version", this->version);
  obj.set("width", this->width);
  obj.set("height", this->height);
  obj.set("x", this->x);
  obj.set("y", this->y);
  obj.set("fullScreen", this->fullScreen);
  obj.set("sidePanelRatio", this->sidePanelRatio);
  obj.set("disableHighRateWarning", this->disableHighRateWarning);
  obj.set("loFreq", this->loFreq);
  obj.set("bandwidth", this->bandwidth);
  obj.set("lastLoadedFile", this->lastLoadedFile);

  obj.setField("source", profileObj);
  obj.setField("analyzerParams", this->analyzerParams.serialize());
  obj.setField("colors", this->colors.serialize());
  obj.setField("audio", this->audioConfig.serialize());
  obj.setField("remoteCtl", this->rcConfig.serialize());
  obj.setField("guiConfig", this->guiConfig.serialize());
  obj.setField("tleSourceConfig", this->tleSourceConfig.serialize());
  obj.setField("panoramicSpectrum", this->panSpectrumConfig->serialize());

  obj.setField("bandPlans", bandPlans);

  componentConfigCopy.copyFrom(this->cachedComponentConfig);
  obj.setField("Components", componentConfigCopy);

  for (auto p : this->enabledBandPlans) {
    Suscan::Object entry(SUSCAN_OBJECT_TYPE_FIELD);
    entry.setValue(p);
    bandPlans.append(entry);
  }

  // Welcome to the world of stupid C++ hacks
  return this->persist(obj);
}

void
AppConfig::loadDefaults(void)
{
  Suscan::Source::Config *config;
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  if ((config = sus->getProfile(SUSCAN_SOURCE_DEFAULT_NAME)) != nullptr) {
    this->profile = *config;
  } else {
    this->profile = Suscan::Source::Config("soapysdr", SUSCAN_SOURCE_FORMAT_AUTO);
    this->profile.setFreq(SUSCAN_SOURCE_DEFAULT_FREQ);
    this->profile.setSampleRate(SUSCAN_SOURCE_DEFAULT_SAMP_RATE);
    this->profile.setBandwidth(SUSCAN_SOURCE_DEFAULT_BANDWIDTH);
  }

}

Suscan::Object
AppConfig::getComponentConfig(const char *obj)
{
  return this->cachedComponentConfig.getField(obj);
}

void
AppConfig::setComponentConfig(const char *field, Suscan::Object const &obj)
{
  if (!obj.isHollow()) {
    Suscan::Object dup;
    dup.copyFrom(obj);
    this->cachedComponentConfig.setField(field, dup);
  }
}

#define TRYSILENT(x) \
  try { x; } catch (Suscan::Exception const &) {}
void
AppConfig::deserialize(Suscan::Object const &conf)
{
  this->loadDefaults();
  if (!conf.isHollow()) {
    TRYSILENT(this->profile = Suscan::Source::Config(conf.getField("source")));
    TRYSILENT(this->analyzerParams.deserialize(conf.getField("analyzerParams")));
    TRYSILENT(this->colors.deserialize(conf.getField("colors")));
    TRYSILENT(this->audioConfig.deserialize(conf.getField("audio")));
    TRYSILENT(this->rcConfig.deserialize(conf.getField("remoteCtl")));
    TRYSILENT(this->guiConfig.deserialize(conf.getField("guiConfig")));
    TRYSILENT(this->tleSourceConfig.deserialize(conf.getField("tleSourceConfig")));
    TRYSILENT(this->panSpectrumConfig->deserialize(conf.getField("panoramicSpectrum")));

    TRYSILENT(this->version    = conf.get("version", SIGDIGGER_UICONFIG_DEFAULT_VERSION));
    TRYSILENT(this->width      = conf.get("width", this->width));
    TRYSILENT(this->height     = conf.get("height", this->height));
    TRYSILENT(this->x          = conf.get("x", this->x));
    TRYSILENT(this->y          = conf.get("y", this->y));
    TRYSILENT(this->fullScreen = conf.get("fullScreen", this->fullScreen));
    TRYSILENT(this->sidePanelRatio = conf.get("sidePanelRatio", this->sidePanelRatio));
    TRYSILENT(this->disableHighRateWarning = conf.get("disableHighRateWarning", this->disableHighRateWarning));
    TRYSILENT(this->loFreq     = conf.get("loFreq", this->loFreq));
    TRYSILENT(this->bandwidth  = conf.get("bandwidth", this->bandwidth));
    TRYSILENT(this->lastLoadedFile = conf.get("lastLoadedFile", this->lastLoadedFile));

    try {
      Suscan::Object set = conf.getField("bandPlans");
      for (unsigned int i = 0; i < set.length(); ++i)
        this->enabledBandPlans.push_back(set[i].value());
    } catch (Suscan::Exception &) {

    }

    try {
      this->cachedComponentConfig.copyFrom(conf.getField("Components"));
      if (this->cachedComponentConfig.getType() != SUSCAN_OBJECT_TYPE_OBJECT)
        this->cachedComponentConfig = Suscan::Object(SUSCAN_OBJECT_TYPE_OBJECT);    
    } catch (Suscan::Exception const &) {
      this->cachedComponentConfig = Suscan::Object(SUSCAN_OBJECT_TYPE_OBJECT);
    }   
  }
}

// OH MY GOD HOLY CHRIST
[[ noreturn ]]
AppConfig::AppConfig(const Suscan::Object &) : AppConfig()
{
  throw Suscan::Exception("Improper call to AppConfig constructor");
}
