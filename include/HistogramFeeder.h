//
//    HistogramFeeder.h: Translate central frequency
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
#ifndef HISTOGRAMFEEDER_H
#define HISTOGRAMFEEDER_H

#include <Suscan/CancellableTask.h>
#include "SamplingProperties.h"

#define SIGDIGGER_HISTOGRAM_FEEDER_BLOCK_LENGTH 4096

namespace SigDigger {
  class HistogramFeeder : public Suscan::CancellableTask {
    Q_OBJECT

    SamplingProperties properties;
    size_t p = 0;

    SUFLOAT block[SIGDIGGER_HISTOGRAM_FEEDER_BLOCK_LENGTH];

  public:
    HistogramFeeder(
        SamplingProperties const &props,
        QObject *parent = nullptr);
    virtual ~HistogramFeeder() override;

    virtual bool work(void) override;
    virtual void cancel(void) override;


  signals:
    void data(const float *data, unsigned int size);
  };
}

#endif // HISTOGRAMFEEDER_H
