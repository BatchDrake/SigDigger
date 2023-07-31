//
//    Library.h: C-level Suscan API initialization
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

#ifndef CPP_LIBRARY_H
#define CPP_LIBRARY_H

#include <vector>

#include <Suscan/Compat.h>
#include <Suscan/Source.h>
#include <Suscan/Logger.h>
#include <Suscan/Config.h>
#include <Suscan/Serializable.h>
#include <SuWidgetsHelpers.h>

#include <analyzer/source.h>
#include <analyzer/estimator.h>
#include <analyzer/spectsrc.h>
#include <analyzer/inspector/inspector.h>
#include <analyzer/discovery.h>
#include <sgdp4/sgdp4.h>
#include <WFHelpers.h>

#include <map>
#include <list>
#include <QMap>

#include <QHash>

namespace SigDigger {
  class ToolWidgetFactory;
  class TabWidgetFactory;
  class InspectionWidgetFactory;
  class SourceConfigWidgetFactory;
  class UIListenerFactory;
};

namespace Suscan {
  uint qHash(const Suscan::Source::Device &dev);

  class MultitaskController;
  class Plugin;

  typedef std::map<std::string, Source::Config> ConfigMap;

  struct Bookmark {
    BookmarkInfo info;

    int entry = -1;
  };

  struct TLESource : public Serializable{
    std::string name;
    std::string url;
    bool user = false;

    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  struct SpectrumUnit {
    std::string name = "dBFS";
    float dBPerUnit  = 1.0f;
    float zeroPoint  = 0.0f;
  };

  struct Location : public Serializable {
    std::string name;
    std::string country;
    xyz_t site = {{0.}, {0.}, {0.}};
    bool userLocation = false;

    inline QString
    getLocationName() const
    {
      return QString::fromStdString(
            this->name + ", " + this->country);
    }

    inline xyz_t
    getQth() const
    {
      xyz_t qth;

      qth.lat    = SU_DEG2RAD(this->site.lat);
      qth.lon    = SU_DEG2RAD(this->site.lon);
      qth.height = this->site.height;

      return qth;
    }

    inline QString
    getGridLocator() const
    {
      QString locator;

      qreal lat = (this->site.lat + 90) / 10;
      qreal lon = (this->site.lon + 180) / 20;

      char laField,  loField;
      char laSquare, loSquare;
      char laSubsq,  loSubsq;

      lat = qBound(0., lat, 18.);
      lon = qBound(0., lon, 18.);

      laField = SCAST(char, floor(lat));
      loField = SCAST(char, floor(lon));

      lat -= laField;
      lon -= loField;

      laSquare = SCAST(char, floor(lat * 10));
      loSquare = SCAST(char, floor(lon * 10));

      lat -= laSquare * 1e-1;
      lon -= loSquare * 1e-1;

      laSubsq  = SCAST(char, floor(lat * 240));
      loSubsq  = SCAST(char, floor(lon * 240));

      locator += QChar('A' + loField);
      locator += QChar('A' + laField);

      locator += QChar('0' + loSquare);
      locator += QChar('0' + laSquare);

      locator += QChar('a' + loSubsq);
      locator += QChar('a' + laSubsq);

      return locator;
    }

    // Overriden methods
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  struct Orbit {
    bool loan = false;
    orbit_t local_info = orbit_INITIALIZER;
    const orbit_t *c_info = nullptr;

    Orbit()
    {
      memset(&local_info, 0, sizeof(orbit_t));
      this->c_info = &this->local_info;
    }

    Orbit(const orbit_t *ptr, bool loan = false)
    {
      this->loan = loan;

      if (loan) {
        this->c_info = ptr;
      } else {
        this->local_info = *ptr;
        if (ptr->name)
          this->local_info.name = strdup(ptr->name);
        this->c_info = &this->local_info;
      }
    }

    Suscan::Orbit&
    operator=(const Suscan::Orbit &lvalue)
    {
      if (!this->loan)
        orbit_finalize(&this->local_info);

      this->local_info = *lvalue.c_info;
      if (lvalue.c_info->name)
        this->local_info.name = strdup(lvalue.c_info->name);
      this->c_info = &this->local_info;

      return *this;
    }

    bool
    loadFromTLE(std::string const &data)
    {
      orbit_t newOrbit = orbit_INITIALIZER;

      if (this->loan)
        return false;

      if (orbit_init_from_data(&newOrbit, data.c_str(), data.size()) > 0) {
        orbit_finalize(&this->local_info);
        this->local_info = newOrbit;

        return true;
      }

      return false;
    }

    bool
    loadFromFile(const char *path)
    {
      orbit_t newOrbit = orbit_INITIALIZER;

      if (this->loan)
        return false;

      if (orbit_init_from_file(&newOrbit, path) > 0) {
        orbit_finalize(&this->local_info);
        this->local_info = newOrbit;

        return true;
      }

      return false;
    }

    Orbit(Orbit const &orbit) : Orbit(orbit.c_info) { }

    ~Orbit()
    {
      if (!this->loan)
        orbit_finalize(&this->local_info);
    }

    orbit_t const &
    getCOrbit(void) const
    {
      return *this->c_info;
    }

    QString
    nameToQString(void) const
    {
      return this->c_info->name;
    }

    void debug(void) const;
  };

