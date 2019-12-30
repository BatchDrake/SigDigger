//
//    Object.cpp: Suscan objects implementation
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

#include <Suscan/Object.h>

using namespace Suscan;

Object::Object(void)
{
  this->borrowed = true;
  this->instance = nullptr;
}

Object::Object(enum suscan_object_type type)
{
  this->borrowed = false;

  SU_ATTEMPT(this->instance = suscan_object_new(type));
}

Object::Object(suscan_object_t *instance)
{
  this->borrowed = true;
  this->instance = instance;
}

Object::Object(std::string const &url, const uint8_t *data, size_t size)
{
  this->borrowed = false;

  SU_ATTEMPT(this->instance = suscan_object_from_xml(url.c_str(), data, size));
}

Object::Object(Object &&rv)
{
  *this = std::move(rv);
}

Object::Object(Object const &obj) : Object(obj.instance)
{

}

Object::~Object()
{
  if (!this->borrowed)
    if (this->instance != nullptr) {
      suscan_object_destroy(this->instance);
    }
}

void
Object::clear(void)
{
  switch (this->getType()) {
    case SUSCAN_OBJECT_TYPE_FIELD:
      suscan_object_set_value(this->instance, "");
      break;

    case SUSCAN_OBJECT_TYPE_SET:
      suscan_object_set_clear(this->instance);
      break;

    case SUSCAN_OBJECT_TYPE_OBJECT:
      suscan_object_clear_fields(this->instance);
      break;
  }
}
