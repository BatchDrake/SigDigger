//
//    AnalyzerParams.h: Analyzer parameters
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

#ifndef CPP_ANALYZERPARAMS_H
#define CPP_ANALYZERPARAMS_H

#include <Suscan/Serializable.h>

#include <analyzer/analyzer.h>

namespace Suscan {
  class AnalyzerParams : public Serializable
  {
      struct suscan_analyzer_params c_params =
          suscan_analyzer_params_INITIALIZER;

      void updateExposedParams(void);

    public:
      enum WindowFunction {
        NONE = SU_CHANNEL_DETECTOR_WINDOW_NONE,
        HAMMING = SU_CHANNEL_DETECTOR_WINDOW_HAMMING,
        HANN = SU_CHANNEL_DETECTOR_WINDOW_HANN,
        FLAT_TOP = SU_CHANNEL_DETECTOR_WINDOW_FLAT_TOP,
        BLACKMANN_HARRIS = SU_CHANNEL_DETECTOR_WINDOW_BLACKMANN_HARRIS
      };

      enum Mode {
        CHANNEL = SUSCAN_ANALYZER_MODE_CHANNEL,
        WIDE_SPECTRUM = SUSCAN_ANALYZER_MODE_WIDE_SPECTRUM
      };

      float channelUpdateInterval;
      float psdUpdateInterval;
      float spectrumAvgAlpha;
      float sAvgAlpha;
      float nAvgAlpha;
      float snr;
      double minFreq;
      double maxFreq;
      unsigned int windowSize;
      WindowFunction windowFunction;
      Mode mode;

      struct suscan_analyzer_params const &getCParams(void);

      void deserialize(Object const &conf) override;
      Object &&serialize(void) override;

      AnalyzerParams();
      AnalyzerParams(struct suscan_analyzer_params const &params);
  };
}

#endif // CPP_ANALYZERPARAMS_H