  typedef void (*DelayedPluginCallback) (Suscan::Plugin *);

  class Singleton {
    static Singleton *instance;
    static Logger *logger;

    // Background tasks
    MultitaskController *backgroundTaskController = nullptr;
    std::vector<Source::Device> devices;
    ConfigMap profiles;
    std::vector<Object> palettes;
    std::vector<Object> autoGains;
    std::vector<Object> uiConfig;
    std::vector<Object> FATs;

    // Singleton config
    Location                        qth;
    QMap<QString, Orbit>            satellites;
    QMap<QString, Location>         locations;
    QMap<std::string, TLESource>    tleSources;
    QMap<qint64, Bookmark>          bookmarks;
    QMap<std::string, SpectrumUnit> spectrumUnits;
    QHash<QString, Source::Config>  networkProfiles;

    // Delayed plugin callbacks
    std::list<std::pair<DelayedPluginCallback, Suscan::Plugin *>> pluginCallbacks;

    // Feature object factories
    QList<SigDigger::ToolWidgetFactory *>         toolWidgetFactories;
    QList<SigDigger::TabWidgetFactory *>          tabWidgetFactories;
    QList<SigDigger::InspectionWidgetFactory *>   inspectionWidgetFactories;
    QList<SigDigger::UIListenerFactory *>         uiListenerFactories;
    QList<SigDigger::SourceConfigWidgetFactory *> sourceConfigWidgetFactories;

    // Used for search only
    QHash<QString, SigDigger::TabWidgetFactory *>          tabWidgetFactoryTable;
    QHash<QString, SigDigger::InspectionWidgetFactory *>   inspectionWidgetFactoryTable;
    QHash<QString, SigDigger::SourceConfigWidgetFactory *> sourceConfigWidgetFactoryTable;

    std::list<std::string> recentProfiles;

    bool sources_initd;
    bool estimators_initd;
    bool spectrum_sources_initd;
    bool inspectors_initd;
    bool have_qth;

    Singleton();
    ~Singleton();

    bool havePalette(std::string const &name);
    bool haveAutoGain(std::string const &name);
    bool haveFAT(std::string const &name);
    void syncUI(void);
    void syncRecent(void);
    void syncLocations(void);
    void syncTLESources(void);
    void syncBookmarks(void);
    void initLocationsFromContext(ConfigContext &ctx, bool user);
    void initTLESourcesFromContext(ConfigContext &ctx, bool user);

    static QString normalizeTLEName(QString const &);

  public:
    void init_sources(void);
    void init_estimators(void);
    void init_spectrum_sources(void);
    void init_inspectors(void);
    void init_palettes(void);
    void init_fats(void);
    void init_autogains(void);
    void init_ui_config(void);
    void init_recent_list(void);
    void init_locations(void);
    void init_bookmarks(void);
    void init_tle_sources(void);
    void init_tle(void);
    void init_plugins(void);
    void detect_devices(void);
    void trigger_delayed(void);

    void sync(void);

    void killBackgroundTaskController(void);

    void registerDelayedCallback(DelayedPluginCallback, Plugin *);
    void registerSourceConfig(suscan_source_config_t *config);
    void registerNetworkProfile(const suscan_source_config_t *config);
    void registerSourceDevice(const suscan_source_device_t *dev);

    MultitaskController *getBackgroundTaskController(void) const;

    ConfigMap::const_iterator getFirstProfile(void) const;
    ConfigMap::const_iterator getLastProfile(void) const;
    Suscan::Source::Config *getProfile(std::string const &name);
    void saveProfile(Suscan::Source::Config const &name);

    bool registerBookmark(BookmarkInfo const& info);
    void replaceBookmark(BookmarkInfo const& info);
    void removeBookmark(qint64);

    bool registerLocation(Location const& loc);
    bool registerTLESource(TLESource const &tleSource);
    bool removeTLESource(std::string const &);

    bool registerSpectrumUnit(std::string const &, float, float);
    void replaceSpectrumUnit(std::string const &, float, float);
    void removeSpectrumUnit(std::string const &);

    void refreshDevices(void);
    void refreshNetworkProfiles(void);

    bool registerTLE(std::string const &);

    bool haveQth() const;
    Location getQth(void) const;
    void setQth(Location const &);

    std::vector<Source::Device>::const_iterator getFirstDevice(void) const;
    std::vector<Source::Device>::const_iterator getLastDevice(void) const;

    std::vector<Object>::const_iterator getFirstPalette(void) const;
    std::vector<Object>::const_iterator getLastPalette(void) const;

    std::vector<Object>::const_iterator getFirstAutoGain(void) const;
    std::vector<Object>::const_iterator getLastAutoGain(void) const;

