//
//    include/Scanner.h: Description
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
#ifndef SCANNER_H
#define SCANNER_H

#include <QObject>
#include <Suscan/Analyzer.h>

#define SIGDIGGER_SCANNER_SPECTRUM_SIZE     16384
#define SIGDIGGER_SCANNER_DEFAULT_BIN_VALUE -200.0f
#define SIGDIGGER_SCANNER_MIN_BIN_VALUE     -150.0f

#define SIGDIGGER_SCANNER_COUNT_MAX         5.0f
#define SIGDIGGER_SCANNER_COUNT_RESET       1.0f

namespace SigDigger {
  //
  // A SpectrumView represents a portion of the electromagnetic
  // spectrum that is updated through FFT messages. Every FFT message
  // covers fftBandwidth Hz. This implies the following:
  //
  // - A SpectrumView requires (freqMax - freqMin) / fftBandwidth to be
  // complete. This is the fftCount value. Therefore:
  //
  // fftCount < SIGDIGGER_SCANNER_SPECTRUM_SIZE.
  //   Simple linear interpolation scenario. There is more than one bin
  //   per FFT (bpfft = SIGDIGGER_SCANNER_SPECTRUM_SIZE / fftCount > 1).
  //   The FFT must be scaled down to bpfft values, this is, we must average
  //   SIGDIGGER_SCANNER_SPECTRUM_SIZE / bpfft = fftCount values. Since
  //   both fftCount and bpfft are real values, we follow a softened approach:
  //
  //   1. We average linearly the FFT in blocks of fftCount values. The last
  //      value is going to be smaller as it is noit going to fit an entire bin.
  //      The result is placed in averaged.
  //   2. We add the averaged PSD to the PSD, interpolating linearly depending
  //      on the bin start.
  //   3. We increment count accordingly, interpolating linearly again. All
  //      PSD values contribute with a count of 1, except the last one,
  //      which is smaller than one.
  //
  // fftCount >= SIGDIGGER_SCANNER_SPECTRUM_SIZE.
  //  Simple histogram scenario. We average the PSD and increment the number
  //  of updates in the count array.
  //
  struct SpectrumView {
      SUFREQ freqMin = 0;
      SUFREQ freqMax = 0;
      SUFREQ freqRange = 0;

      SUFREQ fftBandwidth = 0;
      SUFLOAT fftRelBw = .5f;

      SUFLOAT psd[SIGDIGGER_SCANNER_SPECTRUM_SIZE];

      // TODO: Use complex?
      SUFLOAT psdAccum[SIGDIGGER_SCANNER_SPECTRUM_SIZE];
      SUFLOAT psdCount[SIGDIGGER_SCANNER_SPECTRUM_SIZE];

      SpectrumView();

      void setRange(SUFREQ freqMin, SUFREQ freqMax);

      void feed(
          const SUFLOAT *,
          const SUFLOAT *,
          SUSCOUNT psdSize,
          SUFREQ freqMin,
          SUFREQ freqMax,
          bool adjustSides = true);

      void feed(
          const SUFLOAT *,
          const SUFLOAT *,
          SUSCOUNT psdSize,
          SUFREQ center,
          bool adjustSides = true);

      void feed(SpectrumView const &);

      void reset(void);
      void interpolate(void); // Interpolate empty bins

    private:
      void feedLinearMode(
          const SUFLOAT *,
          const SUFLOAT *,
          SUSCOUNT psdSize,
          SUFREQ freqMin,
          SUFREQ freqMax,
          bool adjustSides = true);

      void feedHistogramMode(
          const SUFLOAT *,
          SUSCOUNT psdSize,
          SUFREQ freqMin,
          SUFREQ freqMax);
  };

  class Scanner : public QObject
  {
      Q_OBJECT

      SUFREQ freqMin;
      SUFREQ freqMax;
      SUFREQ lnb;

      bool fsGuessed = false;
      unsigned int fs = 0;
      unsigned int rtt = 15;
      unsigned int fftSize = 8192;
      SpectrumView views[2];
      int view = 0;

      Suscan::Analyzer *analyzer = nullptr;

    public:
      explicit Scanner(
          QObject *parent,
          SUFREQ freqMin,
          SUFREQ freqMax,
          Suscan::Source::Config const &cfg);

      void setRelativeBw(float ratio);
      void setRttMs(unsigned int);
      void setViewRange(SUFREQ min, SUFREQ max, bool noHop = false);
      void setStrategy(Suscan::Analyzer::SweepStrategy);
      void setPartitioning(Suscan::Analyzer::SpectrumPartitioning);
      void setGain(QString const &, float);

      unsigned int getFs(void) const;
      void flip(void);
      SpectrumView &getSpectrumView(void);
      SpectrumView const &getSpectrumView(void) const;
      void stop(void);

      ~Scanner();

    signals:
      void spectrumUpdated(void);
      void stopped(void);

    public slots:
      void onPSDMessage(const Suscan::PSDMessage &);
      void onAnalyzerHalted(void);

  };
}

#endif // SCANNER_H
