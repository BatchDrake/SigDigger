//
//    Device.cpp: description
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

#include <Suscan/Device.h>
#include <analyzer/analyzer.h>

using namespace Suscan;

///////////////////////////////// DeviceGainDesc ///////////////////////////////
DeviceGainDesc::DeviceGainDesc()
{

}

DeviceGainDesc::DeviceGainDesc(const suscan_device_gain_desc_t *desc)
{
  m_name = desc->name;
  m_min  = desc->min;
  m_max  = desc->max;
  m_step = desc->step;
  m_def  = desc->def;
}

DeviceGainDesc::DeviceGainDesc(const struct suscan_source_gain_info *info)
{
  m_name = info->name;
  m_min  = info->min;
  m_max  = info->max;
  m_step = info->step;
  m_def  = info->value;
}

/////////////////////////////////// DeviceSpec /////////////////////////////////
DeviceSpec
DeviceSpec::wrap(suscan_device_spec_t *spec)
{
  DeviceSpec devSpec = DeviceSpec(spec);

  devSpec.m_borrowed = false;

  return devSpec;
}

DeviceSpec::DeviceSpec()
{
  SU_ATTEMPT(m_instance = suscan_device_spec_new());
  m_borrowed = false;
}

DeviceSpec::DeviceSpec(DeviceProperties const &prop)
{
  SU_ATTEMPT(m_instance = suscan_device_properties_make_spec(prop.instance()));
  m_borrowed = false;
}

DeviceSpec::DeviceSpec(DeviceSpec const &prev)
{
  if (prev.m_borrowed)
    m_instance = prev.m_instance;
  else
    SU_ATTEMPT(m_instance = suscan_device_spec_copy(prev.m_instance));

  m_borrowed = prev.m_borrowed;
}

DeviceSpec::DeviceSpec(DeviceSpec &&rv)
{
  std::swap(m_instance, rv.m_instance);
  std::swap(m_borrowed, m_borrowed);
}

DeviceSpec::DeviceSpec(const Suscan::Object &obj)
{
  SU_ATTEMPT(m_instance = suscan_device_spec_from_object(obj.getInstance()));
  m_borrowed = false;
}

DeviceSpec::DeviceSpec(suscan_device_spec_t *spec)
{
  assert(spec != nullptr);

  m_instance = spec;
  m_borrowed = true;
}

DeviceSpec::DeviceSpec(std::string const &uri)
{
  SU_ATTEMPT(m_instance = suscan_device_spec_from_uri(uri.c_str()));
  m_borrowed = false;
}

DeviceSpec::~DeviceSpec()
{
  if (!m_borrowed && m_instance != nullptr)
    suscan_device_spec_destroy(m_instance);
}

DeviceSpec&
DeviceSpec::operator=(const DeviceSpec &rv)
{
  // This is what the Cobol of the 2020s looks like
  if (this != &rv) {
    if (m_instance != nullptr && !m_borrowed)
      suscan_device_spec_destroy(m_instance);

    m_borrowed = rv.m_borrowed;

    if (rv.m_borrowed)
      m_instance = rv.m_instance;
    else
      SU_ATTEMPT(m_instance = suscan_device_spec_copy(rv.m_instance));
  }

  return *this;
}

DeviceSpec&
DeviceSpec::operator=(DeviceSpec &&rv)
{
  if (this != &rv) {
    std::swap(m_instance, rv.m_instance);
    std::swap(m_borrowed, rv.m_borrowed);
  }

  return *this;
}

// Getters
const DeviceProperties *
DeviceSpec::properties() const
{
  SU_ATTEMPT(m_instance != nullptr);

  suscan_device_properties_t *prop = suscan_device_spec_properties(m_instance);

  if (prop != nullptr) {
    m_propertiesWrapper = DeviceProperties(prop);
    return &m_propertiesWrapper;
  }

  return nullptr;
}

std::string
DeviceSpec::analyzer() const
{
  SU_ATTEMPT(m_instance != nullptr);
  return suscan_device_spec_analyzer(m_instance);
}

