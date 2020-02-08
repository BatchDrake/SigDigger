//
//    Library.cpp: C-level Suscan API initialization
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

#include <Suscan/Library.h>

using namespace Suscan;

// Singleton initialization
Singleton *Singleton::instance = nullptr;
Logger    *Singleton::logger   = nullptr;

Singleton::Singleton()
{
  this->codecs_initd = false;
  this->sources_initd = false;
  this->estimators_initd = false;
  this->spectrum_sources_initd = false;
  this->inspectors_initd = false;

  this->logger = Logger::getInstance();
}

Singleton::~Singleton()
{

}

Singleton *
Singleton::get_instance(void)
{
  if (Singleton::instance == nullptr) {
    try {
      Singleton::instance = new Singleton();
    } catch (std::bad_alloc&) {
      throw Exception("Failed to allocate Suscan's singleton");
    }
  }

  return Singleton::instance;
}

// Initialization methods
void
Singleton::init_codecs(void)
{
  if (!this->codecs_initd)
    SU_ATTEMPT(suscan_codec_class_register_builtin());
}

static SUBOOL
walk_all_sources(suscan_source_config_t *config, void *privdata)
{
  Singleton *instance = static_cast<Singleton *>(privdata);

  instance->registerSourceConfig(config);

  return SU_TRUE;
}

static SUBOOL
walk_all_devices(const suscan_source_device_t *device, unsigned int, void *privdata)
{
  Singleton *instance = static_cast<Singleton *>(privdata);

  instance->registerSourceDevice(device);

  return SU_TRUE;
}

void
Singleton::init_sources(void)
{
  if (!this->sources_initd) {
    SU_ATTEMPT(suscan_init_sources());
    suscan_source_config_walk(walk_all_sources, static_cast<void *>(this));
    suscan_source_device_walk(walk_all_devices, static_cast<void *>(this));
    this->sources_initd = true;
  }
}

void
Singleton::init_estimators(void)
{
  if (!this->estimators_initd) {
    SU_ATTEMPT(suscan_init_estimators());
    this->estimators_initd = true;
  }
}

void
Singleton::init_spectrum_sources(void)
{
  if (!this->spectrum_sources_initd) {
    SU_ATTEMPT(suscan_init_spectsrcs());
    this->spectrum_sources_initd = true;
  }
}

void
Singleton::init_inspectors(void)
{
  if (!this->inspectors_initd) {
    SU_ATTEMPT(suscan_init_inspectors());
    this->inspectors_initd = true;
  }
}

bool
Singleton::haveAutoGain(std::string const &name)
{
  for (auto p = this->autoGains.begin();
       p != this->autoGains.end();
       ++p) {
    try {
      if (p->getField("name").value() == name)
        return true;
    } catch (Suscan::Exception const &) {

    }
  }

  return false;
}

bool
Singleton::haveFAT(std::string const &name)
{
  for (auto p = this->FATs.begin();
       p != this->FATs.end();
       ++p) {
    try {
      if (p->getField("name").value() == name)
        return true;
    } catch (Suscan::Exception const &) {

    }
  }

  return false;
}

bool
Singleton::havePalette(std::string const &name)
{
  for (auto p = this->palettes.begin();
       p != this->palettes.end();
       ++p) {
    try {
      if (p->getField("name").value() == name)
        return true;
    } catch (Suscan::Exception const &) {

    }
  }

  return false;
}

void
Singleton::init_palettes(void)
{
  unsigned int i, count;
  ConfigContext ctx("palettes");
  Object list = ctx.listObject();

  ctx.setSave(false);

  count = list.length();

  for (i = 0; i < count; ++i) {
    try {
      if (!this->havePalette(list[i].getField("name").value()))
        this->palettes.push_back(list[i]);
    } catch (Suscan::Exception const &) { }
  }
}

void
Singleton::init_autogains(void)
{
  unsigned int i, count;
  ConfigContext ctx("autogains");
  Object list = ctx.listObject();

  ctx.setSave(false);

  count = list.length();

  for (i = 0; i < count; ++i) {
    try {
      if (!this->haveAutoGain(list[i].getField("name").value()))
        this->autoGains.push_back(list[i]);
    } catch (Suscan::Exception const &) { }
  }
}

void
Singleton::init_fats(void)
{
  unsigned int i, count;
  ConfigContext ctx("frequency_allocations");
  Object list = ctx.listObject();

  count = list.length();

  for (i = 0; i < count; ++i) {
    try {
      if (!this->haveFAT(list[i].getField("name").value()))
        this->FATs.push_back(list[i]);
    } catch (Suscan::Exception const &) { }
  }
}

void
Singleton::detect_devices(void)
{
  suscan_source_detect_devices();
  this->devices.clear();
  suscan_source_device_walk(walk_all_devices, static_cast<void *>(this));
}

void
Singleton::init_ui_config(void)
{
  unsigned int i, count;
  ConfigContext ctx("uiconfig");

  Object list = ctx.listObject();

  count = list.length();

  for (i = 0; i < count; ++i) {
    try {
      this->uiConfig.push_back(list[i]);
    } catch (Suscan::Exception const &) { }
  }
}

