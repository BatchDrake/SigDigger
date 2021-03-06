//
//    GuiConfig.cpp: Gui configuration object
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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

#include "GuiConfig.h"

using namespace SigDigger;

GuiConfig::GuiConfig()
{
  this->loadDefaults();
}

GuiConfig::GuiConfig(Suscan::Object const &conf) : GuiConfig()
{
  this->deserialize(conf);
}

void
GuiConfig::loadDefaults(void)
{
  this->useLMBdrag = false;
}

#define STRINGFY(x) #x
#define CCSTORE(field) \
  obj.set(STRINGFY(field), this->field.name().toStdString())
#define CCLOAD(field)           \
  this->field = QColor(         \
      QString::fromStdString(   \
        conf.get(STRINGFY(field), this->field.name().toStdString())))

Suscan::Object &&
GuiConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("guicfg");
  obj.set("useLMBdrag", this->useLMBdrag);

  return this->persist(obj);
}

void
GuiConfig::deserialize(Suscan::Object const &conf)
{
  this->useLMBdrag = conf.get("useLMBdrag", this->useLMBdrag);
}
