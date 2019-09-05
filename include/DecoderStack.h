//
//    DecoderStack.h: Decoder stack
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
#ifndef DECODERSTACK_H
#define DECODERSTACK_H

#include "Decoder.h"

namespace SigDigger {
  class DecoderStack
  {
    std::vector<Decoder *> stack;
    uint8_t bps = 1;
    bool connected = false;

  public:
    DecoderStack();

    void setBps(uint8_t bps);
    uint8_t getBps(void) const;
    void clear();
    bool flush();
    void push(Decoder *);

    bool connect(std::vector<Decoder *> &offending);

    bool feed(const Symbol *symbol, size_t len);
  };
}

#endif // DECODERSTACK_H