std::string
DeviceSpec::source() const
{
  SU_ATTEMPT(m_instance != nullptr);
  return suscan_device_spec_source(m_instance);
}

std::string
DeviceSpec::uri() const
{
  char *uri = nullptr;
  SU_ATTEMPT(m_instance != nullptr);
  SU_ATTEMPT(uri = suscan_device_spec_to_uri(m_instance));
  std::string result = uri;
  free(uri);

  return result;
}

std::string
DeviceSpec::host() const
{
  std::string sourceStr = source();

  auto pos = sourceStr.find(':');

  if (pos != std::string::npos)
    sourceStr.resize(pos);

  return sourceStr;
}

std::string
DeviceSpec::port() const
{
  std::string sourceStr = source();

  auto pos = sourceStr.find(':');

  if (pos != std::string::npos)
    return sourceStr.substr(pos + 1);

  return "28001";
}

std::string
DeviceSpec::get(std::string const &key) const
{
  SU_ATTEMPT(m_instance != nullptr);

  auto result = suscan_device_spec_get(m_instance, key.c_str());
  if (result == nullptr)
    result = "";

  return result;
}

uint64_t
DeviceSpec::uuid() const
{
  SU_ATTEMPT(m_instance != nullptr);
  return suscan_device_spec_uuid(m_instance);
}

// Setters
void
DeviceSpec::reset()
{
  SU_ATTEMPT(m_instance != nullptr);
  suscan_device_spec_reset(m_instance);
}

void
DeviceSpec::setAnalyzer(std::string const &analyzer)
{
  SU_ATTEMPT(m_instance != nullptr);
  SU_ATTEMPT(suscan_device_spec_set_analyzer(m_instance, analyzer.c_str()));
}

void
DeviceSpec::setSource(std::string const &source)
{
  SU_ATTEMPT(m_instance != nullptr);
  SU_ATTEMPT(suscan_device_spec_set_source(m_instance, source.c_str()));
}

void
DeviceSpec::set(std::string const &key, std::string const &value)
{
  SU_ATTEMPT(m_instance != nullptr);
  SU_ATTEMPT(suscan_device_spec_set(m_instance, key.c_str(), value.c_str()));
}

void
DeviceSpec::setTraits(std::map<std::string, std::string> const &map)
{
  strmap_t *traits = nullptr;

  SU_ATTEMPT(m_instance != nullptr);

  try {
    SU_ATTEMPT(traits = strmap_new());
    for (auto &p : map)
      SU_ATTEMPT(strmap_set(traits, p.first.c_str(), p.second.c_str()));
    SU_ATTEMPT(suscan_device_spec_set_traits(m_instance, traits));
  } catch (Exception const &e) {
    if (traits != nullptr)
      SU_DISPOSE(strmap, traits);
    throw e;
  }

  if (traits != nullptr)
    SU_DISPOSE(strmap, traits);
}

void
DeviceSpec::setParams(std::map<std::string, std::string> const &map)
{
  strmap_t *params = nullptr;

  SU_ATTEMPT(m_instance != nullptr);

  try {
    SU_ATTEMPT(params = strmap_new());
    for (auto &p : map)
      SU_ATTEMPT(strmap_set(params, p.first.c_str(), p.second.c_str()));
    SU_ATTEMPT(suscan_device_spec_set_params(m_instance, params));
  } catch (Exception const &e) {
    if (params != nullptr)
      SU_DISPOSE(strmap, params);
    throw e;
  }

  if (params != nullptr)
    SU_DISPOSE(strmap, params);
}

void
DeviceSpec::updateUuid()
{
  SU_ATTEMPT(m_instance != nullptr);
  suscan_device_spec_update_uuid(m_instance);
}

//////////////////////////////// DeviceProperties //////////////////////////////
void
DeviceProperties::copyGains()
{
  m_gains.clear();

  if (m_instance != nullptr) {
    suscan_device_gain_desc_t *const *gain_list;
    int gain_count = suscan_device_properties_get_all_gains(
          m_instance,
          &gain_list);

    for (auto i = 0; i < gain_count; ++i)
      m_gains[gain_list[i]->name] = DeviceGainDesc(gain_list[i]);
  }
}

