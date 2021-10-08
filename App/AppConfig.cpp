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

using namespace SigDigger;

AppConfig::AppConfig(AppUI *ui)
  : profile{},
  analyzerParams{},
  colors{},
  guiConfig{},
  enabledBandPlans{}
{
  this->sourceConfig      = ui->sourcePanel->getConfig();
  this->fftConfig         = ui->fftPanel->getConfig();
  this->inspectorConfig   = ui->inspectorPanel->getConfig();
  this->audioConfig       = ui->audioPanel->getConfig();
  this->panSpectrumConfig = ui->panoramicDialog->getConfig();
}

Suscan::Object &&
AppConfig::serialize(void)
{
  Suscan::Object profileObj = this->profile.serialize();
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);
  Suscan::Object bandPlans(SUSCAN_OBJECT_TYPE_SET);

  obj.setClass("qtui");

  obj.set("version", this->version);
  obj.set("width", this->width);
  obj.set("height", this->height);
  obj.set("x", this->x);
  obj.set("y", this->y);
  obj.set("fullScreen", this->fullScreen);
  obj.set("disableHighRateWarning", this->disableHighRateWarning);
  obj.set("loFreq", this->loFreq);
  obj.set("bandwidth", this->bandwidth);

  obj.setField("source", profileObj);
  obj.setField("analyzerParams", this->analyzerParams.serialize());
  obj.setField("colors", this->colors.serialize());
  obj.setField("guiConfig", this->guiConfig.serialize());
  obj.setField("tleSourceConfig", this->tleSourceConfig.serialize());
  obj.setField("sourcePanel", this->sourceConfig->serialize());
  obj.setField("fftPanel", this->fftConfig->serialize());
  obj.setField("audioPanel", this->audioConfig->serialize());
  obj.setField("inspectorPanel", this->inspectorConfig->serialize());
  obj.setField("panoramicSpectrum", this->panSpectrumConfig->serialize());

  obj.setField("bandPlans", bandPlans);

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
    this->profile = Suscan::Source::Config(
          SUSCAN_SOURCE_TYPE_SDR,
          SUSCAN_SOURCE_FORMAT_AUTO);
    this->profile.setFreq(SUSCAN_SOURCE_DEFAULT_FREQ);
    this->profile.setSampleRate(SUSCAN_SOURCE_DEFAULT_SAMP_RATE);
    this->profile.setBandwidth(SUSCAN_SOURCE_DEFAULT_BANDWIDTH);
  }

}

#define TRYSILENT(x) \
  try { x; } catch (Suscan::Exception const &e) { printf ("Error: %s\n", e.what()); }
void
AppConfig::deserialize(Suscan::Object const &conf)
{
  this->loadDefaults();
  if (!conf.isHollow()) {
    TRYSILENT(this->profile = Suscan::Source::Config(conf.getField("source")));
    TRYSILENT(this->analyzerParams.deserialize(conf.getField("analyzerParams")));
    TRYSILENT(this->colors.deserialize(conf.getField("colors")));
    TRYSILENT(this->guiConfig.deserialize(conf.getField("guiConfig")));
    TRYSILENT(this->tleSourceConfig.deserialize(conf.getField("tleSourceConfig")));
    TRYSILENT(this->sourceConfig->deserialize(conf.getField("sourcePanel")));
    TRYSILENT(this->fftConfig->deserialize(conf.getField("fftPanel")));
    TRYSILENT(this->audioConfig->deserialize(conf.getField("audioPanel")));
    TRYSILENT(this->inspectorConfig->deserialize(conf.getField("inspectorPanel")));
    TRYSILENT(this->panSpectrumConfig->deserialize(conf.getField("panoramicSpectrum")));

    TRYSILENT(this->version    = conf.get("version", SIGDIGGER_UICONFIG_DEFAULT_VERSION));
    TRYSILENT(this->width      = conf.get("width", this->width));
    TRYSILENT(this->height     = conf.get("height", this->height));
    TRYSILENT(this->x          = conf.get("x", this->x));
    TRYSILENT(this->y          = conf.get("y", this->y));
    TRYSILENT(this->fullScreen = conf.get("fullScreen", this->fullScreen));
    TRYSILENT(this->disableHighRateWarning = conf.get("disableHighRateWarning", this->disableHighRateWarning));
    TRYSILENT(this->loFreq     = conf.get("loFreq", this->loFreq));
    TRYSILENT(this->bandwidth  = conf.get("bandwidth", this->bandwidth));

    try {
      Suscan::Object set = conf.getField("bandPlans");
      for (unsigned int i = 0; i < set.length(); ++i)
        this->enabledBandPlans.push_back(set[i].value());
    } catch (Suscan::Exception &) {

    }
  }
}

// OH MY GOD HOLY CHRIST
[[ noreturn ]]
AppConfig::AppConfig(const Suscan::Object &) : AppConfig()
{
  throw Suscan::Exception("Improper call to AppConfig constructor");
}
