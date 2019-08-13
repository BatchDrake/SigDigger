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
}

Source::Config::Config(suscan_source_config_t *config)
{
  this->instance = config;
  this->borrowed = true;
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

SUFREQ
Source::Config::getFreq(void) const
{
  if (this->instance == nullptr)
    return 0;

  return suscan_source_config_get_freq(this->instance);
}

unsigned int
Source::Config::getSampleRate(void) const
{
  if (this->instance == nullptr)
    return 0;

  return suscan_source_config_get_samp_rate(this->instance);
}

enum suscan_source_type
Source::Config::getType(void) const
{
  if (this->instance == nullptr)
    return SUSCAN_SOURCE_TYPE_SDR;

  return suscan_source_config_get_type(this->instance);
}

void
Source::Config::setSampleRate(unsigned int rate)
{
  if (this->instance == nullptr)
    return;

  suscan_source_config_set_samp_rate(this->instance, rate);
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
Source::Config::setSDRDevice(const suscan_source_device_t *dev)
{
  if (this->instance == nullptr)
    return;

  SU_ATTEMPT(
        suscan_source_config_set_device(
          this->instance,
          dev));
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
