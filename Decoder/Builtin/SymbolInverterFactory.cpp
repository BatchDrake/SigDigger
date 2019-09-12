//
//    SymbolInverterFactory.cpp: Make symbol inverters
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

#include "SymbolInverterFactory.h"
#include "SymbolInverter.h"

using namespace SigDigger;

std::string
SymbolInverterFactory::getName(void) const
{
  return "Symbol inverter";
}

std::string
SymbolInverterFactory::getDescription(void) const
{
  return "Negates every symbol bit";
}

Suscan::DecoderObjects *
SymbolInverterFactory::make(QObject *)
{
  return this->makeFromObjects(new SymbolInverter(this), nullptr);
}
