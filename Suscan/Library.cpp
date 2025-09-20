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

#define SU_LOG_DOMAIN "sigdigger-library"

#include <Suscan/Library.h>
#include <Suscan/MultitaskController.h>
#include <suscan.h>
#include <analyzer/version.h>
#include <QtGui>
#include <Suscan/Plugin.h>
#include <FeatureFactory.h>
#include <ToolWidgetFactory.h>
#include <TabWidgetFactory.h>
#include <UIListenerFactory.h>
#include <InspectionWidgetFactory.h>
#include <SourceConfigWidgetFactory.h>

using namespace Suscan;

// Singleton initialization
Singleton *Singleton::instance = nullptr;
Logger    *Singleton::logger   = nullptr;

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define STORE_NAME(name, field) obj.set(name, this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)
#define LOAD_NAME(name, field) this->field = conf.get(name, this->field)

void
Location::deserialize(Suscan::Object const &conf)
{
  LOAD(name);
  LOAD(country);
  LOAD_NAME("lat", site.lat);
  LOAD_NAME("lon", site.lon);
  LOAD_NAME("alt", site.height);

  site.height *= 1e-3;
}

Suscan::Object &&
Location::serialize()
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("Location");

  STORE(name);
  STORE(country);
  STORE_NAME("lat", site.lat);
  STORE_NAME("lon", site.lon);
  STORE_NAME("alt", site.height * 1e3);

  return this->persist(obj);
}

void
TLESource::deserialize(Suscan::Object const &conf)
{
  LOAD(name);
  LOAD(url);
}

Suscan::Object &&
TLESource::serialize()
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("tle_source");

  STORE(name);
  STORE(url);

  return this->persist(obj);
}

Singleton::Singleton()
{
  suscan_sigutils_init(SUSCAN_MODE_NOLOG);
  
  this->sources_initd = false;
  this->estimators_initd = false;
  this->spectrum_sources_initd = false;
  this->inspectors_initd = false;
  this->backgroundTaskController = new MultitaskController;

  // Define some read-only units. We may let the user add customized
  // units too.

  this->registerSpectrumUnit("dBFS/Hz",  1.0, 0.0f);
  this->registerSpectrumUnit("dBK",      1.0, -228.60f);
  this->registerSpectrumUnit("dBW/Hz",   1.0, 0.0f);
  this->registerSpectrumUnit("dBm/Hz",   1.0, -30.0f);

  this->registerSpectrumUnit("dBJy",     1.0, 0.0f);

  // The zero point of the AB magnitude scale is at 3631 Jy. This is,
  // at 35.6 dB above the zero point of the dBJy scale. Since 1 mag = -4 dB,
  // the zero point of the scale exactly at -8.9 mag w.r.t the zero point of the
  // dBJy scale.
  this->registerSpectrumUnit("mag (AB)", -4.0, -2.5f * SU_LOG(3631.f));

  this->logger = Logger::getInstance();
}

Singleton::~Singleton()
{
  this->killBackgroundTaskController();
}

Singleton *
Singleton::get_instance()
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

std::string
Singleton::sigutilsVersion()
{
  return std::string(sigutils_api_version())
      + " (" + std::string(sigutils_pkgversion()) + ")";
}

std::string
Singleton::suscanVersion()
{
  return std::string(suscan_api_version())
      + " (" + std::string(suscan_pkgversion()) + ")";
}

// Initialization methods
static SUBOOL
walk_all_sources(suscan_source_config_t *config, void *privdata)
{
  Singleton *instance = static_cast<Singleton *>(privdata);

  instance->registerSourceConfig(config);

  return SU_TRUE;
}

void
Singleton::init_sources()
{
  if (!this->sources_initd) {
    SU_ATTEMPT(suscan_init_sources());
    suscan_source_config_walk(walk_all_sources, static_cast<void *>(this));
    this->sources_initd = true;
  }
}

void
Singleton::init_estimators()
{
  if (!this->estimators_initd) {
    SU_ATTEMPT(suscan_init_estimators());
    this->estimators_initd = true;
  }
}

