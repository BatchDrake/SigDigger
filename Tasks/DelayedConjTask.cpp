//
//    DelayedConjTask.cpp: Product by the delayed conjugate
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
#include <DelayedConjTask.h>
#include <Suscan/Library.h>

#define SIGDIGGER_DELAYEDCONJ_BLOCK_LENGTH 4096

DelayedConjTask::DelayedConjTask(
    const SUCOMPLEX *data,
    SUCOMPLEX *destination,
    size_t length,
    SUSCOUNT delay,
    QObject *parent) :
  Suscan::CancellableTask(parent)
{
  this->origin = data;
  this->destination = destination;
  this->length = length;
  this->delay = delay;

  if (delay == 0)
    throw Suscan::Exception("Delay is zero samples\n");

  this->delayLine.resize(delay);

  this->setProgress(0);
  this->setStatus("Processing...");
}

static inline bool
isinf(SUCOMPLEX val)
{
  return isinf(SU_C_REAL(val)) || isinf(SU_C_IMAG(val));
}

static inline bool
isnan(SUCOMPLEX val)
{
  return isnan(SU_C_REAL(val)) || isnan(SU_C_IMAG(val));
}

bool
DelayedConjTask::work(void)
{
  size_t amount = this->length - this->p;
  size_t p = this->p;
  SUCOMPLEX x, prev;
  SUSCOUNT delay = this->delay;
  SUFLOAT kinv;

  if (amount > SIGDIGGER_DELAYEDCONJ_BLOCK_LENGTH)
    amount = SIGDIGGER_DELAYEDCONJ_BLOCK_LENGTH;

  while (amount--) {
    x = this->origin[p];
    if (p < delay) {
      this->destination[p] = 0;
    } else {
      prev = this->delayLine[q];
      kinv = 1. / (SU_C_ABS(prev) + 1e-3);
      this->destination[p] = kinv * x * SU_C_CONJ(prev);
    }

    this->delayLine[q++] = x;
    if (q == delay)
      q = 0;
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
DelayedConjTask::cancel(void)
{
  emit cancelled();
}

DelayedConjTask::~DelayedConjTask()
{
}
