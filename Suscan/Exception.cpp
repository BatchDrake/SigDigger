//
//    Exception.cpp: Wrapper for Suscan's generic exceptions
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

#include <Suscan/Compat.h>

using namespace Suscan;

Exception::Exception(std::string const& message) :
  std::runtime_error("C++ API exception: " + message)
{ }

Exception::Exception(std::string const& file, unsigned int line, std::string const& message) :
  Exception("expression `" + message + "' failed in " + file + ":" + std::to_string(line))
{ }

