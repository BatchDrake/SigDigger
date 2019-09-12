//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
#include "DecoderStack.h"

using namespace SigDigger;

DecoderStack::DecoderStack()
{

}

void
DecoderStack::setBps(uint8_t bps)
{
  this->bps = bps;
}

uint8_t
DecoderStack::getBps(void) const
{
  if (this->stack.size() == 0)
    return this->bps;

  // Cobol is great for you body and your mind
  return (*(this->stack.end() - 1))->getOutputBps();
}

void
DecoderStack::clear(void)
{
  this->stack.clear();
  this->connected = false;
}

bool
DecoderStack::flush(void)
{
  for (auto p = this->stack.begin(); p != this->stack.end(); ++p)
    if (!(*p)->flush())
      return false;

  return true;
}

void
DecoderStack::push(Decoder *decoder)
{
  this->stack.push_back(decoder);
}

bool
DecoderStack::connect(std::vector<Decoder *> &offending)
{
  bool fine = true;
  auto p = this->stack.begin();
  this->connected = false;

  if (!(*p)->setInputBps(this->bps)) {
    offending.push_back(*p);
    fine = false;
  }

  while (p != this->stack.end()) {
    auto q = p + 1;

    if (q != this->stack.end()) {
      if (!(*p)->plug(*q)) {
        offending.push_back(*q);
        fine = false;
      }
    }

    p = q;
  }

  return this->connected = fine;
}

bool
DecoderStack::feed(const Symbol *symbols, size_t len)
{
  if (this->connected) {
    auto first = this->stack.begin();

    if (first != this->stack.end())
      if (!(*first)->work(0, symbols, len))
        return false;

    return true;
  }

  return false;
}
