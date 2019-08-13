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

#include <codec/codec.h>
#include <analyzer/source.h>
#include <analyzer/estimator.h>
#include <analyzer/spectsrc.h>
#include <analyzer/inspector/inspector.h>

namespace Suscan {
  class Singleton {
  private:
    static Singleton *instance;
    static Logger *logger;

    std::vector<Source::Config> profiles;

    bool codecs_initd;
    bool sources_initd;
    bool estimators_initd;
    bool spectrum_sources_initd;
    bool inspectors_initd;

    Singleton();
    ~Singleton();

  public:
    void init_codecs(void);
    void init_sources(void);
    void init_estimators(void);
    void init_spectrum_sources(void);
    void init_inspectors(void);

    void registerSourceConfig(suscan_source_config_t *config);

    std::vector<Source::Config>::const_iterator getFirstProfile(void) const;
    std::vector<Source::Config>::const_iterator getLastProfile(void) const;

    static Singleton *get_instance(void);
  };
};

#endif // CPP_LIBRARY_H
