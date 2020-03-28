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

  // Gardner by default
  if (this->properties.sync == SamplingClockSync::GARDNER) {
    SUFLOAT bnor = SU_ABS2NORM_BAUD(props.fs, props.rate);
#ifdef SIGDIGGER_WAVESAMPLER_USE_MF
    SUFLOAT tau = 1. / bnor;
    unsigned span;
#endif // SIGDIGGER_WAVESAMPLER_USE_MF

    SU_ATTEMPT(
          su_clock_detector_init(
          &this->cd,
          props.loopGain,
          bnor,
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
WaveSampler::work(void)
{
  bool more;

  if (this->properties.sync == SamplingClockSync::MANUAL)
    more = this->sampleManual();
  else
    more = this->sampleGardner();

  // Perform decision
  this->decider->decide(this->set.block, this->set.symbols, this->set.len);

  this->setStatus("Demodulating ("
                  + QString::number(static_cast<int>(this->progress * 100))
                  + ")...");

  this->setProgress(this->progress);

  // Deliver data
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
