//
//    SNREstimator.h: SNR Estimator
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

#ifndef SNRESTIMATOR_H
#define SNRESTIMATOR_H

#include <cmath>
#include <vector>

//
// Note: this class assumes a normalized interval (with x in range [0, 1))
// SNR needs to be readjusted to the size of the decision interval
//

#define SNR_ESTIMATOR_DEFAULT_SIGMA (1.f / 8.f)
#define SNR_ESTIMATOR_DEFAULT_ALPHA 1.f

namespace SigDigger {
  class SNREstimator
  {
      float sigma = SNR_ESTIMATOR_DEFAULT_SIGMA;
      float alpha = SNR_ESTIMATOR_DEFAULT_ALPHA;

      unsigned int bps = 0;
      unsigned int intervals = 0;
      float hx = 0;
      float delta = 0;
      unsigned int length = 0;
      std::vector<float> gaussian;
      std::vector<float> Hi;     // Model histogram
      std::vector<float> Htilde; // Actual histogram
      float sqerr = INFINITY;
      bool dirty = false;
      void recalculateModel(void);
      void calculateSquareError(void);
      void iterate(void);

    public:
      SNREstimator();
      void setBps(unsigned int bps);
      void feed(std::vector<unsigned int> const &history);
      void setAlpha(float alpha);
      void setSigma(float sigma);

      std::vector<float> const &
      getModel(void) const
      {
        return this->Hi;
      }

      float
      getMSE(void)
      {
        if (this->dirty)
          this->calculateSquareError();

        return this->sqerr / this->length;
      }

      float
      getSigma(void) const
      {
        return this->sigma;
      }

      float
      getSNR(void) const
      {
        return 1.f / (this->intervals * this->sigma);
      }
  };
}

#endif // SNRESTIMATOR_H
