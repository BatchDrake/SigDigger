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
}

WaveSampler::~WaveSampler()
{
}

bool
WaveSampler::work(void)
{
  long amount = static_cast<long>(this->properties.symbolCount) - this->p;
  long p = this->p;
  unsigned int q = 0;
  SUFLOAT start, end;
  SUFLOAT tStart, tEnd;
  SUFLOAT deltaInv = 1.f / static_cast<SUFLOAT>(this->delta);
  long iStart, iEnd;

  SUCOMPLEX avg, prevAvg = 0;

  if (amount > SIGDIGGER_HISTOGRAM_FEEDER_BLOCK_LENGTH)
    amount = SIGDIGGER_HISTOGRAM_FEEDER_BLOCK_LENGTH;

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
          avg += tStart * this->properties.data[i];
        else if (i == iEnd)
          avg += tEnd * this->properties.data[i];
        else
          avg += this->properties.data[i];
      }
    }

    switch (this->properties.space) {
      case AMPLITUDE:
      case PHASE:
        this->set.block[q++] = deltaInv * avg;
        break;

      case FREQUENCY:
        this->set.block[q++] = -avg * SU_C_CONJ(prevAvg);
        break;
    }

    prevAvg = avg;
  }

  this->prevAvg = prevAvg;
  this->p = p;
  this->set.len = q;

  // Perform decision
  this->decider->decide(this->set.block, this->set.symbols, q);

  this->setStatus("Demodulating ("
                  + QString::number(p)
                  + "/"
                  + QString::number(this->properties.symbolCount)
                  + ")...");

  this->setProgress(
        static_cast<qreal>(p) / static_cast<qreal>(this->properties.symbolCount));

  // Deliver data
  emit data(this->set);

  if (this->p < static_cast<long>(this->properties.symbolCount))
    return true;

  emit done();
  return false;
}

void
WaveSampler::cancel(void)
{
  emit cancelled();
}
