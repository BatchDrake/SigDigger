//
//    CostasRecoveryTask.h: Carrier recovery for PSK signals
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
#ifndef COSTASRECOVERYTASK_H
#define COSTASRECOVERYTASK_H

#include <Suscan/CancellableTask.h>
#include <sigutils/pll.h>

#ifndef NULL
#  define NULL nullptr
#endif // NULL

class CostasRecoveryTask : public Suscan::CancellableTask
{
  Q_OBJECT

  const SUCOMPLEX *origin = nullptr;
  SUCOMPLEX       *destination = nullptr;

  size_t length;
  size_t p = 0;

  su_costas_t costas = su_costas_INITIALIZER;

  bool costasInitialized = false;

public:
  explicit CostasRecoveryTask(
      const SUCOMPLEX *data,
      SUCOMPLEX *destination,
      size_t length,
      SUFLOAT tau,
      SUFLOAT loopbw,
      enum sigutils_costas_kind kind = SU_COSTAS_KIND_BPSK,
      QObject *parent = nullptr);

  virtual ~CostasRecoveryTask();

  virtual bool work(void) override;
  virtual void cancel(void) override;
};

#endif // COSTASRECOVERYTASK_H
