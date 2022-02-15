//
//    LPFTask.cpp: Product by the delayed conjugate
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
#include <LPFTask.h>
#include <Suscan/Library.h>

#define SIGDIGGER_LPF_BLOCK_LENGTH 8192

#ifndef NULL
#  define NULL nullptr
#endif // NULL

SUBOOL
LPFTask::onData(
      const struct sigutils_specttuner_channel *,
      void *privdata,
      const SUCOMPLEX *data, /* This pointer remains valid until the next call to feed */
      SUSCOUNT size)
{
  LPFTask *self = reinterpret_cast<LPFTask *>(privdata);

  while (size-- > 0)
    if (self->q < self->length)
      self->destination[self->q++] = *data++;

  return SU_TRUE;
}

LPFTask::LPFTask(
    const SUCOMPLEX *data,
    SUCOMPLEX *destination,
    size_t length,
    SUFLOAT bw,
    QObject *parent) :
  Suscan::CancellableTask(parent)
{
  struct sigutils_specttuner_params params =
      sigutils_specttuner_params_INITIALIZER;
  struct sigutils_specttuner_channel_params cparams =
      sigutils_specttuner_channel_params_INITIALIZER;

  this->origin = data;
  this->destination = destination;
  this->length = length;

  SU_ATTEMPT(this->stuner = su_specttuner_new(&params));

  cparams.f0       = 0; // Centered in 0: low-pass filter
  cparams.bw       = SU_NORM2ANG_FREQ(bw);
  cparams.guard    = 2 * PI / cparams.bw; // Ensures no decimation
  cparams.privdata = this;
  cparams.on_data  = LPFTask::onData;

  SU_ATTEMPT(this->schan  = su_specttuner_open_channel(this->stuner, &cparams));

  this->setProgress(0);
  this->setStatus("Processing...");
}

bool
LPFTask::work(void)
{
  size_t amount = this->length - this->p;

  if (amount > SIGDIGGER_LPF_BLOCK_LENGTH)
    amount = SIGDIGGER_LPF_BLOCK_LENGTH;

  SU_ATTEMPT(
        su_specttuner_feed_bulk(
          this->stuner,
          this->origin + this->p,
          amount));

  this->p += amount;

  this->setStatus("Processing ("
                  + QString::number(p)
                  + "/"
                  + QString::number(this->length)
                  + ")...");

  this->setProgress(static_cast<qreal>(p) / static_cast<qreal>(this->length));

  if (this->p < this->length)
    return true;

  // Processing done. Check if all output samples were written and return

  while (this->q < this->length) {
    SUCOMPLEX zero = 0;
    SU_ATTEMPT(su_specttuner_feed_bulk(this->stuner, &zero, 1));
  }

  emit done();
  return false;
}

void
LPFTask::cancel(void)
{
  emit cancelled();
}

LPFTask::~LPFTask()
{
  // This automatically closes any opened channel
  if (this->stuner != nullptr)
    su_specttuner_destroy(this->stuner);
}
