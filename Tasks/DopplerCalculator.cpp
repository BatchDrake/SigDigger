//
//    DopplerCalculator.cpp: Compute Doppler effect
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
#include "DopplerCalculator.h"
#include <sigutils/taps.h>
#include <sigutils/sampling.h>

#define SPEED_OF_LIGHT 299792458.

using namespace SigDigger;

DopplerCalculator::DopplerCalculator(
    SUFREQ f0,
    const SUCOMPLEX *data,
    size_t len,
    SUFLOAT fs,
    QObject *parent) : CancellableTask(parent)
{
  this->f0   = f0;
  this->data = data;
  this->len  = len;
  this->fs   = fs;
  this->setProgress(0);

  this->setStatus("Estimating best FFT plan");
}

DopplerCalculator::~DopplerCalculator()
{
  if (this->plan != nullptr)
    SU_FFTW(_destroy_plan)(this->plan);

  if (this->buffer != nullptr)
    SU_FFTW(_free)(this->buffer);
}

bool
DopplerCalculator::work(void)
{
  // Initializing state
  switch (this->state) {
    case ESTIMATING:
      while (this->allocation < this->len)
        this->allocation <<= 1;

      this->psd.resize(this->allocation);

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
      this->transitionTo(COMPUTE);
      break;

    case COMPUTE:
      int i;
      int maxNdx = 0;
      int bins = static_cast<int>(this->allocation);
      int delta = bins / 2;
      int start;
      SUFLOAT maxVal = 0;
      SUCOMPLEX *asSuComplex = reinterpret_cast<SUCOMPLEX *>(this->buffer);
      SUFLOAT psd;
      SUCOMPLEX acc = 0;
      SUFLOAT peak;
      SUFLOAT lambda = static_cast<SUFLOAT>(SPEED_OF_LIGHT / this->f0);
      SUFLOAT dispAcc = 0;
      SUFLOAT totalEnergy = 0;
      SUFLOAT err = 0, t, y;

      // Find maximum
      for (i = 0; i < bins; ++i) {
        asSuComplex[i] *= SU_C_CONJ(asSuComplex[i]);
        psd = SU_C_REAL(asSuComplex[i]);
        if (psd > maxVal) {
          maxVal = psd;
          maxNdx = i;
        }

        this->psd[(static_cast<size_t>(bins - i) + delta) % bins] = psd;

        // Accumulate with Kahan
        y = psd - err;
        t = totalEnergy + y;
        err = (t - totalEnergy) - y;
        totalEnergy = t;
      }

      this->max = maxVal;

      // Compute centroid and dispersion.
      start = maxNdx - delta;

      for (i = 0; i < bins; ++i) {
        int64_t j = i + start;
        if (j < 0)
          j += this->allocation;

        j %= this->allocation;

        psd = SU_C_REAL(asSuComplex[j]);
        SUFLOAT nFreq = 2.f * j / static_cast<SUFLOAT>(this->allocation);

        acc += psd * SU_C_EXP(SU_I * SU_ASFLOAT(M_PI) * nFreq);

        // Correct j to make it centered around 0
        j = i;
        if (j >= delta)
          j -= bins;

        // Divide by delta instead of bins to work with normalized frequencies
        // (in half cycles per sample)
        dispAcc += (j * j * psd / totalEnergy) / (delta * delta);
      }

      // Finish
      peak = SU_C_ARG(acc);
      if (peak > PI)
        peak -= 2 * PI;
      peak = SU_NORM2ABS_FREQ(this->fs, SU_ANG2NORM_FREQ(peak));
      this->peak = -lambda * peak;

      // Sigma provides a measure of the standard deviation of velocities.
      // What we have in dispAcc is a normalized frequency variance,
      // with differences:
      this->sigma = SU_NORM2ABS_FREQ(this->fs, std::sqrt(dispAcc));

      emit done();
      return false;

  }

  return true;
}

void
DopplerCalculator::cancel(void)
{
  emit cancelled();
}
