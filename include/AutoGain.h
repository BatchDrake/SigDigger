//
//    AutoGain.h: AutoGain configurations
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

#ifndef AUTOGAIN_H
#define AUTOGAIN_H

#include <Suscan/Object.h>
#include <vector>

namespace SigDigger {
  struct GainTable {
      std::string name;
      std::vector<int> gains;

      GainTable();
      GainTable(Suscan::Object const &obj);

      void deserialize(Suscan::Object const &obj);
  };

  struct GainConfig {
      std::string name;
      int value;

      GainConfig(std::string name, int value);
  };

  class AutoGain {
      std::string name;
      std::string driver;
      std::vector<GainTable> gains;
      int max = 0;

    public:
      AutoGain();
      AutoGain(Suscan::Object const &obj);

      std::vector<GainConfig> translateGain(int val) const;
      void deserialize(const Suscan::Object &obj);

      std::string
      getDriver(void) const
      {
        return this->driver;
      }

      std::string
      getName(void) const
      {
        return this->name;
      }

      int
      getMax(void) const
      {
        return this->max;
      }

      int
      getMin(void) const
      {
        return 0;
      }
  };
}
#endif // AUTOGAIN_H
