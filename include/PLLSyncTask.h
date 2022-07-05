//
//    PLLSyncTask.h: Synchronize to carrier
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
#ifndef PLLSYNCTASK_H
#define PLLSYNCTASK_H

#include <Suscan/CancellableTask.h>
#include <sigutils/pll.h>

#ifndef NULL
#  define NULL nullptr
#endif // NULL

class PLLSyncTask : public Suscan::CancellableTask
{
  Q_OBJECT

  const SUCOMPLEX *origin = nullptr;
  SUCOMPLEX       *destination = nullptr;

  size_t length;
  size_t p = 0;

  su_pll_t pll = su_pll_INITIALIZER;
  bool pllInitialized = false;

public:
  explicit PLLSyncTask(
      const SUCOMPLEX *data,
      SUCOMPLEX *destination,
      size_t length,
      SUFLOAT cutoff,
      QObject *parent = nullptr);

  virtual ~PLLSyncTask() override;

  virtual bool work(void) override;
  virtual void cancel(void) override;
};

#endif // PLLSYNCTASK_H
