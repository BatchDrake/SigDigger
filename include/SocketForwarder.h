//
//    filename: description
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
#ifndef UDPFORWARDER_H
#define UDPFORWARDER_H

#include "GenericDataSaver.h"

#define SIGDIGGER_UDPFORWARDER_MAX_UDP_PAYLOAD_SIZE 508
#define SIGDIGGER_UDPFORWARDER_MAX_UDP_SAMPLES \
  (SIGDIGGER_UDPFORWARDER_MAX_UDP_PAYLOAD_SIZE / static_cast<ssize_t>(sizeof(float _Complex)))

namespace SigDigger {
  class SocketDataWriter;

  class SocketForwarder : public GenericDataSaver {
    Q_OBJECT

    SocketDataWriter *writer = nullptr;

  public:
    SocketForwarder(
        std::string const &host,
        uint16_t port,
        unsigned int size,
        bool tcp,
        QObject *parent = nullptr);
  };
}

#endif // UDPFORWARDER_H
