//
//    Panoramic/Scanner.cpp: Description
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

#include "Scanner.h"
#include <cmath>
#include <algorithm>
#include <cassert>

using namespace SigDigger;

static inline unsigned int nextPow2(unsigned int n)
{
  // note: this will infinite loop for n > 2^31
  unsigned int i = 1;
  while (i < n)
    i <<= 1;
  return i;
}

SpectrumView::SpectrumView()
{
  this->reset();
}

void
SpectrumView::setRange(SUFREQ freqMin, SUFREQ freqMax)
{
  this->freqMin   = freqMin;
  this->freqMax   = freqMax;
  this->freqRange = freqMax - freqMin;

  this->spectrumSize = nextPow2(this->freqRange /
      SIGDIGGER_SCANNER_FREQ_RESOLUTION);
  if (this->spectrumSize > SIGDIGGER_SCANNER_SPECTRUM_SIZE)
    this->spectrumSize = SIGDIGGER_SCANNER_SPECTRUM_SIZE;

  this->reset();
}

void
SpectrumView::interpolate(void)
{
  unsigned int i, j;
  unsigned int count = 1;
  unsigned int zero_pos = 0;
  SUFLOAT t = 0;
  bool first = true;
  SUFLOAT left = SIGDIGGER_SCANNER_DEFAULT_BIN_VALUE;
  SUFLOAT right = SIGDIGGER_SCANNER_DEFAULT_BIN_VALUE;
  bool inGap = false;

  // Find a bin with zero entries, measure its width,
  // compute values in both ends and interpolate

  i = 0;

  for (i = 0; i < this->spectrumSize; ++i) {
    if (!inGap) {
      if (this->psdCount[i] <= .5f) {
        // Found zero!
        inGap = true;
        zero_pos = i;
        count = 1;

        first = i == 0;
        if (!first)
          left = this->psd[i - 1];

      } else {
        this->psd[i] = this->psdAccum[i] / this->psdCount[i];
        if (this->psdCount[i] > SIGDIGGER_SCANNER_COUNT_MAX) {
          this->psdCount[i]    = SIGDIGGER_SCANNER_COUNT_RESET;
          this->psdAccum[i] = this->psd[i] * SIGDIGGER_SCANNER_COUNT_RESET;
        }
      }
    } else {
      if (this->psdCount[i] <= .5f) {
        ++count;
      } else {
        // End of gap of zeroes. Compute right and interpolate
        inGap = false;
        right = this->psd[i] = this->psdAccum[i] / this->psdCount[i];
        if (first) {
          for (j = 0; j < count; ++j)
            this->psd[j + zero_pos] = right;
        } else {
          for (j = 0; j < count; ++j) {
            t = static_cast<SUFLOAT>(j + .5f) / count;
            this->psd[j + zero_pos] = (1 - t) * left + t * right;
          }
        }
      }
    }
  }

  // Deal with trailing zeroes, if any
  if (inGap)
    for (j = 0; j < count; ++j)
      this->psd[j + zero_pos] = right;
}

