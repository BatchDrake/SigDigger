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

using namespace Suscan;

//////////////////////////// Source device wrapper ///////////////////////////
Source::GainDescription::GainDescription(const struct suscan_source_gain_desc *desc)
{
  this->def  = desc->def;
  this->max  = desc->max;
  this->min  = desc->min;
  this->step = desc->step;
  this->name = std::string(desc->name);
}

void
Source::Device::setDevice(const suscan_source_device_t *dev, unsigned int channel)
{
  unsigned int i;

  struct suscan_source_device_info info =
      suscan_source_device_info_INITIALIZER;

  this->instance = dev;

  this->antennas.clear();
  this->gains.clear();

  this->freqMin = 0;
  this->freqMax = 0;

  if (suscan_source_device_get_info(dev, channel, &info)) {
    this->freqMax = info.freq_max;
    this->freqMin = 0; //info.freq_min;

    for (i = 0; i < info.antenna_count; ++i)
      this->antennas.push_back(std::string(info.antenna_list[i]));

    for (i = 0; i < info.gain_desc_count; ++i)
      this->gains.push_back(Source::GainDescription(info.gain_desc_list[i]));

    for (i = 0; i < info.samp_rate_count; ++i)
      this->rates.push_back(info.samp_rate_list[i]);
    suscan_source_device_info_finalize(&info);
  }
}

Source::Device::Device(Source::Device &&rv)
{
  *this = rv;
}

Source::Device::Device(const Source::Device &dev)
{
  this->setDevice(dev);
}

Source::Device::Device(const suscan_source_device_t *dev, unsigned int channel)
{
  this->setDevice(dev, channel);
}

Source::Device::Device()
{
  this->instance = nullptr;
}

//////////////////////////// Source config wrapper ///////////////////////////

Source::Config::Config()
{
  this->instance = nullptr;
  this->borrowed = true;
}

Source::Config::Config(
    enum suscan_source_type type,
    enum suscan_source_format fmt)
{
  SU_ATTEMPT(this->instance = suscan_source_config_new(type, fmt));

  this->borrowed = false;
}

Source::Config::Config(Config const &prev)
{
  if (prev.borrowed) {
    this->instance = prev.instance;
  } else {
    SU_ATTEMPT(this->instance = suscan_source_config_clone(prev.instance));
  }
  this->borrowed = prev.borrowed;
}

Source::Config::Config(Config &&rv) : Config()
{
  std::swap(this->instance, rv.instance);
  std::swap(this->borrowed, rv.borrowed);
  this->devWrapper = std::move(rv.devWrapper);
}

Source::Config::Config(suscan_source_config_t *config)
{
  this->instance = config;
  this->borrowed = true;
}

Source::Config::Config(Suscan::Object const &obj)
{
  SU_ATTEMPT(this->instance = suscan_source_config_from_object(obj.getInstance()));
  this->borrowed = false;
}

Source::Config::~Config()
{
  if (this->instance != nullptr && !this->borrowed)
    suscan_source_config_destroy(this->instance);
}

/////////////////////////////// Operators  ///////////////////////////////////
Source::Config&
Source::Config::operator=(const Config &rv)
{
  // This is what the Cobol of the 2020s looks like
  if (this != &rv) {
    if (this->instance != nullptr && !this->borrowed)
      suscan_source_config_destroy(this->instance);

    this->borrowed = rv.borrowed;

    if (rv.borrowed) {
      this->instance = rv.instance;
    } else {
      SU_ATTEMPT(this->instance = suscan_source_config_clone(rv.instance));
    }
  }

  return *this;
}

Source::Config&
Source::Config::operator=(Config &&rv)
{
  // This is what the Cobol of the 2020s looks like
  if (this != &rv) {
    std::swap(this->instance, rv.instance);
    std::swap(this->borrowed, rv.borrowed);
    this->devWrapper = std::move(rv.devWrapper);
  }

  return *this;
}

/////////////////////////////// Methods //////////////////////////////////////
std::string
Source::Config::label(void) const
{
  if (this->instance == nullptr)
    return "<Null profile>";

  return suscan_source_config_get_label(this->instance);
}

Suscan::Object
Source::Config::serialize(void)
{
  suscan_object_t *obj = nullptr;

  SU_ATTEMPT(this->instance != nullptr);

  SU_ATTEMPT(obj = suscan_source_config_to_object(this->instance));

  return Object::wrap(obj);
}

std::string
Source::Config::getAntenna(void) const
{
  const char *ant;

  if (this->instance == nullptr)
    return "N/C";

  ant = suscan_source_config_get_antenna(this->instance);

  if (ant == nullptr)
    return "N/C";

  return ant;
}

SUFREQ
Source::Config::getFreq(void) const
{
  if (this->instance == nullptr)
    return 0;

  return suscan_source_config_get_freq(this->instance);
}

SUFREQ
Source::Config::getLnbFreq(void) const
{
  if (this->instance == nullptr)
    return 0;

  return suscan_source_config_get_lnb_freq(this->instance);
}

unsigned int
Source::Config::getDecimatedSampleRate(void) const
{
  if (this->instance == nullptr)
    return 0;

  return suscan_source_config_get_samp_rate(this->instance)
      / suscan_source_config_get_average(this->instance);
}

unsigned int
Source::Config::getSampleRate(void) const
{
  if (this->instance == nullptr)
    return 0;

  return suscan_source_config_get_samp_rate(this->instance);
}

