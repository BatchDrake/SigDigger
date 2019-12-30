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

#include <map>
#include <list>

namespace Suscan {
  typedef std::map<std::string, Source::Config> ConfigMap;

  class Singleton {
    static Singleton *instance;
    static Logger *logger;

    std::vector<Source::Device> devices;
    ConfigMap profiles;
    std::vector<Object> palettes;
    std::vector<Object> autoGains;
    std::vector<Object> uiConfig;
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

    void syncUI(void);
    void syncRecent(void);

  public:
    void init_codecs(void);
    void init_sources(void);
    void init_estimators(void);
    void init_spectrum_sources(void);
    void init_inspectors(void);
    void init_palettes(void);
    void init_autogains(void);
    void init_ui_config(void);
    void init_recent_list(void);
    void detect_devices(void);

    void sync(void);

    void registerSourceConfig(suscan_source_config_t *config);

    void registerSourceDevice(const suscan_source_device_t *dev);


    ConfigMap::const_iterator getFirstProfile(void) const;
    ConfigMap::const_iterator getLastProfile(void) const;
    Suscan::Source::Config *getProfile(std::string const &name);
    void saveProfile(Suscan::Source::Config const &name);

    std::vector<Source::Device>::const_iterator getFirstDevice(void) const;
    std::vector<Source::Device>::const_iterator getLastDevice(void) const;

    std::vector<Object>::const_iterator getFirstPalette(void) const;
    std::vector<Object>::const_iterator getLastPalette(void) const;

    std::vector<Object>::const_iterator getFirstAutoGain(void) const;
    std::vector<Object>::const_iterator getLastAutoGain(void) const;

    std::vector<Object>::iterator getFirstUIConfig(void);
    std::vector<Object>::iterator getLastUIConfig(void);

    std::list<std::string>::const_iterator getFirstRecent(void) const;
    std::list<std::string>::const_iterator getLastRecent(void) const;

    bool notifyRecent(std::string const &name);
    bool removeRecent(std::string const &name);
    void clearRecent(void);

    void putUIConfig(unsigned int where, Object &&rv);

    const Source::Device *getDeviceAt(unsigned int index) const;

    static Singleton *get_instance(void);
  };
};

#endif // CPP_LIBRARY_H
