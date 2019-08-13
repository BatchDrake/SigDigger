//
//    Serializable.h: Interface for serializable objects
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

#ifndef CPP_SERIALIZABLE_H
#define CPP_SERIALIZABLE_H

#include <Suscan/Object.h>

namespace Suscan {
  class Serializable {
      Object storage;

    public:
      void
      putStorage(Object &&obj)
      {
        this->storage = std::move(obj);
      }

      Object &&
      takeStorage(void)
      {
        return std::move(this->storage);
      }

      Object &&
      persist(Object &obj)
      {
        this->putStorage(std::move(obj));
        return this->takeStorage();
      }

      virtual void deserialize(Object const &conf) = 0;
      virtual Object &&serialize(void) = 0;
      virtual ~Serializable();
  };
}

#endif // SERIALIZABLE_H
