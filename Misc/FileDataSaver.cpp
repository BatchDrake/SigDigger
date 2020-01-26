//
//    AsyncDataSaver.h: save high bandwidth data to a file
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

#include "FileDataSaver.h"
#include <unistd.h>

using namespace SigDigger;

namespace SigDigger {
  class FileDataWriter : public GenericDataWriter {
    int fd = -1;
    int pad;
    std::string lastError;

  public:
    FileDataWriter(int fd);

    bool prepare(void);
    bool canWrite(void) const;
    std::string getError(void) const;
    ssize_t write(const SUCOMPLEX *data, size_t len);
    bool close(void);
    ~FileDataWriter();
  };
}

std::string
FileDataWriter::getError(void) const
{
  return this->lastError;
}

bool
FileDataWriter::prepare(void)
{
  return true;
}

FileDataWriter::FileDataWriter(int fd)
{
  this->fd = fd;
}

bool
FileDataWriter::canWrite(void) const
{
  return this->fd != -1;
}

ssize_t
FileDataWriter::write(const SUCOMPLEX *data, size_t len)
{
  ssize_t result;

  if (this->fd == -1)
    return 0;

  result = ::write(this->fd, data, len * sizeof(*data));

  if (result < 1)
    lastError = "write() failed: " + std::string(strerror(errno));

  return result / static_cast<ssize_t>(sizeof(*data));
}

bool
FileDataWriter::close(void)
{
  bool ok = true;

  if (this->fd != -1) {
    ok = ::close(this->fd) == 0;
    this->fd = -1;
  }

  return ok;
}

FileDataWriter::~FileDataWriter(void)
{
  this->close();
}

//////////////////////////// FileDataSaver /////////////////////////////////////
FileDataSaver::FileDataSaver(int fd, QObject *parent) :
  GenericDataSaver(this->writer = new FileDataWriter(fd), parent)
{
}

FileDataSaver::~FileDataSaver(void)
{
  if (this->writer != nullptr)
    delete this->writer;
}

