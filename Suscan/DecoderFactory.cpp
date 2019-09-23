//
//    DecoderFactory.cpp: Create decoders on demand
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

#include <Suscan/DecoderFactory.h>

using namespace Suscan;

DecoderUI::~DecoderUI()
{

}

DecoderFactory::DecoderFactory()
{

}

DecoderFactory::~DecoderFactory()
{

}

Decoder::~Decoder(void)
{

}

DecoderObjects::~DecoderObjects(void)
{
  delete this->ui;
  delete this->decoder;
}

DecoderObjects *
DecoderFactory::makeFromObjects(Decoder *decoder, DecoderUI *ui)
{
  DecoderObjects *objects = new DecoderObjects(decoder, ui);

  objects->factory = this;

  if (objects->decoder != nullptr)
    objects->decoder->objs = objects;

  if (objects->ui != nullptr)
    objects->ui->objs = objects;

  return objects;
}
