//
//    Version.h: SigDigger versioning info
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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

#ifndef SIGDIGGER_VERSION_H
#define SIGDIGGER_VERSION_H

#include <sigutils/version.h>

#define SIGDIGGER_VERSION_MAJOR 0
#define SIGDIGGER_VERSION_MINOR 2
#define SIGDIGGER_VERSION_PATCH 0

// This number is incremented every time a new incompatible iteration of
// the config file is added.
#define SIGDIGGER_UICONFIG_VERSION 1

// This is the implicit version of a uiconfig file that does not define
// a version field.
#define SIGDIGGER_UICONFIG_DEFAULT_VERSION 0

#define SIGDIGGER_VERSION \
  SU_VER(SIGDIGGER_VERSION_MAJOR, SIGDIGGER_VERSION_MINOR, SIGDIGGER_VERSION_PATCH)

#define SIGDIGGER_API_VERSION \
  SU_VER(SIGDIGGER_VERSION_MAJOR, SIGDIGGER_VERSION_MINOR, 0)

#define SIGDIGGER_VERSION_STRING        \
  STRINGIFY(SIGDIGGER_VERSION_MAJOR) "." \
  STRINGIFY(SIGDIGGER_VERSION_MINOR) "." \
  STRINGIFY(SIGDIGGER_VERSION_PATCH)

#endif // SIGDIGGER_VERSION_H
