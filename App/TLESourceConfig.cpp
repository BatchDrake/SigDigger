//
//    TLESourceConfig.cpp: Persist TLE source config
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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
#include <TLESourceConfig.h>

using namespace SigDigger;

TLESourceConfig::TLESourceConfig()
{
  this->loadDefaults();
}

TLESourceConfig::TLESourceConfig(Suscan::Object const &conf) : TLESourceConfig()
{
  this->deserialize(conf);
}

void
TLESourceConfig::loadDefaults(void)
{
  this->autoDownloadOnStartup = false;
}

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

Suscan::Object &&
TLESourceConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("tlesrccfg");
  STORE(autoDownloadOnStartup);

  return this->persist(obj);
}

void
TLESourceConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(autoDownloadOnStartup);
}
