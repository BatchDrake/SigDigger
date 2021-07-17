//
//    CarrierDetector.cpp: Find dominant frequency and center
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
#include "CarrierDetector.h"
#include <sigutils/taps.h>

using namespace SigDigger;

CarrierDetector::CarrierDetector(
    const SUCOMPLEX *data,
    size_t len,
    qreal avgRelBw,
    qreal dcNotchRelBw,
    QObject *parent) : CancellableTask(parent)
{
  this->data = data;
  this->len = len;
  this->avgRelBw = avgRelBw;
  this->setProgress(0);
  this->dcNotchRelBw = qBound(0., dcNotchRelBw, 1.);

  this->setStatus("Estimating best FFT plan");
}

CarrierDetector::~CarrierDetector()
{
  if (this->plan != nullptr)
    SU_FFTW(_destroy_plan)(this->plan);

  if (this->buffer != nullptr)
    SU_FFTW(_free)(this->buffer);
}

bool
CarrierDetector::work(void)
{
  // Initializing state
  switch (this->state) {
    case ESTIMATING:
      while (this->allocation < this->len)
        this->allocation <<= 1;

      if ((this->buffer = static_cast<SU_FFTW(_complex) *>(
             SU_FFTW(_malloc)(this->allocation * sizeof(SUCOMPLEX)))) == nullptr) {
        emit error(
              "Failed to allocate "
              + QString::number(this->allocation)
              + " complex samples.");
        return false;
      }

      if ((this->plan = SU_FFTW(_plan_dft_1d)(
             this->allocation,
             this->buffer,
             this->buffer,
             FFTW_FORWARD,
             FFTW_ESTIMATE)) == nullptr) {
        emit error("Failed to initialize FFT plan.");
        return false;
      }

      this->transitionTo(COPYING);
      break;

    case COPYING:
      memcpy(this->buffer, this->data, this->len * sizeof(SUCOMPLEX));
      memset(
            this->buffer + this->len,
            0,
            (this->allocation - this->len) * sizeof(SUCOMPLEX));

      su_taps_apply_blackmann_harris_complex(
            reinterpret_cast<SUCOMPLEX *>(this->buffer),
            static_cast<SUSCOUNT>(this->len));
      this->transitionTo(EXECUTING);
      break;

    case EXECUTING:
      SU_FFTW(_execute)(this->plan);
      this->transitionTo(COMPUTING);
      break;

    case COMPUTING:
      int i;
      int maxNdx = 0;
      int bins = static_cast<int>(this->allocation * this->avgRelBw) + 1;
      int delta = (bins - 1) / 2;
      int start;
      int skipLen = static_cast<int>(.5 * this->dcNotchRelBw * this->allocation);
      SUFLOAT maxVal = 0;
      SUCOMPLEX *asSuComplex = reinterpret_cast<SUCOMPLEX *>(this->buffer);
      SUFLOAT psd;
      SUCOMPLEX acc = 0;

      // Find maximum
      for (i = skipLen; i < static_cast<int>(this->allocation) - skipLen; ++i) {
        asSuComplex[i] *= SU_C_CONJ(asSuComplex[i]);
        psd = SU_C_REAL(asSuComplex[i]);
        if (psd > maxVal) {
          maxVal = psd;
          maxNdx = i;
        }
      }

      // Compute centroid.
      start = maxNdx - delta;

      for (i = 0; i < bins; ++i) {
        int j = i + start;
        if (j < 0)
          j += this->allocation;

        j %= this->allocation;

        psd = SU_C_REAL(asSuComplex[j]);
        SUFLOAT nFreq = 2.f * j / static_cast<SUFLOAT>(this->allocation);

        acc += psd * SU_C_EXP(I * SU_ASFLOAT(M_PI) * nFreq);
      }

      // Finish
      this->peak = SU_C_ARG(acc);

      if (this->peak > M_PI)
        this->peak -= 2 * M_PI;

      emit done();
      return false;
  }

  return true;
}

void
CarrierDetector::cancel(void)
{
  emit cancelled();
}
