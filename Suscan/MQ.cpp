//
//    MQ.cpp: Message queue implementation
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

#include <Suscan/MQ.h>

using namespace Suscan;

// MT-Safe
void *
MQ::read(uint32_t &type)
{
  return suscan_mq_read(&this->mq, &type);
}

MQ::MQ()
{
  this->mq_initialized = false;

  SU_ATTEMPT(suscan_mq_init(&this->mq));

  this->mq_initialized = true;
}

MQ::~MQ()
{
  if (this->mq_initialized)
    suscan_mq_finalize(&this->mq);
}
