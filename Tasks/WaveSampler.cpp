//
//    WaveSampler.cpp: Extract symbols from wave
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

#include <WaveSampler.h>
#include <Suscan/Library.h>
#include <sigutils/sampling.h>
#include <sigutils/taps.h>
#include <SuWidgetsHelpers.h>

using namespace SigDigger;

Q_DECLARE_METATYPE(SigDigger::WaveSampleSet);

static bool registered;

WaveSampler::WaveSampler(
    SamplingProperties const &props,
    const Decider *decider,
    QObject *parent) : CancellableTask(parent)
{
  if (!registered) {
    qRegisterMetaType<SigDigger::WaveSampleSet>();
    registered = true;
  }
  // The number of symbols is already decided on properties
  this->properties = props;
  this->decider    = decider;

  this->delta = props.length / props.symbolCount;
  this->sampOffset = this->properties.symbolSync / delta;

  this->bnor = SU_ABS2NORM_BAUD(props.fs, props.rate);

  if (this->bnor > 1)
    this->bnor = 1;

  // Gardner by default
  if (this->properties.sync == SamplingClockSync::GARDNER) {
#ifdef SIGDIGGER_WAVESAMPLER_USE_MF
    SUFLOAT tau = 1. / bnor;
    unsigned span;
#endif // SIGDIGGER_WAVESAMPLER_USE_MF

    SU_ATTEMPT(
          su_clock_detector_init(
          &this->cd,
          props.loopGain,
          this->bnor,
          SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH) != -1);
    this->cdInit = true;

#ifdef SIGDIGGER_WAVESAMPLER_USE_MF
    span = tau * SIGDIGGER_WAVESAMPLER_MF_PERIODS > SIGDIGGER_WAVESAMPLER_MAX_MF_SPAN
        ? SIGDIGGER_WAVESAMPLER_MAX_MF_SPAN
        : tau * SIGDIGGER_WAVESAMPLER_MF_PERIODS;

    SU_ATTEMPT(
          su_iir_rrc_init(
            &this->mf,
            SIGDIGGER_WAVESAMPLER_MF_PERIODS,
            tau,
            props.loopGain));
    this->mfInit = true;
#endif // SIGDIGGER_WAVESAMPLER_USE_MF
  }

}

WaveSampler::~WaveSampler()
{
  if (this->cdInit)
    su_clock_detector_finalize(&this->cd);

#ifdef SIGDIGGER_WAVESAMPLER_USE_MF
  if (this->mfInit)
    su_iir_filt_finalize(&this->mf);
#endif // SIGDIGGER_WAVESAMPLER_USE_MF
}

bool
WaveSampler::sampleManual(void)
{
  long amount = static_cast<long>(this->properties.symbolCount) - this->p;
  long p = this->p;

  unsigned int q = 0;
  SUFLOAT start, end;
  SUFLOAT tStart, tEnd;
  SUFLOAT deltaInv = 1.f / static_cast<SUFLOAT>(this->delta);
  long iStart, iEnd;

  SUCOMPLEX avg;
  SUCOMPLEX x = 0, prev = this->prevSample;

  if (amount > SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH)
    amount = SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH;

  while (amount--) {
    start = static_cast<SUFLOAT>(
          (p++ - this->sampOffset) * this->delta + this->properties.symbolSync);
    end = start + static_cast<SUFLOAT>(this->delta);
    avg = 0;

    iStart = static_cast<long>(std::floor(start));
    iEnd   = static_cast<long>(std::ceil(end));

    tStart = 1 - (start - iStart);
    tEnd   = 1 - (iEnd  - end);

    // Average all symbols between start and end. This is actually some
    // terrible filtering algorithm, but it should work

    for (auto i = iStart; i <= iEnd; ++i) {
      if (i >= 0 && i < static_cast<long>(this->properties.length)) {
        if (i == iStart)
          x = tStart * this->properties.data[i];
        else if (i == iEnd)
          x = tEnd * this->properties.data[i];
        else
          x = this->properties.data[i];
      } else {
        x = 0;
      }

      if (this->properties.space == FREQUENCY)
        avg += x * SU_C_CONJ(prev);
      else
        avg += x;

      prev = x;
    }

    this->set.block[q++] = deltaInv * avg;
  }

  this->prevSample = prev;

  this->p = p;
  this->set.len = q;

  this->progress = this->p / this->properties.symbolCount;

  return this->p < static_cast<long>(this->properties.symbolCount);
}