void
SpectrumView::feedLinearMode(
    const SUFLOAT *psdData,
    const SUFLOAT *countData,
    SUSCOUNT psdSize,
    SUFREQ freqMin,
    SUFREQ freqMax,
    bool adjustSides)
{
  SUFREQ inpBw, bw, freqSkip;
  double fftCount, bins, pos, delta;
  double srcBinW, dstBinW;
  int skip, j, k;

  // Compute subrange inside PSD message
  inpBw = freqMax - freqMin;
  if (adjustSides) {
    skip = static_cast<int>(
          .5f * (1 - this->fftRelBw) * psdSize);
  } else {
    skip = 0;
  }

  freqSkip = static_cast<SUFREQ>(skip) / psdSize * inpBw;
  bw = inpBw - 2 * freqSkip;
  assert(skip >= 0);

  // Compute dimension variables.
  fftCount  = static_cast<double>(this->freqRange / bw);  // How many FFTs fit in destination
  bins      = this->spectrumSize / fftCount; // Destination bin count
  srcBinW   = static_cast<double>(inpBw) / psdSize;
  dstBinW   = static_cast<double>(this->freqRange) / this->spectrumSize;
  delta     = dstBinW / srcBinW;                          // Source bins per destination bin

  pos = static_cast<double>(freqSkip + freqMin - this->freqMin) / (this->freqRange);
  pos *= this->spectrumSize;
  assert(!std::isnan(pos));

  // j is initial dest index, k is end of dest range
  j = pos > 0 ? static_cast<int>(pos) : 0;
  k = pos + bins < this->spectrumSize ?
    static_cast<int>(pos + bins) : this->spectrumSize;

  // linearly scale from source to destination frequency range and bin count
  while (j < k) {
    double  freqJ     = this->freqMin + dstBinW*j;
    double  srcBin    = (freqJ - freqMin) / srcBinW;
    int     startBin  = static_cast<int>(srcBin);
    int     endBin    = static_cast<int>(srcBin + delta);
    SUFLOAT psdAccum  = 0;
    SUFLOAT psdCount  = 0;

    startBin = std::clamp(startBin, 0, static_cast<int>(psdSize - 1));
    endBin = std::clamp(endBin, startBin + 1, static_cast<int>(psdSize));

    for (int i = startBin; i < endBin; i++) {
      psdAccum += psdData[i];
      psdCount += countData != nullptr ? countData[i] : 1;
    }

    if (psdCount > 0) {
      this->psdAccum[j] += psdAccum / psdCount;
      this->psdCount[j] += 1;
    }

    j++;
  }
}

void
SpectrumView::feedHistogramMode(
    const SUFLOAT *psdData,
    SUSCOUNT psdSize,
    SUFREQ freqMin,
    SUFREQ freqMax)
{
  SUFREQ relBw = (freqMax - freqMin) / this->freqRange;
  SUFREQ fStart = (freqMin - this->freqMin) / this->freqRange;
  SUFREQ fEnd   = (freqMax - this->freqMin) / this->freqRange;
  SUFLOAT t;
  SUFLOAT inv = 1. / psdSize;
  unsigned int i, j;
  SUFLOAT accum = 0;

  fStart *= this->spectrumSize;
  fEnd   *= this->spectrumSize;
  relBw  *= this->spectrumSize;

  j = std::clamp(
      static_cast<unsigned int>(fStart),
      static_cast<unsigned int>(0),
      static_cast<unsigned int>(this->spectrumSize - 1));

  // Now, relBw represents the relative size of the range
  // with respecto to the spectrum bin.

  for (i = 0; i < psdSize; ++i)
    accum += psdData[i];

  accum *= inv;

  assert(!std::isnan(inv));
  assert(!std::isnan(accum));

  if (floor(fStart) != floor(fEnd)) {
    // Between two bins.
    t = static_cast<SUFLOAT>((fStart - floor(fStart)) / relBw);

    this->psdCount[j] += 1 - t;
    this->psdAccum[j] += (1 - t) * accum;

    if (j + 1 < this->spectrumSize) {
      this->psdCount[j + 1] += t;
      this->psdAccum[j + 1] += t * accum;
    }
  } else {
    this->psdCount[j] += 1;
    this->psdAccum[j] += accum;
  }
}

void
SpectrumView::feed(
    const SUFLOAT *psd,
    const SUFLOAT *count,
    SUSCOUNT psdSize,
    SUFREQ freqMin,
    SUFREQ freqMax,
    bool adjustSides)
{
  SUFREQ fftCount = (freqMax - freqMin) / this->freqRange;

  if (fftCount * this->spectrumSize >= 2)
    this->feedLinearMode(psd, count, psdSize, freqMin, freqMax, adjustSides);
  else
    this->feedHistogramMode(psd, psdSize, freqMin, freqMax);

  this->interpolate();
}

