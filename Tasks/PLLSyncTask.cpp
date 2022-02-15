//
//    PLLSyncTask.cpp: PLL-based carrier recovery
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
#include <PLLSyncTask.h>
#include <Suscan/Library.h>

#define SIGDIGGER_COSTAS_BLOCK_LENGTH 4096

PLLSyncTask::PLLSyncTask(
    const SUCOMPLEX *data,
    SUCOMPLEX *destination,
    size_t length,
    SUFLOAT bw,
    QObject *parent) :
  Suscan::CancellableTask(parent)
{
  this->origin = data;
  this->destination = destination;
  this->length = length;

  SU_ATTEMPT(su_pll_init(&this->pll, 0, bw));

  this->pllInitialized = true;

  this->setProgress(0);
  this->setStatus("Processing...");
}

bool
PLLSyncTask::work(void)
{
  size_t amount = this->length - this->p;
  size_t p = this->p;

  if (amount > SIGDIGGER_COSTAS_BLOCK_LENGTH)
    amount = SIGDIGGER_COSTAS_BLOCK_LENGTH;

  while (amount--) {
    this->destination[p] = su_pll_track(&this->pll, this->origin[p]);
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
PLLSyncTask::cancel(void)
{
  emit cancelled();
}

PLLSyncTask::~PLLSyncTask()
{
  if (this->pllInitialized)
    su_pll_finalize(&this->pll);
}