    std::vector<Object>::const_iterator getFirstFAT(void) const;
    std::vector<Object>::const_iterator getLastFAT(void) const;

    std::vector<Object>::iterator getFirstUIConfig(void);
    std::vector<Object>::iterator getLastUIConfig(void);

    std::list<std::string>::const_iterator getFirstRecent(void) const;
    std::list<std::string>::const_iterator getLastRecent(void) const;

    QMap<qint64, Bookmark> const &getBookmarkMap(void) const;
    QMap<qint64, Bookmark>::const_iterator getFirstBookmark(void) const;
    QMap<qint64, Bookmark>::const_iterator getLastBookmark(void) const;
    QMap<qint64, Bookmark>::const_iterator getBookmarkFrom(qint64 bm) const;

    QMap<QString, Location> const &getLocationMap(void) const;
    QMap<QString, Location>::const_iterator getFirstLocation(void) const;
    QMap<QString, Location>::const_iterator getLastLocation(void) const;

    QMap<QString, Orbit> const &getSatelliteMap(void) const;
    QMap<QString, Orbit>::const_iterator getFirstSatellite(void) const;
    QMap<QString, Orbit>::const_iterator getLastSatellite(void) const;

    QMap<std::string, TLESource> const &getTLESourceMap(void) const;
    QMap<std::string, TLESource>::const_iterator getFirstTLESource(void) const;
    QMap<std::string, TLESource>::const_iterator getLastTLESource(void) const;

    QMap<std::string, SpectrumUnit> const &getSpectrumUnitMap(void) const;
    QMap<std::string, SpectrumUnit>::const_iterator getFirstSpectrumUnit(void) const;
    QMap<std::string, SpectrumUnit>::const_iterator getLastSpectrumUnit(void) const;
    QMap<std::string, SpectrumUnit>::const_iterator getSpectrumUnitFrom(std::string const &) const;

    QHash<QString, Source::Config> const &getNetworkProfileMap(void) const;
    QHash<QString, Source::Config>::const_iterator getFirstNetworkProfile(void) const;
    QHash<QString, Source::Config>::const_iterator getLastNetworkProfile(void) const;
    QHash<QString, Source::Config>::const_iterator getNetworkProfileFrom(QString const &) const;

    bool registerToolWidgetFactory(SigDigger::ToolWidgetFactory *);
    bool unregisterToolWidgetFactory(SigDigger::ToolWidgetFactory *);
    QList<SigDigger::ToolWidgetFactory *>::const_iterator getFirstToolWidgetFactory() const;
    QList<SigDigger::ToolWidgetFactory *>::const_iterator getLastToolWidgetFactory() const;

    bool registerTabWidgetFactory(SigDigger::TabWidgetFactory *);
    bool unregisterTabWidgetFactory(SigDigger::TabWidgetFactory *);
    QList<SigDigger::TabWidgetFactory *>::const_iterator getFirstTabWidgetFactory() const;
    QList<SigDigger::TabWidgetFactory *>::const_iterator getLastTabWidgetFactory() const;
    SigDigger::TabWidgetFactory *findTabWidgetFactory(QString const &) const;

    bool registerInspectionWidgetFactory(SigDigger::InspectionWidgetFactory *);
    bool unregisterInspectionWidgetFactory(SigDigger::InspectionWidgetFactory *);
    QList<SigDigger::InspectionWidgetFactory *>::const_iterator getFirstInspectionWidgetFactory() const;
    QList<SigDigger::InspectionWidgetFactory *>::const_iterator getLastInspectionWidgetFactory() const;
    SigDigger::InspectionWidgetFactory *findInspectionWidgetFactory(QString const &) const;

    bool registerSourceConfigWidgetFactory(SigDigger::SourceConfigWidgetFactory *);
    bool unregisterSourceConfigWidgetFactory(SigDigger::SourceConfigWidgetFactory *);
    QList<SigDigger::SourceConfigWidgetFactory *>::const_iterator getFirstSourceConfigWidgetFactory() const;
    QList<SigDigger::SourceConfigWidgetFactory *>::const_iterator getLastSourceConfigWidgetFactory() const;
    SigDigger::SourceConfigWidgetFactory *findSourceConfigWidgetFactory(QString const &) const;

    bool registerUIListenerFactory(SigDigger::UIListenerFactory *);
    bool unregisterUIListenerFactory(SigDigger::UIListenerFactory *);
    QList<SigDigger::UIListenerFactory *>::const_iterator getFirstUIListenerFactory() const;
    QList<SigDigger::UIListenerFactory *>::const_iterator getLastUIListenerFactory() const;

    bool notifyRecent(std::string const &name);
    bool removeRecent(std::string const &name);
    void clearRecent(void);

    void putUIConfig(unsigned int where, Object &&rv);

    const Source::Device *getDeviceAt(unsigned int index) const;

    static Singleton *get_instance(void);

    static std::string sigutilsVersion(void);
    static std::string suscanVersion(void);
  };
};

uint qHash(const xyz_t &site);

#endif // CPP_LIBRARY_H