DeviceProperties
DeviceProperties::wrap(suscan_device_properties_t *spec)
{
  DeviceProperties devSpec = DeviceProperties(spec);

  devSpec.m_borrowed = false;

  return devSpec;
}

DeviceProperties::DeviceProperties()
{
  m_instance = nullptr;
  m_borrowed = false;
}

DeviceProperties::DeviceProperties(DeviceProperties const &prev)
{
  if (prev.m_borrowed)
    m_instance = prev.m_instance;
  else
    SU_ATTEMPT(m_instance = suscan_device_properties_dup(prev.m_instance));

  m_borrowed = prev.m_borrowed;

  copyGains();
}

DeviceProperties::DeviceProperties(DeviceProperties &&rv)
{
  std::swap(m_instance, rv.m_instance);
  std::swap(m_borrowed, m_borrowed);
  std::swap(m_gains, rv.m_gains);
}

DeviceProperties::DeviceProperties(suscan_device_properties_t *spec)
{
  m_instance = spec;
  m_borrowed = true;
  copyGains();
}

DeviceProperties::~DeviceProperties()
{
  if (!m_borrowed && m_instance != nullptr)
    suscan_device_properties_destroy(m_instance);
}

DeviceProperties&
DeviceProperties::operator=(const DeviceProperties &rv)
{
  // This is what the Cobol of the 2020s looks like
  if (this != &rv) {
    if (m_instance != nullptr && !m_borrowed)
      suscan_device_properties_destroy(m_instance);

    m_borrowed = rv.m_borrowed;

    if (rv.m_borrowed)
      m_instance = rv.m_instance;
    else
      SU_ATTEMPT(m_instance = suscan_device_properties_dup(rv.m_instance));
  }

  copyGains();

  return *this;
}

DeviceProperties&
DeviceProperties::operator=(DeviceProperties &&rv)
{
  if (this != &rv) {
    std::swap(m_instance, rv.m_instance);
    std::swap(m_borrowed, rv.m_borrowed);
    std::swap(m_gains, rv.m_gains);
  }

  return *this;
}

// Getters
bool
DeviceProperties::match(DeviceSpec const &spec) const
{
  SU_ATTEMPT(m_instance != nullptr);

  return suscan_device_properties_match(m_instance, spec.instance());
}

DeviceSpec *
DeviceProperties::makeSpec() const
{
  suscan_device_spec *cSpec = nullptr;

  SU_ATTEMPT(m_instance != nullptr);

  SU_ATTEMPT(cSpec = suscan_device_properties_make_spec(m_instance));

  DeviceSpec *spec = new DeviceSpec(DeviceSpec::wrap(cSpec));

  return spec;
}

uint64_t
DeviceProperties::uuid() const
{
  SU_ATTEMPT(m_instance != nullptr);

  return suscan_device_properties_uuid(m_instance);
}

const DeviceGainDesc *
DeviceProperties::lookupGain(std::string const &name) const
{
  auto it = m_gains.find(name);
  if (it == m_gains.end())
      return nullptr;

  return &it->second;
}

SUFREQ
DeviceProperties::minFreq() const
{
  SU_ATTEMPT(m_instance != nullptr);

  return m_instance->freq_min;
}

SUFREQ
DeviceProperties::maxFreq() const
{
  SU_ATTEMPT(m_instance != nullptr);

  return m_instance->freq_max;
}

std::list<std::string>
DeviceProperties::gains() const
{
  std::list<std::string> gains;

  SU_ATTEMPT(m_instance != nullptr);

  for (auto &gain : m_gains)
    gains.push_back(gain.first);

  return gains;
}

