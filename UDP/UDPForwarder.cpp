//
//    UDPForwarder.cpp
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

#include <UDPForwarder.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>

using namespace SigDigger;

namespace SigDigger {
  class UDPDataWriter : public GenericDataWriter {
    std::string host;
    uint16_t port;
    char pad[2];
    struct sockaddr_in addr;
    int fd = -1;
    bool solved = false;
    char pad2[3];
    unsigned int size = 0;

  public:
    UDPDataWriter(std::string const &host, uint16_t port, unsigned int size);
    bool canWrite(void) const override;
    ssize_t write(const float _Complex *data, size_t len) override;
    bool close(void) override;
    ~UDPDataWriter() override;
  };
}

UDPDataWriter::UDPDataWriter(
    std::string const &host,
    uint16_t port,
    unsigned int size) :
  host(host), port(port), size(size / sizeof(float _Complex))
{
  this->pad[0] = this->pad2[0] = 0; // Shut up
}

bool
UDPDataWriter::canWrite(void) const
{
  return !this->solved || this->fd != -1;
}

ssize_t
UDPDataWriter::write(const float _Complex *data, size_t len)
{
  ssize_t sent;

  if (!this->solved) {
    struct hostent *ent;

    if ((ent = gethostbyname(this->host.c_str())) == nullptr)
      return 0;

    if ((this->fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
      return 0;

    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(this->port);
    this->addr.sin_addr = *reinterpret_cast<struct in_addr *>(ent->h_addr);
    memset(this->addr.sin_zero, 0, 8);

    this->solved = true;
  }

  if (len > this->size)
    len = this->size;

  sent = sendto(
          this->fd,
          data,
          static_cast<size_t>(len) * sizeof(float _Complex),
          0,
          reinterpret_cast<struct sockaddr *>(&this->addr),
          sizeof(struct sockaddr_in));

  sent /= static_cast<ssize_t>(sizeof(float _Complex));

  return sent;
}

bool
UDPDataWriter::close(void)
{
  bool ok = true;

  if (this->fd != -1) {
    ok = ::shutdown(this->fd, 2) == 0;
    this->fd = -1;
  }

  return ok;
}

UDPDataWriter::~UDPDataWriter(void)
{
  this->close();
}

UDPForwarder::UDPForwarder(
    std::string const &host,
    uint16_t port,
    unsigned int size,
    QObject *parent) :
  GenericDataSaver(this->writer = new UDPDataWriter(host, port, size), parent)
{

}
