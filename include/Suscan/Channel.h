//
//    Channel.h: Channel description
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

#ifndef CPP_CHANNEL_H
#define CPP_CHANNEL_H

#include <sigutils/softtune.h>

namespace Suscan {
  struct Channel {
      SUFREQ  ft    = 0; // Tuner frequency
      SUFREQ  fc    = 0; // Center frequency
      SUFREQ  fLow  = 0; // Lower cutoff
      SUFREQ  fHigh = 0; // Higher cutoff
      SUFREQ  bw    = 0;
  };
}

#endif // CHANNEL_H