std::list<std::string>
DeviceProperties::antennas() const
{
  std::list<std::string> antennas;

  SU_ATTEMPT(m_instance != nullptr);

  for (unsigned i = 0; i < m_instance->antenna_count; ++i)
    antennas.push_back(m_instance->antenna_list[i]);

  return antennas;
}

std::list<qreal>
DeviceProperties::sampleRates() const
{
  std::list<qreal> rates;

  SU_ATTEMPT(m_instance != nullptr);

  for (unsigned i = 0; i < m_instance->samp_rate_count; ++i)
    rates.push_back(m_instance->samp_rate_list[i]);

  return rates;
}

std::string
DeviceProperties::analyzer() const
{
  SU_ATTEMPT(m_instance != nullptr);

  if (m_instance->analyzer == nullptr)
    return "";

  return m_instance->analyzer->name;
}

std::string
DeviceProperties::source() const
{
  SU_ATTEMPT(m_instance != nullptr);

  if (m_instance->source == nullptr)
    return "";

  return m_instance->source->name;
}

std::string
DeviceProperties::label() const
{
  SU_ATTEMPT(m_instance != nullptr);

  return m_instance->label;
}

std::string
DeviceProperties::get(std::string const &key) const
{
  SU_ATTEMPT(m_instance != nullptr);

  auto result = suscan_device_properties_get(m_instance, key.c_str());
  if (result == nullptr)
    result = "";

  return result;
}

std::string const &
DeviceProperties::uri() const
{
  SU_ATTEMPT(m_instance != nullptr);

  if (m_uri.empty()) {
    char *uri;
    SU_ATTEMPT(uri = suscan_device_properties_uri(m_instance));
    m_uri = uri;
    free(uri);
  }

  return m_uri;
}

///////////////////////////////// DeviceFacade ///////////////////////////////
DeviceFacade *DeviceFacade::m_singleton = nullptr;

DeviceFacade *
DeviceFacade::instance()
{
  if (m_singleton == nullptr)
    m_singleton = new DeviceFacade(suscan_device_facade_instance());

  return m_singleton;
}

DeviceFacade::DeviceFacade(suscan_device_facade_t *instance)
{
  SU_ATTEMPT(instance != nullptr);

  m_instance = instance;
}

std::list<DeviceProperties>
DeviceFacade::devices() const
{
  suscan_device_properties_t **prop_list;
  std::list<DeviceProperties> devices;
  int prop_count;

  SU_ATTEMPT(m_instance != nullptr);
  SU_ATTEMPT(
        (prop_count = suscan_device_facade_get_all_devices(m_instance, &prop_list)) != -1);

  for (auto i = 0; i < prop_count; ++i)
    devices.push_back(DeviceProperties::wrap(prop_list[i]));

  if (prop_list != nullptr)
    free(prop_list);

  return devices;
}

DeviceProperties *
DeviceFacade::deviceByUuid(uint64_t uuid) const
{
  SU_ATTEMPT(m_instance != nullptr);

  auto prop = suscan_device_facade_get_device_by_uuid(m_instance, uuid);

  if (prop == nullptr)
    return nullptr;

  return new DeviceProperties(DeviceProperties::wrap(prop));
}


void
DeviceFacade::discoverAll()
{
  SU_ATTEMPT(m_instance != nullptr);
  SU_ATTEMPT(suscan_device_facade_discover_all(m_instance));
}

bool
DeviceFacade::startDiscovery(std::string const &name)
{
  SU_ATTEMPT(m_instance != nullptr);

  return suscan_device_facade_start_discovery(m_instance, name.c_str());
}

bool
DeviceFacade::stopDiscovery(std::string const &name)
{
  SU_ATTEMPT(m_instance != nullptr);

  return suscan_device_facade_stop_discovery(m_instance, name.c_str());
}

bool
DeviceFacade::waitForDevices(std::string &name, unsigned int timeoutMs)
{
  SU_ATTEMPT(m_instance != nullptr);

  auto result = suscan_device_facade_wait_for_devices(m_instance, timeoutMs);

  if (result != nullptr) {
    name = result;
    free(result);
    return true;
  }

  return false;
}