void
Singleton::init_recent_list(void)
{
  unsigned int i, count;
  ConfigContext ctx("recent");

  Object list = ctx.listObject();

  count = list.length();

  for (i = 0; i < count; ++i) {
    try {
      if (list[i].getType() == SUSCAN_OBJECT_TYPE_FIELD) {
        this->recentProfiles.push_back(list[i].value());
      }
    } catch (Suscan::Exception const &) { }
  }
}

void
Singleton::syncRecent(void)
{
  ConfigContext ctx("recent");
  Object list = ctx.listObject();

  list.clear();

  for (auto p : this->recentProfiles) {
    try {
      list.append(Object::makeField(p));
    } catch (Suscan::Exception const &) {
      // Don't even bother to warn
    }
  }
}

void
Singleton::syncUI(void)
{
  unsigned int i, count;
  ConfigContext ctx("uiconfig");
  Object list = ctx.listObject();

  count = static_cast<unsigned int>(this->uiConfig.size());

  // Sync all modified configurations
  for (i = 0; i < count; ++i) {
    if (!this->uiConfig[i].isBorrowed()) {
      try {
        list.put(this->uiConfig[i], i);
      } catch (Suscan::Exception const &) {
        list.append(this->uiConfig[i]);
      }
    }
  }
}

void
Singleton::sync(void)
{
  this->syncRecent();
  this->syncUI();
}

// Singleton methods
void
Singleton::registerSourceConfig(suscan_source_config_t *config)
{
  const char *label = suscan_source_config_get_label(config);

  if (label == nullptr)
    label = "(Null profile)";

  this->profiles[label] = Suscan::Source::Config(config);
}

ConfigMap::const_iterator
Singleton::getFirstProfile(void) const
{
  return this->profiles.begin();
}

ConfigMap::const_iterator
Singleton::getLastProfile(void) const
{
  return this->profiles.end();
}

Suscan::Source::Config *
Singleton::getProfile(std::string const &name)
{
  if (this->profiles.find(name) == this->profiles.end())
    return nullptr;

  return &this->profiles[name];
}

void
Singleton::saveProfile(Suscan::Source::Config const &profile)
{
  this->profiles[profile.label()] = profile;

  SU_ATTEMPT(
        suscan_source_config_register(
          this->profiles[profile.label()].instance));
}

void
Singleton::registerSourceDevice(const suscan_source_device_t *dev)
{
  this->devices.push_back(Source::Device(dev, 0));
}

std::vector<Source::Device>::const_iterator
Singleton::getFirstDevice(void) const
{
  return this->devices.begin();
}

std::vector<Source::Device>::const_iterator
Singleton::getLastDevice(void) const
{
  return this->devices.end();
}

std::vector<Object>::const_iterator
Singleton::getFirstPalette(void) const
{
  return this->palettes.begin();
}

std::vector<Object>::const_iterator
Singleton::getLastPalette(void) const
{
  return this->palettes.end();
}

std::vector<Object>::const_iterator
Singleton::getFirstAutoGain(void) const
{
  return this->autoGains.begin();
}

std::vector<Object>::const_iterator
Singleton::getLastAutoGain(void) const
{
  return this->autoGains.end();
}

std::vector<Object>::iterator
Singleton::getFirstUIConfig(void)
{
  return this->uiConfig.begin();
}

std::vector<Object>::iterator
Singleton::getLastUIConfig(void)
{
  return this->uiConfig.end();
}

std::vector<Object>::const_iterator
Singleton::getFirstFAT(void) const
{
  return this->FATs.begin();
}

std::vector<Object>::const_iterator
Singleton::getLastFAT(void) const
{
  return this->FATs.end();
}

void
Singleton::putUIConfig(unsigned int pos, Object &&rv)
{
  if (pos >= this->uiConfig.size())
    this->uiConfig.resize(pos + 1);

  this->uiConfig[pos] = std::move(rv);
}

const Source::Device *
Singleton::getDeviceAt(unsigned int index) const
{
  if (index < this->devices.size())
    return &this->devices[index];

  return nullptr;
}

std::list<std::string>::const_iterator
Singleton::getFirstRecent(void) const
{
  return this->recentProfiles.cbegin();
}

std::list<std::string>::const_iterator
Singleton::getLastRecent(void) const
{
  return this->recentProfiles.cend();
}

bool
Singleton::notifyRecent(std::string const &name)
{
  bool found = false;

  found = this->removeRecent(name);

  this->recentProfiles.push_front(name);

  return found;
}

bool
Singleton::removeRecent(std::string const &name)
{
  bool found = false;

  for (auto p = this->getFirstRecent(); p != this->getLastRecent(); ++p) {
    if (*p == name) {
      auto current = p;
      --current;

      this->recentProfiles.erase(p);

      p = current;
      found = true;
      continue;
    }
  }

  return found;
}

void
Singleton::clearRecent(void)
{
  this->recentProfiles.clear();
}
