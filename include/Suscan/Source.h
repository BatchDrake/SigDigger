//
//    Source.h: Signal source object
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

#ifndef CPP_SOURCE_H
#define CPP_SOURCE_H

#include <Suscan/Compat.h>

#include <analyzer/source.h>

namespace Suscan {
  class Source
  {
  private:
    suscan_source_config_t *config;
    suscan_source_t *instance; // TODO: Make it unique_ptr

    friend class Analyzer;

  public:
    class Config;

    Source(Config const&);
    ~Source();
  };

  class Source::Config {
  private:
    bool borrowed;

    // Convenience friendship
    friend class Analyzer;
    friend class Source;

  public:
    suscan_source_config_t *instance;
    std::string label(void) const;
    SUFREQ getFreq(void) const;
    unsigned int getSampleRate() const;
    enum suscan_source_type getType(void) const;

    void setLabel(const std::string &);
    void setPath(const std::string &);
    void setSDRDevice(const suscan_source_device_t *);
    void setSampleRate(unsigned int value);

    Config& operator=(const Config &);
    Config& operator=(Config &&);

    Config();
    Config(suscan_source_config_t *);
    Config(const Config &);
    Config(Config &&);
    Config(enum suscan_source_type type, enum suscan_source_format format);
    ~Config();
  };
};

#endif // CPP_SOURCE_H
