//
//    Decoder.cpp: Abstract decoder
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
#include "Decoder.h"

using namespace SigDigger;

Decoder::Decoder(Suscan::DecoderFactory *manufacturer)
{
  this->manufacturer = manufacturer;
}

Decoder::~Decoder(void)
{

}

Suscan::DecoderFactory *
Decoder::getManufacturer(void) const
{
  return this->manufacturer;
}

void
Decoder::setBufferSize(size_t size)
{
  if (size < this->bufferSize) {
    // Buffer size is too small. Don't worry about it and flush.
    this->flush();
  }

  this->bufferSize = size;
}

bool
Decoder::write(const Symbol *output, size_t len)
{
  size_t chunklen;

  while (len > 0) {
    chunklen = this->bufferSize - this->buffer.size();
    if (chunklen > len)
      chunklen = len;

    this->buffer.insert(this->buffer.end(), output, output + chunklen);
    output += chunklen;
    len -= chunklen;

    if (this->buffer.size() == SIGDIGGER_DECODER_DEFAULT_BUFFER_SIZE)
      if (!this->flush())
        return false;
  }

  return true;
}

bool
Decoder::flush(void)
{
  bool ok = true;

  if (this->next != nullptr)
    ok = this->work(this->frame, this->buffer.data(), this->buffer.size());

  this->buffer.clear();

  return ok;
}

bool
Decoder::plug(Decoder *next)
{
  if (next != nullptr)
    if (!next->accepts(this->outputBps()))
      return false;

  this->next = next;

  return true;
}

bool
Decoder::nextFrame(void)
{
  bool ok = true;

  if (this->buffer.size() > 0)
    ok = this->flush();

  ++this->frame;

  return ok;
}