void
Singleton::init_spectrum_sources()
{
  if (!this->spectrum_sources_initd) {
    SU_ATTEMPT(suscan_init_spectsrcs());
    this->spectrum_sources_initd = true;
  }
}

void
Singleton::init_inspectors()
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
Singleton::init_palettes()
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
Singleton::init_autogains()
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
Singleton::init_fats()
{
  unsigned int i, count;
  ConfigContext ctx("frequency_allocations");
  Object list = ctx.listObject();

  ctx.setSave(false);

  count = list.length();

  for (i = 0; i < count; ++i) {
    try {
      if (!this->haveFAT(list[i].getField("name").value()))
        this->FATs.push_back(list[i]);
    } catch (Suscan::Exception const &) { }
  }
}

void
Singleton::init_bookmarks()
{
  unsigned int i, count;
  ConfigContext ctx("bookmarks");
  Object list = ctx.listObject();
  qreal freq;

  ctx.setSave(true);

  count = list.length();

  for (i = 0; i < count; ++i) {
    try {
      Bookmark bm;
      std::string frequency = list[i].getField("frequency").value();

      bm.info.name = QString::fromStdString(list[i].getField("name").value());
      bm.info.color = QColor(QString::fromStdString(list[i].getField("color").value()));

      try {
        // try parse extended informations
        std::string lowFreqCut = list[i].getField("low_freq_cut").value();
        std::string highFreqCut = list[i].getField("high_freq_cut").value();
        bm.info.modulation = QString::fromStdString(list[i].getField("modulation").value());

        (void) sscanf(lowFreqCut.c_str(), "%d", &bm.info.lowFreqCut);
        (void) sscanf(highFreqCut.c_str(), "%d", &bm.info.highFreqCut);

      } catch (Suscan::Exception const &) { }

      if (sscanf(frequency.c_str(), "%lg", &freq) == 1 && bm.info.name.size() > 0) {
        bm.info.frequency = static_cast<qint64>(freq);
        bm.entry = static_cast<int>(i);
        this->bookmarks[bm.info.frequency] = bm;
      }

    } catch (Suscan::Exception const &) { }
  }
}

void
Singleton::initLocationsFromContext(ConfigContext &ctx, bool user)
{
  Object list = ctx.listObject();
  unsigned int i, count;
  Location loc;

  count = list.length();

  loc.userLocation = user;
  loc.site.height  = 0;

  for (i = 0; i < count; ++i) {
    loc.deserialize(list[i]);
    this->locations[loc.getLocationName()] = loc;
  }
}

void
Singleton::init_locations()
{
  ConfigContext globalCtx("locations");
  ConfigContext userCtx("user_locations");
  ConfigContext qthCtx("qth");
  Object list = qthCtx.listObject();

  globalCtx.setSave(false);
  userCtx.setSave(true);
  qthCtx.setSave(true);

  initLocationsFromContext(globalCtx, false);
  initLocationsFromContext(userCtx, true);

  if (list.length() > 0) {
    if (list[0].getType() == SUSCAN_OBJECT_TYPE_OBJECT
        && list[0].getClass() == "Location") {
      this->qth.deserialize(list[0]);
      this->have_qth = true;
    }
  }
}

void
Singleton::initTLESourcesFromContext(ConfigContext &ctx, bool user)
{
  Object list = ctx.listObject();
  unsigned int i, count;
  TLESource src;

  count = list.length();

  src.user = user;

  for (i = 0; i < count; ++i) {
    src.deserialize(list[i]);
    this->tleSources[src.name] = src;
  }
}

void
Singleton::init_tle_sources()
{
  ConfigContext globalCtx("tle");
  ConfigContext userCtx("user_tle");

  globalCtx.setSave(false);
  userCtx.setSave(true);

  initTLESourcesFromContext(globalCtx, false);
  initTLESourcesFromContext(userCtx, true);
}


