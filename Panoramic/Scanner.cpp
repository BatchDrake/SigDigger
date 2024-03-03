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

  for (i = 0; i < SIGDIGGER_SCANNER_SPECTRUM_SIZE; ++i) {
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
  bins      = SIGDIGGER_SCANNER_SPECTRUM_SIZE / fftCount; // Destination bin count
  srcBinW   = static_cast<double>(inpBw) / psdSize;
  dstBinW   = static_cast<double>(this->freqRange) / SIGDIGGER_SCANNER_SPECTRUM_SIZE;
  delta     = dstBinW / srcBinW;                          // Source bins per destination bin

  pos = static_cast<double>(freqSkip + freqMin - this->freqMin) / (this->freqRange);
  pos *= SIGDIGGER_SCANNER_SPECTRUM_SIZE;
  assert(!std::isnan(pos));

  // j is initial dest index, k is end of dest range
  j = pos > 0 ? static_cast<int>(pos) : 0;
  k = pos + bins < SIGDIGGER_SCANNER_SPECTRUM_SIZE ?
    static_cast<int>(pos + bins) : SIGDIGGER_SCANNER_SPECTRUM_SIZE;

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
  unsigned int i;
  SUFLOAT accum = 0;

  fStart *= SIGDIGGER_SCANNER_SPECTRUM_SIZE;
  fEnd   *= SIGDIGGER_SCANNER_SPECTRUM_SIZE;
  relBw  *= SIGDIGGER_SCANNER_SPECTRUM_SIZE;

  unsigned int j = static_cast<unsigned int>(fStart);

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

    if (j + 1 < SIGDIGGER_SCANNER_SPECTRUM_SIZE) {
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

  if (fftCount * SIGDIGGER_SCANNER_SPECTRUM_SIZE >= 2)
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
        SIGDIGGER_SCANNER_SPECTRUM_SIZE,
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
    Suscan::Source::Config const &cfg) : QObject(parent)
{
  Suscan::AnalyzerParams params;
  unsigned int ms_samples;

  if (freqMin > freqMax) {
    SUFREQ tmp = freqMin;
    freqMin = freqMax;
    freqMax = tmp;
  }

  this->freqMin = freqMin;
  this->freqMax = freqMax;

  // choose an FFT size such that data capture is at least 1 ms
  ms_samples = cfg.getSampleRate() * 0.001;
  this->fftSize = 256;
  while (this->fftSize < ms_samples)
    this->fftSize <<= 1;

  params.channelUpdateInterval = 0;
  params.spectrumAvgAlpha = .001f;
  params.sAvgAlpha = 0.001f;
  params.nAvgAlpha = 0.5;
  params.snr = 2;
  params.windowSize = this->fftSize;

  params.mode = Suscan::AnalyzerParams::Mode::WIDE_SPECTRUM;
  params.minFreq = freqMin;
  params.maxFreq = freqMax;


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
  else if (ratio < 2.f / SIGDIGGER_SCANNER_SPECTRUM_SIZE)
    ratio = 2.f / SIGDIGGER_SCANNER_SPECTRUM_SIZE;

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
    this->getSpectrumView().setRange(this->freqMin, this->freqMax);
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
