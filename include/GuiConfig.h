//
//    GuiConfig.h: Gui configuration object
//    Copyright (C) 2021 Jaroslav Šafka
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

#ifndef GUICONFIG_H
#define GUICONFIG_H

#include <Suscan/Serializable.h>

namespace SigDigger {
  class GuiConfig : public Suscan::Serializable
  {
    public:
        /**
         * @brief use left mouse button to drag
         * drag / change center frequency on FFT spectrum area
         */
        bool useLMBdrag;
        bool noLimits;
        bool useGLWaterfall;

      GuiConfig();
      GuiConfig(Suscan::Object const &conf);

      // Overriden methods
      void loadDefaults(void);
      void deserialize(Suscan::Object const &conf) override;
      Suscan::Object &&serialize(void) override;
  };
}

#endif // GUICONFIG_H