void
Singleton::init_tle()
{
  const char *userTLEDir;

  if ((userTLEDir = suscan_confdb_get_local_tle_path()) != nullptr) {
    QDirIterator it(userTLEDir, QDirIterator::NoIteratorFlags);

    while (it.hasNext()) {
      QFile f(it.next());
      QFileInfo fi(f);

      if (fi.completeSuffix().toLower() == "tle") {
        Orbit orbit;
        if (orbit.loadFromFile(f.fileName().toStdString().c_str()))
          this->satellites[orbit.nameToQString()] = orbit;
      }
    }
  }
}

void
Singleton::init_plugins()
{
  Plugin *defPlug = Plugin::getDefaultPlugin();

  if (defPlug == nullptr)
    throw Exception(
        "Failed to load the default plugin. "
        "Please report this error to "
        "<a href=\"https://github.com/BatchDrake/SigDigger/issues\">"
        "https://github.com/BatchDrake/SigDigger/issues"
        "</a>");

  if (!Plugin::registerSigDiggerPluginService())
    throw Exception(
        "Failed to register SigDigger plugin service. "
        "Please report this error to "
        "<a href=\"https://github.com/BatchDrake/SigDigger/issues\">"
        "https://github.com/BatchDrake/SigDigger/issues"
        "</a>");

  if (!suscan_plugin_load_all())
    throw Exception(
        "Failed to load plugins. "
        "Please report this error to "
        "<a href=\"https://github.com/BatchDrake/SigDigger/issues\">"
        "https://github.com/BatchDrake/SigDigger/issues"
        "</a>");
}

bool
Singleton::haveQth() const
{
  return this->have_qth;
}

Location
Singleton::getQth() const
{
  return this->qth;
}

void
Singleton::setQth(Location const &loc)
{
  this->qth = loc;
  this->have_qth = true;
  suscan_set_qth(&loc.site);
}

void
Singleton::detect_devices()
{
  std::string ignored;

  while (Suscan::DeviceFacade::instance()->waitForDevices(ignored, 0));
  Suscan::DeviceFacade::instance()->discoverAll();
}

void
Singleton::trigger_delayed()
{
  for (auto p : pluginCallbacks)
    (p.first)(p.second);
}

void
Singleton::init_ui_config()
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
Singleton::init_recent_list()
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
Singleton::syncRecent()
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
Singleton::syncLocations()
{
  ConfigContext ctx("user_locations");
  Object list = ctx.listObject();

  // Save all user locations
  list.clear();

  for (auto p : this->locations) {
    try {
      if (p.userLocation)
        list.append(p.serialize());
    } catch (Suscan::Exception const &) {
      // Don't even bother to warn
    }
  }

  // Save QTH, if defined
  if (this->have_qth) {
    ConfigContext ctx("qth");
    Object list = ctx.listObject();
    list.clear();
    list.append(this->qth.serialize());
  }
}

void
Singleton::syncTLESources()
{
  ConfigContext ctx("user_tle");
  Object list = ctx.listObject();

  // Save all user TLE sources
  list.clear();

  for (auto p : this->tleSources) {
    try {
      if (p.user)
        list.append(p.serialize());
    } catch (Suscan::Exception const &) {
      // Don't even bother to warn
    }
  }
}

void
Singleton::syncUI()
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
Singleton::syncBookmarks()
{
  ConfigContext ctx("bookmarks");
  Object list = ctx.listObject();

  // Sync all modified configurations
  for (auto p : this->bookmarks.keys()) {
    if (this->bookmarks[p].entry == -1) {
      try {
        Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

        obj.set("name", this->bookmarks[p].info.name.toStdString());
        obj.set("frequency", static_cast<double>(this->bookmarks[p].info.frequency));
        obj.set("color", this->bookmarks[p].info.color.name().toStdString());
        obj.set("low_freq_cut", this->bookmarks[p].info.lowFreqCut);
        obj.set("high_freq_cut", this->bookmarks[p].info.highFreqCut);
        obj.set("modulation", this->bookmarks[p].info.modulation.toStdString());

        list.append(std::move(obj));
      } catch (Suscan::Exception const &) {
      }
    }
  }
}