bool
WaveSampler::sampleGardner(void)
{
  long amount = static_cast<long>(this->properties.length) - this->p;
  long p = this->p;
  SUCOMPLEX x = 0, prev = this->prevSample;
  SUSDIFF count;

  if (amount > SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH)
    amount = SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH;

  if (this->properties.space == FREQUENCY) {
    // Perform quadrature demodulation directly in here.
    while (amount--) {
      x = this->properties.data[p++];
      su_clock_detector_feed(&this->cd, x * SU_C_CONJ(prev));
      prev = x;
    }

    this->prevSample = prev;
  } else {
    while (amount--)
      su_clock_detector_feed(&this->cd, this->properties.data[p++]);
  }

  count = su_clock_detector_read(
        &this->cd,
        this->set.block,
        SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH);

  this->p = p;
  this->set.len = static_cast<size_t>(count);

  this->progress = this->p / static_cast<qreal>(this->properties.length);

  return this->p < static_cast<long>(this->properties.length);
}

bool
WaveSampler::sampleZeroCrossing(void)
{
  long amount = SCAST(long, this->properties.length) - this->p;
  long p = this->p;
  bool last = false;
  long i = 0;
  SUCOMPLEX x = 0, prev = this->prevSample;
  SUFLOAT var = 0, prevVar = this->prevVar;

  if (amount > SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH)
    amount = SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH;

  last = p + amount >= SCAST(long, this->properties.length);

  this->set.len = 0;

  while (amount--) {
    switch (this->properties.space) {
      case AMPLITUDE:
        var = SU_C_REAL(
              this->properties.data[p] * this->properties.zeroCrossingAngle);
        break;

      case PHASE:
        var = SU_C_ARG(
              this->properties.data[p] * this->properties.zeroCrossingAngle);
        break;

      case FREQUENCY:
        x = this->properties.data[p];
        var = SU_C_ARG(I * x * SU_C_CONJ(prev));
        prev = x;
        break;
    }

    if ((var > 0 || var < 0) || last) {
      /* Zero crossing? */
      if (var * prevVar < 0 || last) {
        long samples = p - this->lastZc;
        long symbols = SCAST(long, round(samples * this->bnor));
        printf("Run of %d symbols equal to %d\n", symbols, var > 0);

        while (symbols-- > 0 && i < SIGDIGGER_WAVESAMPLER_FEEDER_BLOCK_LENGTH) {
          this->set.block[i]   = var > 0;
          this->set.symbols[i] = var > 0;
          ++i;
        }

        this->set.len = i;
        this->lastZc = p;
        prevVar = var;
      }
    }

    ++p;
  }

  this->p = p;
  this->progress = this->p / static_cast<qreal>(this->properties.length);

  return this->p < static_cast<long>(this->properties.length);
}

bool
WaveSampler::work(void)
{
  bool more = false;
  bool decided = false;

  switch (this->properties.sync) {
    case MANUAL:
      more = this->sampleManual();
      break;

    case GARDNER:
      more = this->sampleGardner();
      break;

    case ZERO_CROSSING:
      more = this->sampleZeroCrossing();
      decided = true;
      break;
  }

  // Perform decision, if necessary
  if (!decided)
    this->decider->decide(this->set.block, this->set.symbols, this->set.len);

  this->setStatus("Demodulating ("
                  + QString::number(static_cast<int>(this->progress * 100))
                  + ")...");

  this->setProgress(this->progress);

  // Deliver data
  if (this->set.len > 0)
    emit data(this->set);

  if (!more)
    emit done();

  return more;
}

void
WaveSampler::cancel(void)
{
  emit cancelled();
}
