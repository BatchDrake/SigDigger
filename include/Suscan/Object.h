//
//    Object.h: Suscan objects
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

#ifndef CPP_SUSCAN_OBJECT_H
#define CPP_SUSCAN_OBJECT_H

#include <Suscan/Compat.h>

#include <util/object.h>
#include <vector>

namespace Suscan {
  class Object {
      suscan_object_t *instance = nullptr;
      bool borrowed = true;

    public:
      Object();
      Object(enum suscan_object_type type);
      Object(suscan_object_t *instance);
      Object(std::string const &url, const uint8_t *data, size_t size);
      Object(const Object &obj);
      Object(Object &&rv);

      ~Object();

      static Object
      makeField(std::string const &val)
      {
        Object obj(SUSCAN_OBJECT_TYPE_FIELD);
        obj.setValue(val);
        return obj;
      }

      static Object
      wrap(suscan_object_t *instance)
      {
        Object obj;

        if (instance == nullptr)
          throw Suscan::Exception("Attempting to wrap null object");

        obj.instance = instance;
        obj.borrowed = false;

        return obj;
      }

      Object &
      operator=(const Object &obj)
      {
        this->borrowed = true;
        this->instance = obj.instance;

        return *this;
      }

      Object &
      operator=(Object &&obj)
      {
        std::swap(this->borrowed, obj.borrowed);
        std::swap(this->instance, obj.instance);

        return *this;
      }

      bool
      operator==(const Object &obj)
      {
        return this->instance == obj.instance;
      }

      bool
      isBorrowed(void) const
      {
        return this->borrowed;
      }

      bool
      isHollow(void) const
      {
        return this->instance == nullptr;
      }

      suscan_object_t *
      getInstance(void) const
      {
        return this->instance;
      }

      std::vector<char>
      serialize(void) const
      {
        if (!this->isHollow()) {
          void *data = nullptr;
          size_t size;

          SU_ATTEMPT(suscan_object_to_xml(this->instance, &data, &size));

          std::vector<char> serialization(
                static_cast<uint8_t *>(data),
                static_cast<uint8_t *>(data) + size);

          free(data);

          return serialization;
        } else {
          return std::vector<char>();
        }
      }

      void
      deserialize(
          std::string const &url,
          std::vector<char> const &data)
      {
        suscan_object_t *object = nullptr;

        SU_ATTEMPT(object = suscan_object_from_xml(
              url.c_str(),
              data.data(),
              data.size()));

        if (!this->isBorrowed()) {
          if (!this->isHollow())
            suscan_object_destroy(this->instance);
        } else {
          this->borrowed = false;
        }

        this->instance = object;
      }

      std::string
      getClass(void) const
      {
        const char *className = suscan_object_get_class(this->instance);
        return className == nullptr ? "" : className;
      }

      void
      setClass(std::string const &name)
      {
        SU_ATTEMPT(suscan_object_set_class(this->instance, name.c_str()));
      }

      enum suscan_object_type
      getType(void) const {
        return suscan_object_get_type(this->instance);
      }

      Object
      getField(std::string const &field) const
      {
        suscan_object_t *obj;

        SU_ATTEMPT(obj = suscan_object_get_field(this->instance, field.c_str()));

        return Object(obj);
      }

      void
      setField(std::string const &field, Object &&obj)
      {
        if (obj.isBorrowed())
          throw Suscan::Exception("Cannot set borrowed objects as object fields");

        SU_ATTEMPT(suscan_object_set_field(this->instance, field.c_str(), obj.instance));

        obj.borrowed = true;
      }

      void
      setField(std::string const &field, Object &obj)
      {
        if (obj.isBorrowed())
          throw Suscan::Exception("Cannot set borrowed objects as object fields");

        SU_ATTEMPT(suscan_object_set_field(this->instance, field.c_str(), obj.instance));

        obj.borrowed = true;
      }

      unsigned int
      getFieldCount(void) const
      {
        return suscan_object_field_count(this->instance);
      }

      Object
      getFieldByIndex(unsigned int field) const
      {
        suscan_object_t *obj;

        SU_ATTEMPT(obj = suscan_object_get_field_by_index(this->instance, field));

        return Object(obj);
      }