void
Singleton::killBackgroundTaskController()
{
  if (this->backgroundTaskController != nullptr) {
    delete this->backgroundTaskController;
    this->backgroundTaskController = nullptr;
  }
}

void
Singleton::sync()
{
  this->syncRecent();
  this->syncUI();
  this->syncBookmarks();
  this->syncLocations();
  this->syncTLESources();
}

// Singleton methods
void
Singleton::registerDelayedCallback(DelayedPluginCallback cb, Plugin *plugin)
{
  pluginCallbacks.push_back(std::pair<DelayedPluginCallback, Plugin *>(cb, plugin));
}

void
Singleton::registerSourceConfig(suscan_source_config_t *config)
{
  const char *label = suscan_source_config_get_label(config);

  if (label == nullptr)
    label = "(Null profile)";

  this->profiles[label] = Suscan::Source::Config(config);
}

MultitaskController *
Singleton::getBackgroundTaskController() const
{
  return this->backgroundTaskController;
}

ConfigMap::const_iterator
Singleton::getFirstProfile() const
{
  return this->profiles.begin();
}

ConfigMap::const_iterator
Singleton::getLastProfile() const
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
          this->profiles[profile.label()].instance()));
}

void
Singleton::removeBookmark(qint64 freq)
{
  if (this->bookmarks.find(freq) != this->bookmarks.end()) {
    Bookmark bm = this->bookmarks[freq];
    this->bookmarks.remove(freq);

    if (bm.entry != -1) {
      ConfigContext ctx("bookmarks");
      Object list = ctx.listObject();
      list.remove(static_cast<unsigned>(bm.entry));
    }
  }
}

void
Singleton::replaceBookmark(BookmarkInfo const& info)
{
  Bookmark bm;

  bm.info = info;

  this->removeBookmark(info.frequency);
  this->bookmarks[info.frequency] = bm;
}

bool
Singleton::registerBookmark(BookmarkInfo const& info)
{
  if (this->bookmarks.find(info.frequency) != this->bookmarks.end())
    return false;

  Bookmark bm;

  bm.info = info;
  this->bookmarks[info.frequency] = bm;

  return true;
}

bool
Singleton::registerLocation(Location const& loc)
{
  if (this->locations.find(loc.getLocationName()) != this->locations.end())
    return false;

  Location newLoc;

  newLoc = loc;
  newLoc.userLocation = true;

  this->locations[newLoc.getLocationName()] = newLoc;

  return true;
}

QString
Singleton::normalizeTLEName(QString const &name)
{
  QString normalized = name;

  return name.trimmed().replace(
        QRegularExpression(QStringLiteral("[^-a-zA-Z0-9()]")),
        "_");
}

bool
Singleton::registerTLE(std::string const &tleData)
{
  Orbit newOrbit;
  const char *userTLEDir;
  QString fullTLEPath;

  if (newOrbit.loadFromTLE(tleData)) {
    // Valid TLE file, overwrite current TLE
    if ((userTLEDir = suscan_confdb_get_local_tle_path()) == nullptr)
      return false;

    fullTLEPath =
        userTLEDir + QString("/")
        + normalizeTLEName(newOrbit.nameToQString()) + ".tle";

    // Attempt to save it. If we could
    QFile qFile(fullTLEPath);
    if (qFile.open(QIODevice::WriteOnly)) {
      bool ok;
      QTextStream out(&qFile);
      out << QString::fromStdString(tleData);
      out.flush();

      ok = !qFile.error();

      qFile.close();

      if (ok) {
        this->satellites[newOrbit.nameToQString()] = newOrbit;
        return true;
      }
    }
  }

  return false;
}

bool
Singleton::registerTLESource(TLESource const& tleSrc)
{
  if (this->tleSources.find(tleSrc.name) != this->tleSources.end())
    return false;

  TLESource newSrc;

  newSrc = tleSrc;
  newSrc.user = true;

  this->tleSources[newSrc.name] = newSrc;

  return true;
}

