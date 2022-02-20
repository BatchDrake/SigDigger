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

#include <Suscan/CancellableTask.h>
#include "SamplingProperties.h"
#include "Decider.h"
#include <sigutils/clock.h>
#include <sigutils/iir.h>

#define SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH 4096
#define SIGDIGGER_WAVESAMPLER_MAX_MF_SPAN         1024
#define SIGDIGGER_WAVESAMPLER_MF_PERIODS          6

namespace SigDigger {
  struct WaveSampleSet {
    SUCOMPLEX block[SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH];
    Symbol symbols[SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH];
    size_t len;
  };

  class WaveSampler : public Suscan::CancellableTask {
    Q_OBJECT

    const Decider *decider;
    SamplingProperties properties;
    su_clock_detector_t cd;
    bool cdInit = false;
    SUFLOAT prevVar = -1;
    SUFLOAT bnor = 0;

#ifdef SIGDIGGER_WAVESAMPLER_USE_MF
    su_iir_filt_t mf;
    bool mfInit = false;
#endif // SIGDIGGER_WAVESAMPLER_USE_MF
    long lastZc = 0;
    long p = 0;
    qreal delta = 0;
    qreal sampOffset;
    qreal progress;

    SUCOMPLEX prevSample = 0;

    WaveSampleSet set;

    bool sampleManual(void);
    bool sampleGardner(void);
    bool sampleZeroCrossing(void);

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