unsigned int
Source::Config::getDecimation(void) const
{
  if (this->instance == nullptr)
    return 0;

  return suscan_source_config_get_average(this->instance);
}

bool
Source::Config::getLoop(void) const
{
  if (this->instance == nullptr)
    return true;

  return suscan_source_config_get_loop(this->instance) != SU_FALSE;
}

bool
Source::Config::getDCRemove(void) const
{
  if (this->instance == nullptr)
    return true;

  return suscan_source_config_get_dc_remove(this->instance) != SU_FALSE;
}

bool
Source::Config::getIQBalance(void) const
{
  if (this->instance == nullptr)
    return false;

  return suscan_source_config_get_iq_balance(this->instance) != SU_FALSE;
}

SUFLOAT
Source::Config::getBandwidth(void) const
{
  if (this->instance == nullptr)
    return 0;

  return suscan_source_config_get_bandwidth(this->instance);
}

enum suscan_source_format
Source::Config::getFormat(void) const
{
  if (this->instance == nullptr)
    return SUSCAN_SOURCE_FORMAT_AUTO;

  return suscan_source_config_get_format(this->instance);
}

enum suscan_source_type
Source::Config::getType(void) const
{
  if (this->instance == nullptr)
    return SUSCAN_SOURCE_TYPE_SDR;

  return suscan_source_config_get_type(this->instance);
}

std::string
Source::Config::getPath(void) const
{
  const char *path;

  if (this->instance == nullptr)
    return "";

  path = suscan_source_config_get_path(this->instance);

  if (path == nullptr)
    return "";

  return path;
}

void
Source::Config::setSampleRate(unsigned int rate)
{
  if (this->instance == nullptr)
    return;

  suscan_source_config_set_samp_rate(this->instance, rate);
}

void
Source::Config::setDecimation(unsigned int rate)
{
  if (this->instance == nullptr)
    return;

  suscan_source_config_set_average(this->instance, rate);
}

void
Source::Config::setPath(const std::string &path)
{
  if (this->instance == nullptr)
    return;

  SU_ATTEMPT(
        suscan_source_config_set_path(
          this->instance,
          path.c_str()));
}

void
Source::Config::setFreq(SUFREQ freq)
{
  if (this->instance == nullptr)
    return;

  suscan_source_config_set_freq(this->instance, freq);
}

void
Source::Config::setLnbFreq(SUFREQ freq)
{
  if (this->instance == nullptr)
    return;

  suscan_source_config_set_lnb_freq(this->instance, freq);
}

void
Source::Config::setAntenna(const std::string &antenna)
{
  if (this->instance == nullptr)
    return;

  SU_ATTEMPT(suscan_source_config_set_antenna(this->instance, antenna.c_str()));
}

void
Source::Config::setBandwidth(SUFLOAT bw)
{
  if (this->instance == nullptr)
    return;

  suscan_source_config_set_bandwidth(this->instance, bw);
}

void
Source::Config::setLoop(bool value)
{
  if (this->instance == nullptr)
    return;

  suscan_source_config_set_loop(
        this->instance,
        value ? SU_TRUE : SU_FALSE);
}

void
Source::Config::setDCRemove(bool value)
{
  if (this->instance == nullptr)
    return;

  suscan_source_config_set_dc_remove(
        this->instance,
        value ? SU_TRUE : SU_FALSE);
}

void
Source::Config::setIQBalance(bool value)
{
  if (this->instance == nullptr)
    return;

  suscan_source_config_set_iq_balance(
        this->instance,
        value ? SU_TRUE : SU_FALSE);
}

void
Source::Config::setType(enum suscan_source_type type)
{
  if (this->instance == nullptr)
    return;

  suscan_source_config_set_type_format(
        this->instance,
        type,
        suscan_source_config_get_format(this->instance));
}

void
Source::Config::setFormat(enum suscan_source_format fmt)
{
  if (this->instance == nullptr)
    return;

  suscan_source_config_set_type_format(
        this->instance,
        suscan_source_config_get_type(this->instance),
        fmt);
}

void
Source::Config::setLabel(const std::string &path)
{
  if (this->instance == nullptr)
    return;

  SU_ATTEMPT(
        suscan_source_config_set_label(
          this->instance,
          path.c_str()));
}

void
Source::Config::setDevice(const Source::Device &dev)
{
  SU_ATTEMPT(
        suscan_source_config_set_device(
          this->instance,
          dev.instance));
}

const Source::Device &
Source::Config::getDevice(void)
{
  if (this->instance != nullptr) {
    if (this->devWrapper.getInstance() !=
        suscan_source_config_get_device(this->instance)) {
      this->devWrapper.setDevice(
            suscan_source_config_get_device(this->instance),
            0);
    }
  }

  return this->devWrapper;
}

void
Source::Config::setGain(const std::string &name, SUFLOAT val)
{
  if (this->instance == nullptr)
    return;

  SU_ATTEMPT(
        suscan_source_config_set_gain(
          this->instance,
          name.c_str(),
          val));
}

SUFLOAT
Source::Config::getGain(const std::string &name) const
{
  if (this->instance == nullptr)
    return 0;

  return suscan_source_config_get_gain(this->instance, name.c_str());
}

///////////////////////////////// Source Wrappers ////////////////////////////
Source::Source(Config const& config)
{
  SU_ATTEMPT(this->instance = suscan_source_new(config.instance));

  this->config = this->instance->config; // Borrowed
}

Source::~Source()
{
  if (this->instance != nullptr)
    suscan_source_destroy(this->instance);
}
