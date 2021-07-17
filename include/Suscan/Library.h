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

#include <codec/codec.h>
#include <analyzer/source.h>
#include <analyzer/estimator.h>
#include <analyzer/spectsrc.h>
#include <analyzer/inspector/inspector.h>
#include <analyzer/discovery.h>
#include <BookmarkInfo.h>

#include <map>
#include <list>
#include <QMap>

#include <QHash>

namespace Suscan {
  uint qHash(const Suscan::Source::Device &dev);

  class MultitaskController;

  typedef std::map<std::string, Source::Config> ConfigMap;

  struct Bookmark {
    BookmarkInfo info;

    int entry = -1;
  };

  struct SpectrumUnit {
    std::string name = "dBFS";
    float dBPerUnit  = 1.0f;
    float zeroPoint  = 0.0f;
  };

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

    QMap<qint64, Bookmark> bookmarks;
    QMap<std::string, SpectrumUnit> spectrumUnits;
    QHash<QString, Source::Config> networkProfiles;
    std::list<std::string> recentProfiles;

    bool codecs_initd;
    bool sources_initd;
    bool estimators_initd;
    bool spectrum_sources_initd;
    bool inspectors_initd;

    Singleton();
    ~Singleton();

    bool havePalette(std::string const &name);
    bool haveAutoGain(std::string const &name);
    bool haveFAT(std::string const &name);
    void syncUI(void);
    void syncRecent(void);
    void syncBookmarks(void);

  public:
    void init_codecs(void);
    void init_sources(void);
    void init_estimators(void);
    void init_spectrum_sources(void);
    void init_inspectors(void);
    void init_palettes(void);
    void init_fats(void);
    void init_autogains(void);
    void init_ui_config(void);
    void init_recent_list(void);
    void init_bookmarks(void);
    void detect_devices(void);

    void sync(void);

    void killBackgroundTaskController(void);

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

    bool registerSpectrumUnit(std::string const &, float, float);
    void replaceSpectrumUnit(std::string const &, float, float);
    void removeSpectrumUnit(std::string const &);

    void refreshDevices(void);
    void refreshNetworkProfiles(void);

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

    QMap<std::string, SpectrumUnit> const &getSpectrumUnitMap(void) const;
    QMap<std::string, SpectrumUnit>::const_iterator getFirstSpectrumUnit(void) const;
    QMap<std::string, SpectrumUnit>::const_iterator getLastSpectrumUnit(void) const;
    QMap<std::string, SpectrumUnit>::const_iterator getSpectrumUnitFrom(std::string const &) const;

    QHash<QString, Source::Config> const &getNetworkProfileMap(void) const;
    QHash<QString, Source::Config>::const_iterator getFirstNetworkProfile(void) const;
    QHash<QString, Source::Config>::const_iterator getLastNetworkProfile(void) const;
    QHash<QString, Source::Config>::const_iterator getNetworkProfileFrom(QString const &) const;

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


#endif // CPP_LIBRARY_H
