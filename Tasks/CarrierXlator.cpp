//
//    CarrierXlator.cpp: Translate central frequency
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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

#include <CarrierXlator.h>

using namespace SigDigger;

CarrierXlator::CarrierXlator(
    const SUCOMPLEX *data,
    SUCOMPLEX *destination,
    size_t length,
    SUFLOAT relFreq,
    SUFLOAT phase,
    QObject *parent) : CancellableTask(parent)
{
  this->origin      = data;
  this->destination = destination;
  this->length      = length;

  su_ncqo_init(&this->ncqo, -relFreq);
  su_ncqo_set_phase(&this->ncqo, -phase);

  this->setProgress(0);

  this->setStatus("Translating...");
}

CarrierXlator::~CarrierXlator()
{
}

bool
CarrierXlator::work(void)
{
  size_t amount = this->length - this->p;
  size_t p = this->p;

  if (amount > SIGDIGGER_CARRIER_XLATOR_BLOCK_LENGTH)
    amount = SIGDIGGER_CARRIER_XLATOR_BLOCK_LENGTH;

  while (amount--) {
    this->destination[p] = this->origin[p] * su_ncqo_read(&this->ncqo);
    ++p;
  }

  this->p = p;

  this->setStatus("Translating ("
                  + QString::number(p)
                  + "/"
                  + QString::number(this->length)
                  + ")...");

  this->setProgress(static_cast<qreal>(p) / static_cast<qreal>(this->length));

  if (this->p < this->length)
    return true;

  emit done();
  return false;
}

void
CarrierXlator::cancel(void)
{
  emit cancelled();
}