bool
Singleton::removeTLESource(std::string const &name)
{
  if (this->tleSources.find(name) == this->tleSources.end())
    return false;

  // Non-user TLEs cannot be removed
  if (!this->tleSources[name].user)
    return false;

  this->tleSources.remove(name);

  return true;
}

bool
Singleton::registerSpectrumUnit(
    std::string const &name,
    float dBPerUnit,
    float zeroPoint)
{
  if (this->spectrumUnits.find(name) != this->spectrumUnits.end())
    return false;

  SpectrumUnit su;

  su.name      = name;
  su.dBPerUnit = dBPerUnit;
  su.zeroPoint = zeroPoint;

  this->spectrumUnits[name] = su;

  return true;
}

void
Singleton::replaceSpectrumUnit(
    std::string const &name,
    float dBPerUnit,
    float zeroPoint)
{
  SpectrumUnit su;

  su.name      = name;
  su.dBPerUnit = dBPerUnit;
  su.zeroPoint = zeroPoint;

  this->removeSpectrumUnit(name);
  this->spectrumUnits[name] = su;
}

void
Singleton::removeSpectrumUnit(std::string const &name)
{
  if (this->spectrumUnits.find(name) != this->spectrumUnits.end()) {
    SpectrumUnit bm = this->spectrumUnits[name];
    this->spectrumUnits.remove(name);
  }
}

std::vector<Object>::const_iterator
Singleton::getFirstPalette() const
{
  return this->palettes.begin();
}

std::vector<Object>::const_iterator
Singleton::getLastPalette() const
{
  return this->palettes.end();
}

std::vector<Object>::const_iterator
Singleton::getFirstAutoGain() const
{
  return this->autoGains.begin();
}

std::vector<Object>::const_iterator
Singleton::getLastAutoGain() const
{
  return this->autoGains.end();
}

std::vector<Object>::iterator
Singleton::getFirstUIConfig()
{
  return this->uiConfig.begin();
}

std::vector<Object>::iterator
Singleton::getLastUIConfig()
{
  return this->uiConfig.end();
}

std::vector<Object>::const_iterator
Singleton::getFirstFAT() const
{
  return this->FATs.begin();
}

std::vector<Object>::const_iterator
Singleton::getLastFAT() const
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

std::list<std::string>::const_iterator
Singleton::getFirstRecent() const
{
  return this->recentProfiles.cbegin();
}

std::list<std::string>::const_iterator
Singleton::getLastRecent() const
{
  return this->recentProfiles.cend();
}

QMap<qint64,Bookmark> const &
Singleton::getBookmarkMap() const
{
  return this->bookmarks;
}

QMap<qint64,Bookmark>::const_iterator
Singleton::getFirstBookmark() const
{
  return this->bookmarks.cbegin();
}

QMap<qint64,Bookmark>::const_iterator
Singleton::getLastBookmark() const
{
  return this->bookmarks.cend();
}

QMap<qint64,Bookmark>::const_iterator
Singleton::getBookmarkFrom(qint64 freq) const
{
  return this->bookmarks.lowerBound(freq);
}

QMap<QString, Location> const &
Singleton::getLocationMap() const
{
  return this->locations;
}

QMap<QString, Location>::const_iterator
Singleton::getFirstLocation() const
{
  return this->locations.cbegin();
}

QMap<QString, Location>::const_iterator
Singleton::getLastLocation() const
{
  return this->locations.cend();
}

QMap<QString, Orbit> const &
Singleton::getSatelliteMap() const
{
  return this->satellites;
}


QMap<QString, Orbit>::const_iterator
Singleton::getFirstSatellite() const
{
  return this->satellites.cbegin();
}

QMap<QString, Orbit>::const_iterator
Singleton::getLastSatellite() const
{
return this->satellites.cend();
}

QMap<std::string, TLESource> const &
Singleton::getTLESourceMap() const
{
  return this->tleSources;
}

