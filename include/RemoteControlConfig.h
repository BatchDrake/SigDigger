//
//    RemoteControlConfig.h: Remote Control config
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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
#ifndef REMOTECONTROLCONFIG_H
#define REMOTECONTROLCONFIG_H

#include <Suscan/Serializable.h>
#include <QObject>

namespace SigDigger {
  class RemoteControlConfig : public Suscan::Serializable
  {
    public:
        std::string host = "localhost";
        unsigned    port = 1234;
        bool        enabled = false;

      RemoteControlConfig();
      RemoteControlConfig(Suscan::Object const &conf);

      // Overriden methods
      void loadDefaults(void);
      void deserialize(Suscan::Object const &conf) override;
      Suscan::Object &&serialize(void) override;
  };
}

#endif // REMOTECONTROLCONFIG_H
