//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
#ifndef DopplerCalculator_H
#define DopplerCalculator_H

#include "CancellableTask.h"
#include <sigutils/types.h>

namespace SigDigger {
  class DopplerCalculator : public CancellableTask {
    Q_OBJECT

    enum State {
      ESTIMATING,
      COPYING,
      EXECUTING,
      COMPUTE
    };

    State state             = ESTIMATING;
    const SUCOMPLEX   *data = nullptr;
    SU_FFTW(_plan)     plan = nullptr;
    SU_FFTW(_complex) *buffer = nullptr;
    std::vector<SUCOMPLEX> psd;
    SUFLOAT peak = 0;
    SUFLOAT sigma;
    SUFLOAT max;
    SUFLOAT fs;
    SUFREQ  f0;

    size_t len;
    size_t allocation = 1;

    State
    getState(void) const
    {
      return this->state;
    }

    void
    transitionTo(State s)
    {
      this->state = s;
      this->setProgress(static_cast<qreal>(s) / 3);

      switch (s) {
        case ESTIMATING:
          this->setStatus("Estimating best FFT plan");
          break;

        case COPYING:
          this->setStatus("Copying I/Q data to FFT buffer");
          break;

        case EXECUTING:
          this->setStatus("Executing FFT");
          break;

        case COMPUTE:
          this->setStatus("Computing dominant velocity and dispersion");
          break;
      }
    }

  public:
    DopplerCalculator(
        SUFREQ f0,
        const SUCOMPLEX *data,
        size_t len,
        SUFLOAT fs,
        QObject *parent = nullptr);
    virtual ~DopplerCalculator() override;

    virtual bool work(void) override;
    virtual void cancel(void) override;

    SUFLOAT
    getPeak(void) const
    {
      return this->peak;
    }

    SUFLOAT
    getSigma(void) const
    {
      return this->sigma;
    }

    std::vector<SUCOMPLEX> &&
    takeSpectrum(void)
    {
      return std::move(this->psd);
    }

    SUFLOAT
    getMax(void) const
    {
      return this->max;
    }
  };
}

#endif // DopplerCalculator_H
