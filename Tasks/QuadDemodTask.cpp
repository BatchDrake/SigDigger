//
//    QuadDemodTask.cpp: Product by the delayed conjugate
//    Copyright (C) 2022 Gonzalo José Carracedo Carballal
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
#include <QuadDemodTask.h>
#include <Suscan/Library.h>

#define SIGDIGGER_CYCLO_BLOCK_LENGTH 4096

QuadDemodTask::QuadDemodTask(
    const SUCOMPLEX *data,
    SUCOMPLEX *destination,
    size_t length,
    QObject *parent) :
  Suscan::CancellableTask(parent)
{
  this->origin = data;
  this->destination = destination;
  this->length = length;

  this->setProgress(0);
  this->setStatus("Processing...");
}

bool
QuadDemodTask::work(void)
{
  size_t amount = this->length - this->p;
  size_t p = this->p;
  SUFLOAT k = 1. / PI;
  SUCOMPLEX x;

  if (amount > SIGDIGGER_CYCLO_BLOCK_LENGTH)
    amount = SIGDIGGER_CYCLO_BLOCK_LENGTH;

  while (amount--) {
    x = this->origin[p];
    if (p < 1) {
      this->destination[p] = 0;
    } else {
      this->destination[p] = k * SU_C_ARG(x * SU_C_CONJ(this->prev));
    }

    this->prev = x;
    ++p;
  }

  this->p = p;
  this->setStatus("Processing ("
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
QuadDemodTask::cancel(void)
{
  emit cancelled();
}

QuadDemodTask::~QuadDemodTask()
{
}
