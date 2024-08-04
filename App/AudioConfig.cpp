//
//    AudioConfig.cpp: Audio device configuration
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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

#include "AudioConfig.h"

using namespace SigDigger;

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

AudioConfig::AudioConfig()
{
  loadDefaults();
}

AudioConfig::AudioConfig(Suscan::Object const &conf) : AudioConfig()
{
  deserialize(conf);
}

void
AudioConfig::loadDefaults()
{
  devStr = "";
  description = "";
}

Suscan::Object &&
AudioConfig::serialize()
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("AudioConfig");

  STORE(devStr);
  STORE(description);

  return this->persist(obj);
}

void
AudioConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(devStr);
  LOAD(description);
}