void
SpectrumView::feed(
    const SUFLOAT *psd,
    const SUFLOAT *count,
    SUSCOUNT psdSize,
    SUFREQ center,
    bool adjustSides)
{
  this->feed(
        psd,
        count,
        psdSize,
        center - this->fftBandwidth / 2,
        center + this->fftBandwidth / 2,
        adjustSides);
}

void
SpectrumView::feed(SpectrumView const &detail)
{
  this->feed(
        detail.psdAccum,
        detail.psdCount,
        detail.spectrumSize,
        detail.freqMin,
        detail.freqMax,
        false);
}

void
SpectrumView::reset(void)
{
  memset(this->psd, 0, SIGDIGGER_SCANNER_SPECTRUM_SIZE * sizeof(SUFLOAT));
  memset(this->psdAccum, 0, SIGDIGGER_SCANNER_SPECTRUM_SIZE * sizeof(SUFLOAT));
  memset(this->psdCount, 0, SIGDIGGER_SCANNER_SPECTRUM_SIZE * sizeof(SUFLOAT));
}

Scanner::Scanner(
    QObject *parent,
    SUFREQ freqMin,
    SUFREQ freqMax,
    SUFREQ initFreqMin,
    SUFREQ initFreqMax,
    Suscan::Source::Config const &cfg) : QObject(parent)
{
  unsigned int targSampRate = cfg.getSampleRate();
  Suscan::AnalyzerParams params;
  bool noHop = false;

  if (freqMin > freqMax) {
    SUFREQ tmp = freqMin;
    freqMin = freqMax;
    freqMax = tmp;
  }

  if (initFreqMin > initFreqMax) {
    SUFREQ tmp = initFreqMin;
    initFreqMin = initFreqMax;
    initFreqMax = tmp;
  }

  if (initFreqMax - initFreqMin <= targSampRate) {
    SUFREQ centreFreq = (initFreqMin + initFreqMax) / 2;
    initFreqMin = centreFreq - (targSampRate / 2);
    initFreqMax = centreFreq + (targSampRate / 2);
    noHop = true;
  }

  if (initFreqMin < freqMin) {
    initFreqMax += freqMin - initFreqMin;
    initFreqMin = freqMin;
  }

  if (initFreqMax > freqMax) {
    initFreqMin -= initFreqMax - freqMax;
    initFreqMax = freqMax;
  }

  this->freqMin = freqMin;
  this->freqMax = freqMax;
  this->getSpectrumView().setRange(initFreqMin, initFreqMax);

  // choose an FFT size to achieve the required frequency resolution
  this->fftSize = nextPow2(targSampRate / SIGDIGGER_SCANNER_FREQ_RESOLUTION);

  params.channelUpdateInterval = 0;
  params.spectrumAvgAlpha = .001f;
  params.sAvgAlpha = 0.001f;
  params.nAvgAlpha = 0.5;
  params.snr = 2;
  params.windowSize = this->fftSize;

  params.mode = Suscan::AnalyzerParams::Mode::WIDE_SPECTRUM;
  if (noHop) {
    SUFREQ centreFreq = (initFreqMin + initFreqMax) / 2;
    params.minFreq = centreFreq;
    params.maxFreq = centreFreq;
  } else {
    params.minFreq = initFreqMin;
    params.maxFreq = initFreqMax;
  }

  this->analyzer = new Suscan::Analyzer(params, cfg);

  connect(
        this->analyzer,
        SIGNAL(halted(void)),
        this,
        SLOT(onAnalyzerHalted(void)));

  connect(
        this->analyzer,
        SIGNAL(eos(void)),
        this,
        SLOT(onAnalyzerHalted(void)));

  connect(
        this->analyzer,
        SIGNAL(read_error(void)),
        this,
        SLOT(onAnalyzerHalted(void)));

  connect(
        this->analyzer,
        SIGNAL(psd_message(const Suscan::PSDMessage &)),
        this,
        SLOT(onPSDMessage(const Suscan::PSDMessage &)));
}