QMap<std::string, TLESource>::const_iterator
Singleton::getFirstTLESource() const
{
  return this->tleSources.cbegin();
}

QMap<std::string, TLESource>::const_iterator
Singleton::getLastTLESource() const
{
  return this->tleSources.cend();
}

QMap<std::string, SpectrumUnit> const &
Singleton::getSpectrumUnitMap() const
{
  return this->spectrumUnits;
}

QMap<std::string, SpectrumUnit>::const_iterator
Singleton::getFirstSpectrumUnit() const
{
  return this->spectrumUnits.cbegin();
}

QMap<std::string, SpectrumUnit>::const_iterator
Singleton::getLastSpectrumUnit() const
{
  return this->spectrumUnits.cend();
}

QMap<std::string, SpectrumUnit>::const_iterator
Singleton::getSpectrumUnitFrom(std::string const &name) const
{
  return this->spectrumUnits.lowerBound(name);
}

bool
Singleton::registerToolWidgetFactory(SigDigger::ToolWidgetFactory *factory)
{
  // Not a bug. The plugin went ahead of ourselves.
  if (this->toolWidgetFactories.contains(factory))
    return true;

  this->toolWidgetFactories.push_back(factory);

  return true;
}

bool
Singleton::unregisterToolWidgetFactory(SigDigger::ToolWidgetFactory *factory)
{
  int index = this->toolWidgetFactories.indexOf(factory);

  if (index == -1)
    return false;

  this->toolWidgetFactories.removeAt(index);

  return true;
}

QList<SigDigger::ToolWidgetFactory *>::const_iterator
Singleton::getFirstToolWidgetFactory() const
{
  return this->toolWidgetFactories.begin();
}

QList<SigDigger::ToolWidgetFactory *>::const_iterator
Singleton::getLastToolWidgetFactory() const
{
  return this->toolWidgetFactories.end();
}

bool
Singleton::registerToolBarWidgetFactory(SigDigger::ToolBarWidgetFactory *factory)
{
  // Not a bug. The plugin went ahead of ourselves.
  if (this->toolBarWidgetFactories.contains(factory))
    return true;

  this->toolBarWidgetFactories.push_back(factory);

  return true;
}

bool
Singleton::unregisterToolBarWidgetFactory(SigDigger::ToolBarWidgetFactory *factory)
{
  int index = this->toolBarWidgetFactories.indexOf(factory);

  if (index == -1)
    return false;

  this->toolBarWidgetFactories.removeAt(index);

  return true;
}

QList<SigDigger::ToolBarWidgetFactory *>::const_iterator
Singleton::getFirstToolBarWidgetFactory() const
{
  return this->toolBarWidgetFactories.begin();
}

QList<SigDigger::ToolBarWidgetFactory *>::const_iterator
Singleton::getLastToolBarWidgetFactory() const
{
  return this->toolBarWidgetFactories.end();
}


bool
Singleton::registerTabWidgetFactory(SigDigger::TabWidgetFactory *factory)
{
  // Not a bug. The plugin went ahead of ourselves.
  if (this->tabWidgetFactories.contains(factory))
    return true;

  this->tabWidgetFactories.push_back(factory);
  this->tabWidgetFactoryTable[factory->name()] = factory;

  return true;
}

bool
Singleton::unregisterTabWidgetFactory(SigDigger::TabWidgetFactory *factory)
{
  int index = this->tabWidgetFactories.indexOf(factory);

  if (index == -1)
    return false;

  this->tabWidgetFactories.removeAt(index);
  this->tabWidgetFactoryTable.remove(factory->name());

  return true;
}

SigDigger::TabWidgetFactory *
Singleton::findTabWidgetFactory(QString const &name) const
{
  if (!this->tabWidgetFactoryTable.contains(name))
    return nullptr;

  return this->tabWidgetFactoryTable[name];
}

QList<SigDigger::TabWidgetFactory *>::const_iterator
Singleton::getFirstTabWidgetFactory() const
{
  return this->tabWidgetFactories.begin();
}

