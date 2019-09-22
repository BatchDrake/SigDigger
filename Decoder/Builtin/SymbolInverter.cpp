//
//    SymbolInverter.cpp: Symbol inverter decoder
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

#include "SymbolInverter.h"

using namespace SigDigger;

////////////////////////////////// Config //////////////////////////////////////
void
SymbolInverterConfig::deserialize(Suscan::Object const &)
{
}

Suscan::Object &&
SymbolInverterConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  return this->persist(obj);
}

////////////////////////// Inverter implementation /////////////////////////////
SymbolInverter::SymbolInverter(Suscan::DecoderFactory *manufacturer) :
  Decoder(manufacturer)
{

}

Suscan::Serializable const &
SymbolInverter::getConfig(void) const
{
  return this->config;
}

bool
SymbolInverter::setConfig(Suscan::Serializable &config)
{
  this->config.deserialize(config.serialize());

  return true;
}

std::string
SymbolInverter::getStateString(void) const
{
  return "Inverting all bits";
}

bool
SymbolInverter::setInputBps(uint8_t bps)
{
  this->bps = bps;
  this->mask = static_cast<Symbol>((1 << bps) - 1);
  return true; // Accepts any bps
}

uint8_t
SymbolInverter::getOutputBps(void) const
{
  return this->bps;
}

bool
SymbolInverter::work(FrameId, const Symbol *buffer, size_t len)
{
  Symbol sym;
  while (len-- > 0) {
    sym = ~*buffer++ & this->mask;
    if (!this->write(&sym, 1))
      return false;
  }

  return true;
}
