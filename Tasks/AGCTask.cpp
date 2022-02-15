//
//    AGCTask.cpp: Automatic Gain Control
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
#include <AGCTask.h>
#include <Suscan/Library.h>

#define SIGDIGGER_AGC_FAST_RISE_FRAC   (2 * 3.9062e-1)
#define SIGDIGGER_AGC_FAST_FALL_FRAC   (2 * SIGDIGGER_AGC_FAST_RISE_FRAC)
#define SIGDIGGER_AGC_SLOW_RISE_FRAC   (10 * SIGDIGGER_AGC_FAST_RISE_FRAC)
#define SIGDIGGER_AGC_SLOW_FALL_FRAC   (10 * SIGDIGGER_AGC_FAST_FALL_FRAC)
#define SIGDIGGER_AGC_HANG_MAX_FRAC    (SIGDIGGER_AGC_FAST_RISE_FRAC * 5)
#define SIGDIGGER_AGC_DELAY_LINE_FRAC  (SIGDIGGER_AGC_FAST_RISE_FRAC * 10)
#define SIGDIGGER_AGC_MAG_HISTORY_FRAC (SIGDIGGER_AGC_FAST_RISE_FRAC * 10)

#define SIGDIGGER_AGC_BLOCK_LENGTH 4096


AGCTask::AGCTask(
    const SUCOMPLEX *data,
    SUCOMPLEX *destination,
    size_t length,
    SUFLOAT tau,
    QObject *parent) :
  Suscan::CancellableTask(parent)
{
  struct su_agc_params agc_params = su_agc_params_INITIALIZER;

  agc_params.fast_rise_t = tau * SIGDIGGER_AGC_FAST_RISE_FRAC;
  agc_params.fast_fall_t = tau * SIGDIGGER_AGC_FAST_FALL_FRAC;
  agc_params.slow_rise_t = tau * SIGDIGGER_AGC_SLOW_RISE_FRAC;
  agc_params.slow_fall_t = tau * SIGDIGGER_AGC_SLOW_FALL_FRAC;
  agc_params.hang_max    = tau * SIGDIGGER_AGC_HANG_MAX_FRAC;

  this->origin = data;
  this->destination = destination;
  this->length = length;

  SU_ATTEMPT(su_agc_init(&this->agc, &agc_params));

  this->agcInitialized = true;

  this->setProgress(0);
  this->setStatus("Processing...");
}

bool
AGCTask::work(void)
{
  size_t amount = this->length - this->p;
  size_t p = this->p;

  if (amount > SIGDIGGER_AGC_BLOCK_LENGTH)
    amount = SIGDIGGER_AGC_BLOCK_LENGTH;

  while (amount--) {
    this->destination[p] = su_agc_feed(&this->agc, this->origin[p]);
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
AGCTask::cancel(void)
{
  emit cancelled();
}

AGCTask::~AGCTask()
{
  if (this->agcInitialized)
    su_agc_finalize(&this->agc);
}
