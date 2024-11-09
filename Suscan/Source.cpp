//
//    Source.cpp: Signal source wrapper
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

#include <Suscan/Source.h>
#include <QList>

using namespace Suscan;

//////////////////////////// Source config wrapper ///////////////////////////
Source::Config::Config()
{
  m_instance = nullptr;
  m_borrowed = true;
}

Source::Config::Config(
    const std::string &type,
    enum suscan_source_format fmt)
{
  SU_ATTEMPT(m_instance = suscan_source_config_new(type.c_str(), fmt));

  m_borrowed = false;
}

Source::Config::Config(Config const &prev)
{
  if (prev.m_borrowed) {
    m_instance = prev.m_instance;
  } else {
    SU_ATTEMPT(m_instance = suscan_source_config_clone(prev.m_instance));
  }
  m_borrowed = prev.m_borrowed;
}

Source::Config::Config(Config &&rv) : Config()
{
  std::swap(m_instance, rv.m_instance);
  std::swap(m_borrowed, rv.m_borrowed);
}

Source::Config::Config(suscan_source_config_t *config)
{
  m_instance = config;
  m_borrowed = true;
}

Source::Config::Config(Suscan::Object const &obj)
{
  SU_ATTEMPT(m_instance = suscan_source_config_from_object(obj.getInstance()));
  m_borrowed = false;
}

Source::Config::~Config()
{
  if (m_instance != nullptr && !m_borrowed)
    suscan_source_config_destroy(m_instance);
}

Source::Config
Source::Config::wrap(suscan_source_config_t *config)
{
  Source::Config result = Source::Config(config);

  result.m_borrowed = false;

  return result;
}

/////////////////////////////// Operators  ///////////////////////////////////
Source::Config&
Source::Config::operator=(const Config &rv)
{
  // This is what the Cobol of the 2020s looks like
  if (this != &rv) {
    if (m_instance != nullptr && !m_borrowed)
      suscan_source_config_destroy(m_instance);

    m_borrowed = rv.m_borrowed;

    if (rv.m_borrowed) {
      m_instance = rv.m_instance;
    } else {
      SU_ATTEMPT(m_instance = suscan_source_config_clone(rv.m_instance));
    }
  }

  return *this;
}

Source::Config&
Source::Config::operator=(Config &&rv)
{
  // This is what the Cobol of the 2020s looks like
  if (this != &rv) {
    std::swap(m_instance, rv.m_instance);
    std::swap(m_borrowed, rv.m_borrowed);
    std::swap(m_devSpecWrapper, m_devSpecWrapper);
  }

  return *this;
}

/////////////////////////////// Methods //////////////////////////////////////
std::string
Source::Config::label(void) const
{
  if (m_instance == nullptr)
    return "<Null profile>";

  return suscan_source_config_get_label(m_instance);
}

Suscan::Object
Source::Config::serialize(void)
{
  suscan_object_t *obj = nullptr;

  SU_ATTEMPT(m_instance != nullptr);

  SU_ATTEMPT(obj = suscan_source_config_to_object(m_instance));

  return Object::wrap(obj);
}

std::string
Source::Config::getAntenna(void) const
{
  const char *ant;

  if (m_instance == nullptr)
    return "N/C";

  ant = suscan_source_config_get_antenna(m_instance);

  if (ant == nullptr)
    return "N/C";

  return ant;
}

SUFREQ
Source::Config::getFreq(void) const
{
  if (m_instance == nullptr)
    return 0;

  return suscan_source_config_get_freq(m_instance);
}

SUFREQ
Source::Config::getLnbFreq(void) const
{
  if (m_instance == nullptr)
    return 0;

  return suscan_source_config_get_lnb_freq(m_instance);
}

unsigned int
Source::Config::getDecimatedSampleRate(void) const
{
  if (m_instance == nullptr)
    return 0;

  return suscan_source_config_get_samp_rate(m_instance)
      / suscan_source_config_get_average(m_instance);
}

unsigned int
Source::Config::getSampleRate(void) const
{
  if (m_instance == nullptr)
    return 0;

  return suscan_source_config_get_samp_rate(m_instance);
}

unsigned int
Source::Config::getDecimation(void) const
{
  if (m_instance == nullptr)
    return 0;

  return suscan_source_config_get_average(m_instance);
}

