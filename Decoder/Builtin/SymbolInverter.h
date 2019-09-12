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
#ifndef SYMBOLINVERTER_H
#define SYMBOLINVERTER_H

#include <Decoder.h>

namespace SigDigger {
  class SymbolInverterConfig : public Suscan::Serializable {
  public:
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class SymbolInverter : public Decoder
  {
    SymbolInverterConfig config;
    uint8_t bps;
    Symbol mask;

  public:
    SymbolInverter(Suscan::DecoderFactory *manufacturer);

    Suscan::Serializable const &getConfig(void) const override;
    bool setConfig(Suscan::Serializable &config) override;

    std::string getStateString(void) const override;
    bool setInputBps(uint8_t bps) override;
    uint8_t getOutputBps(void) const override;
    bool work(FrameId frame, const Symbol *buffer, size_t len) override;
  };
}

#endif // SYMBOLINVERTER_H
