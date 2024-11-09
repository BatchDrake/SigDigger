//
//    Device.h: description
//    Copyright (C) 2024 Gonzalo José Carracedo Carballal
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
#ifndef _SUSCAN_DEVICE_H
#define _SUSCAN_DEVICE_H

#include <Suscan/Compat.h>
#include <Suscan/Object.h>
#include <QPair>
#include <map>
#include <list>
#include <analyzer/device/spec.h>
#include <analyzer/device/properties.h>
#include <analyzer/device/facade.h>
#include <analyzer/source/info.h>

namespace Suscan {
  class DeviceSpec;

  class DeviceGainDesc {
    std::string m_name = "";
    SUFLOAT     m_min  = 0;
    SUFLOAT     m_max  = 0;
    SUFLOAT     m_step = 0;
    SUFLOAT     m_def  = 0;

  public:
    DeviceGainDesc();
    DeviceGainDesc(const suscan_device_gain_desc_t *);
    DeviceGainDesc(const struct suscan_source_gain_info *);

    inline std::string
    getName() const
    {
      return m_name;
    }

    inline SUFLOAT
    getMin() const
    {
      return m_min;
    }

    inline SUFLOAT
    getMax() const
    {
      return m_max;
    }

    inline SUFLOAT
    getStep() const
    {
      return m_step;
    }

    inline SUFLOAT
    getDefault() const
    {
      return m_def;
    }
  };

  class DeviceProperties {
    bool                                  m_borrowed = false;
    suscan_device_properties_t           *m_instance = nullptr;

    std::map<std::string, DeviceGainDesc> m_gains;
    mutable std::string                   m_uri;

    void copyGains();

  public:
    inline suscan_device_properties_t *
    instance() const
    {
      return m_instance;
    }

    static DeviceProperties wrap(suscan_device_properties_t *config);

    DeviceProperties();
    DeviceProperties(suscan_device_properties_t *);
    DeviceProperties(DeviceProperties const &prev);
    DeviceProperties(DeviceProperties &&rv);
    ~DeviceProperties();

    DeviceProperties& operator=(const DeviceProperties &);
    DeviceProperties& operator=(DeviceProperties &&);

    bool match(DeviceSpec const &) const;
    DeviceSpec *makeSpec() const;
    uint64_t uuid() const;
    const DeviceGainDesc *lookupGain(std::string const &) const;
    SUFREQ minFreq() const;
    SUFREQ maxFreq() const;
    std::list<std::string> gains() const;
    std::list<std::string> antennas() const;
    std::list<qreal> sampleRates() const;
    std::string analyzer() const;
    std::string source() const;
    std::string label() const;
    std::string get(std::string const &) const;
    std::string const &uri() const;

    // No setters. Only the C API can handle these objects.
  };

  class DeviceSpec {
    bool                     m_borrowed = false;
    suscan_device_spec_t    *m_instance = nullptr;
    mutable DeviceProperties m_propertiesWrapper;

  public:
    inline suscan_device_spec_t *
    instance() const
    {
      return m_instance;
    }

    static DeviceSpec wrap(suscan_device_spec_t *config);

    DeviceSpec();
    DeviceSpec(DeviceProperties const &);
    DeviceSpec(DeviceSpec const &prev);
    DeviceSpec(DeviceSpec &&rv);
    DeviceSpec(const Suscan::Object &obj);
    DeviceSpec(suscan_device_spec_t *);
    DeviceSpec(std::string const &uri);
    ~DeviceSpec();

    DeviceSpec& operator=(const DeviceSpec &);
    DeviceSpec& operator=(DeviceSpec &&);

    // Getters
    const DeviceProperties *properties() const;
    std::string analyzer() const;
    std::string source() const;
    std::string uri() const;
    std::string get(std::string const &) const;
    uint64_t uuid() const;

    // Setters
    void reset();
    void setAnalyzer(std::string const &);
    void setSource(std::string const &);
    void set(std::string const &, std::string const &);
    void setTraits(std::map<std::string, std::string> const &);
    void setParams(std::map<std::string, std::string> const &);
    void updateUuid();
  };

  class DeviceFacade {
    static DeviceFacade *m_singleton;

    suscan_device_facade_t *m_instance = nullptr;

    DeviceFacade(suscan_device_facade_t *instance);

  public:
    static DeviceFacade *instance();

    std::list<DeviceProperties> devices() const;
    DeviceProperties *deviceByUuid(uint64_t) const;

    void discoverAll();
    bool startDiscovery(std::string const &);
    bool stopDiscovery(std::string const &);
    bool waitForDevices(std::string &, unsigned int);
  };
}
#endif // _SUSCAN_DEVICE_H