bool
Source::Config::getLoop(void) const
{
  if (m_instance == nullptr)
    return true;

  return suscan_source_config_get_loop(m_instance) != SU_FALSE;
}

bool
Source::Config::getDCRemove(void) const
{
  if (m_instance == nullptr)
    return true;

  return suscan_source_config_get_dc_remove(m_instance) != SU_FALSE;
}

bool
Source::Config::getIQBalance(void) const
{
  if (m_instance == nullptr)
    return false;

  return suscan_source_config_get_iq_balance(m_instance) != SU_FALSE;
}

struct timeval
Source::Config::getStartTime(void) const
{
  struct timeval tv = {0, 0};

  if (m_instance != nullptr)
    suscan_source_config_get_start_time(m_instance, &tv);

  return tv;
}

struct timeval
Source::Config::getEndTime(void) const
{
  struct timeval tv = {0, 0};

  if (m_instance != nullptr)
    if (!suscan_source_config_get_end_time(m_instance, &tv))
      suscan_source_config_get_start_time(m_instance, &tv);

  return tv;
}

bool
Source::Config::fileIsValid(void) const
{
  if (m_instance != nullptr)
    return suscan_source_config_file_is_valid(m_instance) != SU_FALSE;

  return false;
}

SUFLOAT
Source::Config::getBandwidth(void) const
{
  if (m_instance == nullptr)
    return 0;

  return suscan_source_config_get_bandwidth(m_instance);
}

enum suscan_source_format
Source::Config::getFormat(void) const
{
  if (m_instance == nullptr)
    return SUSCAN_SOURCE_FORMAT_AUTO;

  return suscan_source_config_get_format(m_instance);
}

std::string
Source::Config::getType(void) const
{
  if (m_instance == nullptr)
    return "soapysdr";

  return suscan_source_config_get_type(m_instance);
}

std::string
Source::Config::getPath(void) const
{
  const char *path;

  if (m_instance == nullptr)
    return "";

  path = suscan_source_config_get_path(m_instance);

  if (path == nullptr)
    return "";

  return path;
}

std::string
Source::Config::getParam(const std::string &key) const
{
  const char *param;

  if (m_instance == nullptr)
    return "";

  param = suscan_source_config_get_param(m_instance, key.c_str());

  if (param == nullptr)
    return "";

  return param;
}

bool
Source::Config::hasParam(const std::string &key) const
{
  const char *param;

  if (m_instance == nullptr)
    return "";

  param = suscan_source_config_get_param(m_instance, key.c_str());

  return param != nullptr;
}

SUBOOL
Source::Config::walkParams(
    const suscan_source_config_t *,
    const char *key,
    const char *value,
    void *userdata)
{
  QList<QPair<std::string, std::string>> *dest =
      reinterpret_cast<QList<QPair<std::string, std::string>> *>(userdata);

  dest->append(QPair<std::string, std::string>(key, value));

  return SU_TRUE;
}

QList<QPair<std::string, std::string>>
Source::Config::getParamList(void) const
{
  QList<QPair<std::string, std::string>> list;

  SU_ATTEMPT(
        suscan_source_config_walk_params(
          m_instance,
          Source::Config::walkParams,
          &list));

  return list;
}

bool
Source::Config::isRealTime() const
{
  if (m_instance == nullptr)
    return false;

  return suscan_source_config_is_real_time(m_instance) == SU_TRUE;
}

bool
Source::Config::isSeekable() const
{
  if (m_instance == nullptr)
    return false;

  return suscan_source_config_is_seekable(m_instance) == SU_TRUE;
}

bool
Source::Config::getFreqLimits(SUFREQ &min, SUFREQ &max) const
{
  if (m_instance == nullptr)
    return false;

  return
     suscan_source_config_get_freq_limits(m_instance, &min, &max) == SU_TRUE;
}

bool
Source::Config::guessMetadata(struct suscan_source_metadata &meta) const
{
  if (m_instance == nullptr)
    return false;

  return suscan_source_config_guess_metadata(m_instance, &meta) == SU_TRUE;
}

SUFLOAT
Source::Config::getPPM(void) const
{
  if (m_instance == nullptr)
    return 0.;

  return suscan_source_config_get_ppm(m_instance);
}

void
Source::Config::setSampleRate(unsigned int rate)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_samp_rate(m_instance, rate);
}

