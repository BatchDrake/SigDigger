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

#include <SocketForwarder.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdexcept>

#ifndef MSG_NOSIGNAL
#  define MSG_NOSIGNAL 0
#endif // MSG_NOSIGNAL

using namespace SigDigger;

namespace SigDigger {
  class SocketDataWriter : public GenericDataWriter {
    std::string host;
    uint16_t port;
    char pad[2];
    struct sockaddr_in addr;
    int fd = -1;
    bool solved = false;
    bool tcp = false;
    char pad2[2];
    unsigned int size = 0;
    std::string lastError;

  public:
    SocketDataWriter(
        std::string const &host,
        uint16_t port,
        unsigned int size,
        bool tcp);

    bool prepare(void) override;
    std::string getError(void) const override;
    bool canWrite(void) const override;
    ssize_t write(const void *data, size_t len) override;
    bool close(void) override;
    ~SocketDataWriter() override;
  };
}

SocketDataWriter::SocketDataWriter(
    std::string const &host,
    uint16_t port,
    unsigned int size,
    bool tcp) :
  host(host), port(port), tcp(tcp), size(size)
{
  this->pad[0] = this->pad2[0] = 0; // Shut up
}

bool
SocketDataWriter::prepare(void)
{
  if (!this->solved) {
    struct hostent *ent;

    if ((ent = gethostbyname(this->host.c_str())) == nullptr) {
      this->lastError = "Failed to resolve hostname " + this->host;
      return false;
    }

    if ((this->fd = socket(
           AF_INET,
           this->tcp ? SOCK_STREAM : SOCK_DGRAM,
           0)) == -1) {
      this->lastError = "Failed to open socket: " + std::string(strerror(errno));
      return false;
    }

    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(this->port);
    this->addr.sin_addr = *reinterpret_cast<struct in_addr *>(ent->h_addr);
    memset(this->addr.sin_zero, 0, 8);

    if (this->tcp) {
      if (connect(
            this->fd,
            reinterpret_cast<struct sockaddr *>(&this->addr),
            sizeof(struct sockaddr_in)) == -1) {
        this->lastError = "Cannot connect to host: " + std::string(strerror(errno));
        return false;
      }
    }
    this->solved = true;
  }

  return this->solved;
}

std::string
SocketDataWriter::getError(void) const
{
  return this->lastError;
}

bool
SocketDataWriter::canWrite(void) const
{
  return !this->solved || this->fd != -1;
}

ssize_t
SocketDataWriter::write(const void *data, size_t len)
{
  ssize_t sent;

  if (len > this->size)
    len = this->size;

  sent = sendto(
          this->fd,
          data,
          len,
          MSG_NOSIGNAL,
          reinterpret_cast<struct sockaddr *>(&this->addr),
          sizeof(struct sockaddr_in));

  if (sent < 1)
    this->lastError = std::string(strerror(errno));

  return sent;
}

bool
SocketDataWriter::close(void)
{
  bool ok = true;

  if (this->fd != -1) {
    ok = ::shutdown(this->fd, 2) == 0;
    this->fd = -1;
  }

  return ok;
}

SocketDataWriter::~SocketDataWriter(void)
{
  this->close();
}

SocketForwarder::SocketForwarder(
    std::string const &host,
    uint16_t port,
    unsigned int size,
    bool tcp,
    QObject *parent) :
  GenericDataSaver(
    this->writer = new SocketDataWriter(host, port, size, tcp),
    parent)
{

}
