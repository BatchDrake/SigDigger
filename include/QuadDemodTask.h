//
//    filename: description
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
#ifndef QUADDEMODTASK_H
#define QUADDEMODTASK_H

#include <Suscan/CancellableTask.h>
#include <sigutils/types.h>

class QuadDemodTask : public Suscan::CancellableTask
{
  Q_OBJECT

  const SUCOMPLEX *origin = nullptr;
  SUCOMPLEX       *destination = nullptr;
  SUCOMPLEX prev = 0;

  size_t length;
  size_t p = 0;

  bool costasInitialized = false;

public:
  explicit QuadDemodTask(
      const SUCOMPLEX *data,
      SUCOMPLEX *destination,
      size_t length,
      QObject *parent = nullptr);

  virtual ~QuadDemodTask() override;

  virtual bool work(void) override;
  virtual void cancel(void) override;
};

#endif // QUADDEMODTASK_H
