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

#ifndef _SUSCAN_SOURCE_H
#define _SUSCAN_SOURCE_H

#include <vector>

#include <Suscan/Compat.h>
#include <Suscan/Object.h>
#include <QPair>
#include <analyzer/source.h>
#include "Device.h"

namespace Suscan {
  class Source
  {
  private:
    suscan_source_config_t *m_config;
    suscan_source_t        *m_instance; // TODO: Make it unique_ptr

    friend class Analyzer;

  public:
    class Config;
    class GainDescription;
    Source(Config const&);
    ~Source();
  };

  class Source::Config {
    suscan_source_config_t *m_instance = nullptr;
    bool                    m_borrowed = false;
    mutable DeviceSpec      m_devSpecWrapper;

    // Convenience friendship
    friend class Analyzer;
    friend class Source;

    static SUBOOL walkParams(
        const suscan_source_config_t *,
        const char *,
        const char *,
        void *);

  public:
    inline suscan_source_config_t *
    instance() const
    {
      return m_instance;
    }

    std::string label() const;
    SUFREQ getFreq() const;
    SUFREQ getLnbFreq() const;
    unsigned int getSampleRate() const;
    unsigned int getDecimatedSampleRate() const;
    unsigned int getDecimation() const;
    std::string getType() const;
    bool getLoop() const;
    std::string getPath() const;
    std::string getAntenna() const;

    bool getDCRemove() const;
    bool getIQBalance() const;
    struct timeval getStartTime() const;
    struct timeval getEndTime() const;
    bool fileIsValid() const;
    SUFLOAT getBandwidth() const;
    SUFLOAT getGain(const std::string &) const;
    std::string getParam(const std::string &key) const;
    bool hasParam(const std::string &key) const;
    QList<QPair<std::string, std::string>> getParamList() const;
    SUFLOAT getPPM() const;
    bool isRealTime() const;
    bool isSeekable() const;
    bool getFreqLimits(SUFREQ &, SUFREQ &) const;
    bool guessMetadata(struct suscan_source_metadata &) const;

    DeviceSpec getDeviceSpec() const;
    enum suscan_source_format getFormat() const;

    void setFreq(SUFREQ freq);
    void setLnbFreq(SUFREQ freq);
    void setBandwidth(SUFLOAT bw);
    void setLoop(bool);
    void setDCRemove(bool);
    void setIQBalance(bool);
    void setStartTime(struct timeval const &tv);
    void setFormat(enum suscan_source_format fmt);
    void setType(const std::string &);
    void setLabel(const std::string &);
    void setPath(const std::string &);
    void setSampleRate(unsigned int value);
    void setDecimation(unsigned int);
    void setDeviceSpec(const DeviceSpec &dev);
    void setGain(const std::string &, SUFLOAT);
    void setAntenna(const std::string &);    
    void setParam(std::string const &key, std::string const &param);
    void clearParams();

    void debugGains(std::string const &) const;

    void setPPM(SUFLOAT);

    Config& operator=(const Config &);
    Config& operator=(Config &&);

    Suscan::Object serialize();

    Config();
    Config(const Suscan::Object &obj);
    Config(suscan_source_config_t *);
    Config(const Config &);
    Config(Config &&);
    Config(const std::string &type, enum suscan_source_format format);
    ~Config();

    static Source::Config wrap(suscan_source_config_t *config);
  };
};

#endif // CPP_SOURCE_H
