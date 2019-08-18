//
//    Compat.h: Suscan's C API compatibility layer
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

#ifndef CPP_COMPAT_H
#define CPP_COMPAT_H

#include <stdexcept>
#include <memory>
#include <cmath>
#include <sigutils/types.h>

#define SU_ATTEMPT(expr)     \
  if (!(expr)) {             \
    throw Suscan::Exception( \
      __FILE__,              \
      __LINE__,              \
      STRINGIFY(expr));      \
  }

namespace Suscan {
  class Exception: public std::runtime_error {

  public:
    Exception(std::string const &file, unsigned int line, std::string const &message);
    Exception(std::string const &predicate);
  };
};

#endif // CPP_COMPAT_H