QList<SigDigger::TabWidgetFactory *>::const_iterator
Singleton::getLastTabWidgetFactory() const
{
  return this->tabWidgetFactories.end();
}

bool
Singleton::registerInspectionWidgetFactory(SigDigger::InspectionWidgetFactory *factory)
{
  // Not a bug. The plugin went ahead of ourselves.
  if (this->inspectionWidgetFactories.contains(factory))
    return true;

  this->inspectionWidgetFactories.push_back(factory);
  this->inspectionWidgetFactoryTable[factory->name()] = factory;

  return true;
}

bool
Singleton::unregisterInspectionWidgetFactory(SigDigger::InspectionWidgetFactory *factory)
{
  int index = this->inspectionWidgetFactories.indexOf(factory);

  if (index == -1)
    return false;

  this->inspectionWidgetFactories.removeAt(index);
  this->inspectionWidgetFactoryTable.remove(factory->name());

  return true;
}

SigDigger::InspectionWidgetFactory *
Singleton::findInspectionWidgetFactory(QString const &name) const
{
  if (!this->inspectionWidgetFactoryTable.contains(name))
    return nullptr;

  return this->inspectionWidgetFactoryTable[name];
}

QList<SigDigger::InspectionWidgetFactory *>::const_iterator
Singleton::getFirstInspectionWidgetFactory() const
{
  return this->inspectionWidgetFactories.begin();
}

QList<SigDigger::InspectionWidgetFactory *>::const_iterator
Singleton::getLastInspectionWidgetFactory() const
{
  return this->inspectionWidgetFactories.end();
}

bool
Singleton::registerSourceConfigWidgetFactory(SigDigger::SourceConfigWidgetFactory *factory)
{
  // Not a bug. The plugin went ahead of ourselves.
  if (this->sourceConfigWidgetFactories.contains(factory))
    return true;

  this->sourceConfigWidgetFactories.push_back(factory);
  this->sourceConfigWidgetFactoryTable[factory->name()] = factory;

  return true;
}

bool
Singleton::unregisterSourceConfigWidgetFactory(SigDigger::SourceConfigWidgetFactory *factory)
{
  int index = this->sourceConfigWidgetFactories.indexOf(factory);

  if (index == -1)
    return false;

  this->sourceConfigWidgetFactories.removeAt(index);
  this->sourceConfigWidgetFactoryTable.remove(factory->name());

  return true;
}

SigDigger::SourceConfigWidgetFactory *
Singleton::findSourceConfigWidgetFactory(QString const &name) const
{
  if (!this->sourceConfigWidgetFactoryTable.contains(name))
    return nullptr;

  return this->sourceConfigWidgetFactoryTable[name];
}

QList<SigDigger::SourceConfigWidgetFactory *>::const_iterator
Singleton::getFirstSourceConfigWidgetFactory() const
{
  return this->sourceConfigWidgetFactories.begin();
}

QList<SigDigger::SourceConfigWidgetFactory *>::const_iterator
Singleton::getLastSourceConfigWidgetFactory() const
{
  return this->sourceConfigWidgetFactories.end();
}


bool
Singleton::registerUIListenerFactory(SigDigger::UIListenerFactory *factory)
{
  // Not a bug. The plugin got ahead of ourselves.
  if (this->uiListenerFactories.contains(factory))
    return true;

  this->uiListenerFactories.push_back(factory);

  return true;
}

bool
Singleton::unregisterUIListenerFactory(SigDigger::UIListenerFactory *factory)
{
  int index = this->uiListenerFactories.indexOf(factory);

  if (index == -1)
    return false;

  this->uiListenerFactories.removeAt(index);

  return true;
}

QList<SigDigger::UIListenerFactory *>::const_iterator
Singleton::getFirstUIListenerFactory() const
{
  return this->uiListenerFactories.begin();
}

QList<SigDigger::UIListenerFactory *>::const_iterator
Singleton::getLastUIListenerFactory() const
{
  return this->uiListenerFactories.end();
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
Singleton::clearRecent()
{
  this->recentProfiles.clear();
}
