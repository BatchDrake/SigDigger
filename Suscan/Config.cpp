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

#include <Suscan/Config.h>

using namespace Suscan;

FieldValue::FieldValue(struct suscan_field_value *inst)
{
  this->instance = inst;
}

Config::Config()
{
  this->instance = nullptr;
}

Config::~Config()
{
  if (this->owned)
    suscan_config_destroy(this->instance);
}

void
Config::populate(void)
{
  if (instance != nullptr)
    for (unsigned int i = 0; i < instance->desc->field_count; ++i)
      this->fields.push_back(FieldValue(instance->values[i]));
}

Config::Config(suscan_config_t const *instance)
{
  SU_ATTEMPT(this->instance = suscan_config_dup(instance));
  this->owned = true;
  this->populate();
}

Config::Config(suscan_config_t *instance)
{
  this->instance = instance;
  this->owned = false;
  this->populate();
}

FieldValue const *
Config::get(std::string const &name) const
{
  for (auto p = this->fields.begin();
         p != this->fields.end();
         ++p) {
    if (p->getName() == name)
      return &*p;
  }

  return nullptr;
}

ConfigContext::ConfigContext(suscan_config_context_t *ctx)
{
  this->ctx = ctx;
}

ConfigContext::ConfigContext(std::string const &name)
{
  suscan_config_context_t *ctx;

  if ((ctx = suscan_config_context_lookup(name.c_str())) == nullptr) {
    SU_ATTEMPT(suscan_confdb_use(name.c_str()));
    SU_ATTEMPT(ctx = suscan_config_context_lookup(name.c_str()));
  }

  this->ctx = ctx;
}

void
ConfigContext::setSave(bool save)
{
  suscan_config_context_set_save(this->ctx, save ? SU_TRUE : SU_FALSE);
}

Object
ConfigContext::listObject(void) const
{
  return Object(
        (suscan_object_t *) suscan_config_context_get_list(this->ctx));
}