void
Source::Config::setDecimation(unsigned int rate)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_average(m_instance, rate);
}

void
Source::Config::setPath(const std::string &path)
{
  if (m_instance == nullptr)
    return;

  SU_ATTEMPT(
        suscan_source_config_set_path(
          m_instance,
          path.c_str()));
}

void
Source::Config::setFreq(SUFREQ freq)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_freq(m_instance, freq);
}

void
Source::Config::setLnbFreq(SUFREQ freq)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_lnb_freq(m_instance, freq);
}

void
Source::Config::setAntenna(const std::string &antenna)
{
  if (m_instance == nullptr)
    return;

  SU_ATTEMPT(suscan_source_config_set_antenna(m_instance, antenna.c_str()));
}

void
Source::Config::setParam(std::string const &key, std::string const &val)
{
  if (m_instance == nullptr)
    return;

  SU_ATTEMPT(
        suscan_source_config_set_param(
          m_instance,
          key.c_str(),
          val.c_str()));
}

void
Source::Config::clearParams(void)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_clear_params(m_instance);
}

void
Source::Config::debugGains(std::string const &prefix) const
{
  SU_INFO("%s: Debug gains\n", prefix.c_str());
  auto spec = getDeviceSpec();
  auto props = spec.properties();

  if (props == nullptr) {
    SU_INFO("%s: ... but device does not exist\n", prefix.c_str());
  } else {
    for (auto &gain : props->gains()) {
      auto value = getGain(gain);
      SU_INFO("%s:   %s = %g dB\n", prefix.c_str(), gain.c_str(), value);
    }
  }
}

void
Source::Config::setPPM(SUFLOAT ppm)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_ppm(m_instance, ppm);
}

void
Source::Config::setBandwidth(SUFLOAT bw)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_bandwidth(m_instance, bw);
}

void
Source::Config::setLoop(bool value)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_loop(
        m_instance,
        value ? SU_TRUE : SU_FALSE);
}

void
Source::Config::setDCRemove(bool value)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_dc_remove(
        m_instance,
        value ? SU_TRUE : SU_FALSE);
}

void
Source::Config::setStartTime(struct timeval const &tv)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_start_time(m_instance,tv);
}


void
Source::Config::setIQBalance(bool value)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_iq_balance(
        m_instance,
        value ? SU_TRUE : SU_FALSE);
}

void
Source::Config::setType(const std::string &type)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_type_format(
        m_instance,
        type.c_str(),
        suscan_source_config_get_format(m_instance));
}

void
Source::Config::setFormat(enum suscan_source_format fmt)
{
  if (m_instance == nullptr)
    return;

  suscan_source_config_set_type_format(
        m_instance,
        suscan_source_config_get_type(m_instance),
        fmt);
}

void
Source::Config::setLabel(const std::string &path)
{
  if (m_instance == nullptr)
    return;

  SU_ATTEMPT(
        suscan_source_config_set_label(
          m_instance,
          path.c_str()));
}

void
Source::Config::setDeviceSpec(const DeviceSpec &dev)
{
  if (m_instance == nullptr)
    return;

  SU_ATTEMPT(
        suscan_source_config_set_device_spec(
          m_instance,
          dev.instance()));
}

DeviceSpec
Source::Config::getDeviceSpec() const
{
  if (m_instance == nullptr)
    return DeviceSpec();

  return DeviceSpec::wrap(
        suscan_device_spec_copy(
          suscan_source_config_get_device_spec(m_instance)));
}

void
Source::Config::setGain(const std::string &name, SUFLOAT val)
{
  if (m_instance == nullptr)
    return;

  SU_ATTEMPT(
        suscan_source_config_set_gain(
          m_instance,
          name.c_str(),
          val));
}

SUFLOAT
Source::Config::getGain(const std::string &name) const
{
  if (m_instance == nullptr)
    return 0;

  return suscan_source_config_get_gain(m_instance, name.c_str());
}

///////////////////////////////// Source Wrappers ////////////////////////////
Source::Source(Config const& config)
{
  SU_ATTEMPT(m_instance = suscan_source_new(config.m_instance));

  m_config = m_instance->config; // Borrowed
}

Source::~Source()
{
  if (m_instance != nullptr)
    suscan_source_destroy(m_instance);
}