      int
      get(std::string const &field, int dfl) const
      {
        return suscan_object_get_field_int(this->instance, field.c_str(), dfl);
      }

      bool
      get(std::string const &field, bool dfl) const
      {
        return suscan_object_get_field_bool(
              this->instance,
              field.c_str(),
              dfl == SU_FALSE ? false : true) != SU_FALSE;
      }

      unsigned int
      get(std::string const &field, unsigned int dfl) const
      {
        return suscan_object_get_field_uint(this->instance, field.c_str(), dfl);
      }

      SUFLOAT
      get(std::string const &field, SUFLOAT dfl) const
      {
        return suscan_object_get_field_float(this->instance, field.c_str(), dfl);
      }

      std::string
      get(std::string const &field, std::string const &dfl) const
      {
        const char *val;

        val = suscan_object_get_field_value(this->instance, field.c_str());

        if (val == nullptr)
          return dfl;

        return val;
      }

      double
      get(std::string const &field, double dfl) const
      {
        const char *val;
        double asDbl;

        val = suscan_object_get_field_value(this->instance, field.c_str());

        if (val == nullptr)
          return dfl;

        try {
          asDbl = std::stod(val);
        } catch (const std::exception &) {
          asDbl = dfl;
        }

        return asDbl;
      }

      void
      set(std::string const &field, int val)
      {
        SU_ATTEMPT(suscan_object_set_field_int(this->instance, field.c_str(), val));
      }

      void
      set(std::string const &field, unsigned int val)
      {
        SU_ATTEMPT(suscan_object_set_field_uint(this->instance, field.c_str(), val));
      }

      void
      set(std::string const &field, bool val)
      {
        SU_ATTEMPT(suscan_object_set_field_bool(this->instance, field.c_str(), val != SU_FALSE));
      }

      void
      set(std::string const &field, SUFLOAT val)
      {
        SU_ATTEMPT(suscan_object_set_field_float(this->instance, field.c_str(), val));
      }

      void
      set(std::string const &field, double val)
      {
        char asString[32];

        snprintf(asString, sizeof(asString), "%.18e", val);

        SU_ATTEMPT(suscan_object_set_field_value(this->instance, field.c_str(), asString));
      }

      void
      set(std::string const &field, std::string const &val)
      {
        SU_ATTEMPT(suscan_object_set_field_value(this->instance, field.c_str(), val.c_str()));
      }

      void
      setValue(std::string const &val)
      {
        SU_ATTEMPT(suscan_object_set_value(this->instance, val.c_str()));
      }

      std::string
      name(void) const
      {
        return suscan_object_get_name(this->instance);
      }

      std::string
      value(void) const
      {
        return suscan_object_get_value(this->instance);
      }

      // Set-type objects
      unsigned int
      length(void) const
      {
        return suscan_object_set_get_count(this->instance);
      }

      Object
      operator [] (unsigned int index) const
      {
        suscan_object_t *obj;

        SU_ATTEMPT(obj = suscan_object_set_get(this->instance, index));

        return Object(obj);
      }

      void
      put(Object &obj, unsigned int index)
      {
        if (obj.isBorrowed())
          throw Suscan::Exception("Cannot put borrowed objects into a set");

        SU_ATTEMPT(suscan_object_set_put(this->instance, index, obj.instance));

        obj.borrowed = true;
      }

      void
      remove(unsigned int index)
      {
        SU_ATTEMPT(suscan_object_set_delete(this->instance, index));
      }

      void
      append(Object &obj)
      {
        if (obj.isBorrowed())
          throw Suscan::Exception("Cannot put borrowed objects into a set");

        SU_ATTEMPT(suscan_object_set_append(this->instance, obj.instance));

        obj.borrowed = true;
      }

      void
      append(Object &&obj)
      {
        if (obj.isBorrowed())
          throw Suscan::Exception("Cannot put borrowed objects into a set");

        SU_ATTEMPT(suscan_object_set_append(this->instance, obj.instance));

        obj.borrowed = true;
      }

      void clear(void);
  };
}

#endif // OBJECT_H
