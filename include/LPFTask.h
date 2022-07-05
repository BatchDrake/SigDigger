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
#ifndef LPFTASK_H
#define LPFTASK_H

#include <Suscan/CancellableTask.h>
#include <sigutils/specttuner.h>

class LPFTask : public Suscan::CancellableTask
{
  Q_OBJECT

  const SUCOMPLEX *origin = nullptr;
  SUCOMPLEX       *destination = nullptr;

  su_specttuner_t *stuner        = nullptr;
  su_specttuner_channel_t *schan = nullptr;

  size_t length;
  size_t p = 0; // Read pointer
  size_t q = 0; // Write pointer

  static SUBOOL
  onData(
        const struct sigutils_specttuner_channel *channel,
        void *privdata,
        const SUCOMPLEX *data,
        SUSCOUNT size);

public:
  explicit LPFTask(
      const SUCOMPLEX *data,
      SUCOMPLEX *destination,
      size_t length,
      SUFLOAT bw,
      QObject *parent = nullptr);

  virtual ~LPFTask() override;

  virtual bool work(void) override;
  virtual void cancel(void) override;
};

#endif // LPFTASK_H
