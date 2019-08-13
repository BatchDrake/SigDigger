//
//    SNREstimator.cpp: SNR Estimator
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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

#include "SNREstimator.h"
#include <cstdio>

using namespace SigDigger;

SNREstimator::SNREstimator()
{

}

void
SNREstimator::recalculateModel(void)
{
  if (this->length > 0 && this->intervals > 0) {
    unsigned int i, j;
    float x;
    float max = 0;
    float intlen, start;
    float sigma2 = this->sigma * this->sigma;

    // Step 1: compute gaussian
    for (i = 0; i < this->length; ++i) {
      x = i * this->hx;
      if (x >= .5f)
        x -= 1.f;

      this->gaussian[i] = expf(-x * x / sigma2);
    }

    // Step 2: Repeat in through the whole interval, N times
    intlen = 1.f  / this->intervals;
    start = .5f * intlen;

    std::fill(this->Hi.begin(), this->Hi.end(), 0.f);

    for (j = 0; j < this->intervals; ++j) {
      float skip = start + j * intlen;
      float t = 1.f - (skip - floorf(skip));
      unsigned int skipint = static_cast<unsigned>(floorf(this->length * skip));
      unsigned int i1, i2;
      for (i = 0; i < this->length; ++i) {
        i1 = static_cast<unsigned>(this->length + i - skipint) % this->length;
        i2 = static_cast<unsigned>(this->length + i1 - 1) % this->length;

        this->Hi[i] += t * this->gaussian[i1];
        this->Hi[i] += (1 - t) * this->gaussian[i2];
      }
    }

    // Step 3: Normalize
    for (i = 0; i < this->length; ++i)
      if (this->Hi[i] > max)
        max = this->Hi[i];

    if (max > 0.f)
      for (i = 0; i < this->length; ++i)
        this->Hi[i] /= max;
  }
}

void
SNREstimator::iterate()
{
  if (this->length > 0 && this->intervals > 0) {
    float delta = 0, x;
    float term;
    unsigned int i, j;
    float intlen, start;
    float skip;
    float sigmainv = 1.f / this->sigma;
    float sigma3inv = sigmainv * sigmainv * sigmainv;

    this->recalculateModel();

    intlen = 1.f  / this->intervals;
    start = .5f * intlen;

    for (i = 0; i < this->length; ++i) {
      x = i * this->hx;
      if (x >= .5f)
        x -= 1.f;

      term = 0;
      for (j = 0; j < this->intervals; ++j) {
        skip = start + j * intlen;
        term += (x - skip) * (x - skip);
      }

      term *= (this->Hi[i] - this->Htilde[i]) / sigma3inv;

      delta += term;
    }

    this->delta = delta / this->length;
    this->sigma += -this->alpha * this->delta;
    this->dirty = true;
  }
}

void
SNREstimator::calculateSquareError(void)
{
  if (this->length && this->intervals > 0) {
    float err;
    this->sqerr = 0;
    for (unsigned i = 0; i < this->length; ++i) {
      err = (this->Hi[i] - this->Htilde[i]) * (this->Hi[i] - this->Htilde[i]);
      this->sqerr += err * err;
    }

    this->dirty = false;
  }
}

void
SNREstimator::setBps(unsigned int bps)
{
  if (this->bps != bps) {
    this->bps = bps;
    this->sigma = SNR_ESTIMATOR_DEFAULT_SIGMA;
    this->intervals = 1 << bps;
    this->hx = 1.f / this->length;
  }
}

void
SNREstimator::feed(std::vector<unsigned int> const &history)
{
  unsigned int max = 0;

  if (this->length != history.size()) {
    this->length = static_cast<unsigned int>(history.size());
    this->gaussian.resize(this->length);
    this->Hi.resize(this->length);
    this->Htilde.resize(this->length);
    this->hx = 1.f / this->length;
  }

  for (unsigned int i = 0; i < history.size(); ++i)
    if (max < history[i])
      max = history[i];

  if (max == 0)
    max = 1;

  for (unsigned int i = 0; i < history.size(); ++i)
    this->Htilde[i] = static_cast<float>(history[i]) / max;

  this->iterate();
}

void
SNREstimator::setAlpha(float alpha)
{
  this->alpha = alpha;
}

void
SNREstimator::setSigma(float sigma)
{
  this->sigma = sigma;
}

