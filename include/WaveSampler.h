//
//    WaveSampler.h: Extract symbols from wave
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
#ifndef WAVESAMPLER_H
#define WAVESAMPLER_H

#include "CancellableTask.h"
#include "SamplingProperties.h"
#include "Decider.h"

#define SIGDIGGER_HISTOGRAM_FEEDER_BLOCK_LENGTH 4096

namespace SigDigger {
  struct WaveSampleSet {
    SUCOMPLEX block[SIGDIGGER_HISTOGRAM_FEEDER_BLOCK_LENGTH];
    Symbol symbols[SIGDIGGER_HISTOGRAM_FEEDER_BLOCK_LENGTH];
    size_t len;
  };

  class WaveSampler : public CancellableTask {
    Q_OBJECT

    const Decider *decider;
    SamplingProperties properties;
    long p = 0;
    qreal delta = 0;
    qreal sampOffset;
    SUCOMPLEX prevAvg = 0;

    WaveSampleSet set;

  public:
    WaveSampler(
        SamplingProperties const &props,
        const Decider *decider,
        QObject *parent = nullptr);
    virtual ~WaveSampler() override;

    virtual bool work(void) override;
    virtual void cancel(void) override;


  signals:
    void data(SigDigger::WaveSampleSet);
  };
}

#endif // WAVESAMPLER_H
