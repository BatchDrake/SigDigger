//
//    CarrierXlator.h: Translate central frequency
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
#ifndef CARRIERXLATOR_H
#define CARRIERXLATOR_H

#include <Suscan/CancellableTask.h>

#include <sigutils/types.h>
#include <sigutils/ncqo.h>

#define SIGDIGGER_CARRIER_XLATOR_BLOCK_LENGTH 4096

namespace SigDigger {
  class CarrierXlator : public Suscan::CancellableTask {
    Q_OBJECT

    const SUCOMPLEX *origin = nullptr;
    SUCOMPLEX       *destination = nullptr;

    size_t length;
    size_t p = 0;

    su_ncqo_t ncqo;

  public:
    CarrierXlator(
        const SUCOMPLEX *data,
        SUCOMPLEX *destination,
        size_t length,
        SUFLOAT relFreq,
        SUFLOAT phase,
        QObject *parent = nullptr);
    virtual ~CarrierXlator() override;

    virtual bool work(void) override;
    virtual void cancel(void) override;
  };
}

#endif // CARRIERXLATOR_H