Scanner::~Scanner()
{
  if (this->analyzer != nullptr)
    delete this->analyzer;
}

void
Scanner::setRelativeBw(float ratio)
{
  if (ratio > 1)
    ratio = 1;
  else if (ratio < 2.f / this->fftSize)
    ratio = 2.f / this->fftSize;

  this->views[0].fftRelBw = this->views[1].fftRelBw = ratio;
  this->analyzer->setRelBandwidth(ratio);
}

SpectrumView &
Scanner::getSpectrumView(void)
{
  return this->views[this->view];
}

SpectrumView const &
Scanner::getSpectrumView(void) const
{
  return this->views[this->view];
}

void
Scanner::stop(void)
{
  this->analyzer->halt();
}

void
Scanner::flip(void)
{
  this->view = 1 - this->view;
  this->getSpectrumView().reset();
}

void
Scanner::setStrategy(Suscan::Analyzer::SweepStrategy strategy)
{
  this->analyzer->setSweepStrategy(strategy);
}

void
Scanner::setPartitioning(Suscan::Analyzer::SpectrumPartitioning partitioning)
{
  this->analyzer->setSpectrumPartitioning(partitioning);
}

void
Scanner::setGain(QString const &name, float value)
{
  this->analyzer->setGain(name.toStdString(), value);
}

unsigned int
Scanner::getFs(void) const
{
  return this->fs;
}

void
Scanner::setViewRange(SUFREQ freqMin, SUFREQ freqMax, bool noHop)
{
  SUFREQ searchMin, searchMax;

  if (fs == 0)
      return;

  if (freqMin > freqMax) {
    SUFREQ tmp = freqMin;
    freqMin = freqMax;
    freqMax = tmp;
  }

  if (!noHop) {
    searchMin = freqMin - fs / 2;
    searchMax = freqMax + fs / 2;
  } else {
    searchMin = searchMax = .5 * (freqMin + freqMax);
    freqMin = searchMin - fs / 2;
    freqMax = searchMax + fs / 2;
  }

  if (searchMin < this->freqMin)
    searchMin = this->freqMin;

  if (searchMax > this->freqMax)
    searchMax = this->freqMax;

  // Scanner in zoom mode, copy this view back to mainView
  try {
    // Limits adjusted.
    if (std::fabs(this->getSpectrumView().freqMin - freqMin) > 1 ||
        std::fabs(this->getSpectrumView().freqMax - freqMax) > 1) {
      SpectrumView &previous = this->getSpectrumView();
      this->flip();
      this->getSpectrumView().setRange(freqMin, freqMax);
      this->getSpectrumView().feed(previous);
    }

    this->analyzer->setHopRange(searchMin, searchMax);
  } catch (Suscan::Exception const &) {
    // Invalid limits, warn?
  }
}

void
Scanner::setRttMs(unsigned int rtt)
{
  this->rtt = rtt;

  if (this->fs > 0)
    this->analyzer->setBufferingSize(rtt * this->fs / 1000);
}

////////////////////////////// Slots /////////////////////////////////////
void
Scanner::onPSDMessage(const Suscan::PSDMessage &msg)
{
  if (!this->fsGuessed) {
    this->fs = msg.getSampleRate();
    this->analyzer->setBufferingSize(this->rtt * this->fs / 1000);
    this->analyzer->setBandwidth(this->fs);
    this->fsGuessed = true;
    this->views[0].fftBandwidth = this->views[1].fftBandwidth = this->fs;
  }

  if (msg.size() == this->fftSize) {
    this->getSpectrumView().feed(
          msg.get(),
          nullptr,
          this->fftSize,
          msg.getFrequency());
  }

  emit spectrumUpdated();
}

void
Scanner::onAnalyzerHalted(void)
{
  emit stopped();
}
