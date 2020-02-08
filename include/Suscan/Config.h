//
//    Object.h: Suscan config
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

#ifndef CPP_SUSCAN_CONFIG_H
#define CPP_SUSCAN_CONFIG_H

#include <Suscan/Compat.h>
#include <Suscan/Object.h>
#include <vector>
#include <util/confdb.h>
#include <cfg.h>

namespace Suscan {
  class FieldValue {
      struct suscan_field_value *instance = nullptr;

    public:
      FieldValue(struct suscan_field_value *inst);

      enum suscan_field_type
      getType(void) const
      {
        return this->instance->field->type;
      }

      std::string
      getName(void) const
      {
        return this->instance->field->name;
      }

      std::string
      getDescription(void) const
      {
        return this->instance->field->desc;
      }

      uint64_t
      getUint64(void) const
      {
        return this->instance->as_int;
      }

      bool
      getBoolean(void) const
      {
        return this->instance->as_bool != SU_FALSE;
      }

      SUFLOAT
      getFloat(void) const
      {
        return this->instance->as_float;
      }

      std::string
      getString(void) const
      {
        return this->instance->as_string;
      }
  };

  class Config {
      suscan_config_t *instance = nullptr;
      bool owned = false;
      std::vector<FieldValue> fields;

      void populate(void);

    public:
      Config();
      Config(const suscan_config_t *instance);
      Config(suscan_config_t *instance);
      ~Config();

      FieldValue const *get(std::string const &name) const;

      const suscan_config_t *
      getInstance(void) const
      {
        return this->instance;
      }

      void
      set(std::string const &name, uint64_t val)
      {
        SU_ATTEMPT(
              suscan_config_set_integer(this->instance, name.c_str(), val));
      }

      void
      set(std::string const &name, SUFLOAT val)
      {
        SU_ATTEMPT(
              suscan_config_set_float(this->instance, name.c_str(), val));
      }

      void
      set(std::string const &name, bool val)
      {
        SU_ATTEMPT(
              suscan_config_set_bool(
                this->instance,
                name.c_str(),
                val ? SU_TRUE : SU_FALSE));
      }

      void
      set(std::string const &name, std::string const &val)
      {
        SU_ATTEMPT(
              suscan_config_set_string(
                this->instance,
                name.c_str(),
                val.c_str()));
      }

      bool
      hasPrefix(std::string const &name) const
      {
        return
            suscan_config_desc_has_prefix(
              instance->desc,
              name.c_str()) != SU_FALSE;
      }

      std::vector<FieldValue>::const_iterator
      begin(void) const
      {
        return this->fields.begin();
      }

      std::vector<FieldValue>::const_iterator
      end(void) const
      {
        return this->fields.end();
      }
  };

  class ConfigContext {
      suscan_config_context_t *ctx;

  public:
      ConfigContext(suscan_config_context_t *instance);
      ConfigContext(std::string const &name);
      void setSave(bool);
      void save(void) const;
      Object listObject(void) const;

      static void
      saveAll(void)
      {
        SU_ATTEMPT(suscan_confdb_save_all());
      }
  };
}

#endif // CONFIG_H
