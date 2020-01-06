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
  SUFLOAT left = SIGDIGGER_SCANNER_DEFAULT_BIN_VALUE;
  SUFLOAT right;
  bool inGap = false;

  // Find a bin with zero entries, measure its width,
  // compute values in both ends and interpolate

  i = 0;

  for (i = 0; i < SIGDIGGER_SCANNER_SPECTRUM_SIZE; ++i) {
    if (!inGap) {
      if (this->count[i] <= 1e-12f) {
        // Found zero!
        inGap = true;
        zero_pos = i;
        count = 1;
        if (i > 0)
          left = this->psd[i - 1];
      } else {
        this->psd[i] = this->psdAccum[i] / this->count[i];
        if (this->count[i] > SIGDIGGER_SCANNER_COUNT_MAX) {
          this->count[i]    = SIGDIGGER_SCANNER_COUNT_RESET;
          this->psdAccum[i] = this->psd[i] * SIGDIGGER_SCANNER_COUNT_RESET;
        }
      }
    } else {
      if (this->count[i] <= 1e-12f) {
        ++count;
      } else {
        // End of gap of zeroes. Compute right and interpolate
        inGap = false;
        right = this->psd[i] = this->psdAccum[i] / this->count[i];

        for (j = 0; j < count; ++j) {
          t = static_cast<SUFLOAT>(j + .5f) / count;
          this->psd[j + zero_pos] = (1 - t) * left + t * right;
        }
      }
    }
  }

  // Deal with trailing zeroes, if any
  if (inGap) {
    right = SIGDIGGER_SCANNER_DEFAULT_BIN_VALUE;
    for (j = 0; j < count; ++j) {
      t = static_cast<SUFLOAT>(j + .5f) / count;
      this->psd[j + zero_pos] = (1 - t) * left + t * right;
    }
  }
}

void
SpectrumView::feedLinearMode(
    const SUFLOAT *psdData,
    SUFREQ freqMin,
    SUFREQ freqMax)
{
  SUFREQ inpBw;
  SUFREQ bw;
  SUFLOAT fftCount; // Number of FFTs in a full spectrum
  SUFLOAT bins;
  int skip;
  int i, j = 0, p = 0;
  int pieceWidth;
  int startBin, endBin;
  int scaledLen;
  SUFLOAT accum = 0;
  SUFREQ pos = 0;
  SUFLOAT prev;
  SUFLOAT curr;
  SUFLOAT x;
  SUFLOAT t = 0;
  SUFLOAT delta;
  SUFREQ freqSkip;
  SUFLOAT tStart, tEnd;

  // Compute subrange inside PSD message
  inpBw = freqMax - freqMin;
  skip = static_cast<int>(
        .5f * (1 - this->fftRelBw) * SIGDIGGER_SCANNER_SPECTRUM_SIZE);
  freqSkip = static_cast<SUFREQ>(skip) / SIGDIGGER_SCANNER_SPECTRUM_SIZE
      * inpBw;
  pieceWidth = SIGDIGGER_SCANNER_SPECTRUM_SIZE - 2 * skip;
  bw = inpBw - 2 * freqSkip;
  assert(skip >= 0);

  // Delicate step 1: Average in blocks of fftCount.
  // In this range, we can fit fftCount = range / bw pieces
  // Each piece is w = SIGDIGGER_SCANNER_SPECTRUM_SIZE / fftCount bins wide
  // We must average SIGDIGGER_SCANNER_SPECTRUM_SIZE / w = fftCount bins

  // Compute dimension variables.
  fftCount  = static_cast<SUFLOAT>(this->freqRange / bw); // How many FFTs fit in here.
  bins      = SIGDIGGER_SCANNER_SPECTRUM_SIZE / fftCount; // Target bin count
  delta     = static_cast<SUFLOAT>(pieceWidth - 1) / bins;
  scaledLen = static_cast<int>(SU_FLOOR(bins));

  // This is basically a linear scale
  for (i = 0; i < scaledLen; ++i) {
    startBin  = static_cast<int>(SU_FLOOR(i * delta));
    endBin    = static_cast<int>(SU_FLOOR((i + 1) * delta));
    tStart    =  1 - (i * delta - startBin);
    tEnd      =  (i + 1) * delta - endBin;
    accum     = 0;

    for (j = startBin; j <= endBin; ++j) {
      if (j == startBin)
        accum += tStart * psdData[j + skip];
      else if (j == endBin)
        accum += tEnd * psdData[j + skip];
      else
        accum += psdData[j + skip];
    }

    this->averaged[i] = accum / delta;
  }

  p = scaledLen;

  // Delicate step 2: we have bpfft samples inside averaged. These
  // samples now must be placed via interpolation in this->psd.
  // The interpolation parameter t is calculated according to the
  // relative position of the bpfft-sized block in the range.
  //

  pos = (freqSkip + freqMin - this->freqMin) / (this->freqRange);
  pos *= SIGDIGGER_SCANNER_SPECTRUM_SIZE;
  j = static_cast<int>(floor(pos));
  t = static_cast<SUFLOAT>(pos - j);

  assert(!isnan(pos));

  prev = 0;
  for (i = 1; i <= p; ++i, ++j) {
    curr = i < p ? this->averaged[i] : 0;
    assert(!isnan(curr));
    x = (1 - t) * prev + t * curr;
    assert(!isnan(x));

    if (j >= 0 && j < SIGDIGGER_SCANNER_SPECTRUM_SIZE) {
      // Add, taking into account the interpolation parameter
      // and the weight of the last coefficient.
      this->psdAccum[j] += x;
      if (i == 1)
        delta = static_cast<SUFLOAT>(t);
      else if (i == p)
        delta = static_cast<SUFLOAT>(1.f - t);
      else
        delta = 1;

      this->count[j] += delta;
    }

    prev = curr;
  }
}

void
SpectrumView::feedHistogramMode(
    const SUFLOAT *psdData,
    SUFREQ freqMin,
    SUFREQ freqMax)
{
  SUFREQ relBw = (freqMax - freqMin) / this->freqRange;
  SUFREQ fStart = (freqMin - this->freqMin) / this->freqRange;
  SUFREQ fEnd   = (freqMax - this->freqMin) / this->freqRange;
  SUFLOAT t;
  SUFLOAT inv = 1. / SIGDIGGER_SCANNER_SPECTRUM_SIZE;
  unsigned int i;
  SUFLOAT accum = 0;
  unsigned int j = static_cast<unsigned int>(fStart);

  fStart *= SIGDIGGER_SCANNER_SPECTRUM_SIZE;
  fEnd   *= SIGDIGGER_SCANNER_SPECTRUM_SIZE;
  relBw  *= SIGDIGGER_SCANNER_SPECTRUM_SIZE;

  // Now, relBw represents the relative size of the range
  // with respecto to the spectrum bin.

  for (i = 0; i < SIGDIGGER_SCANNER_SPECTRUM_SIZE; ++i)
    accum += psdData[i];

  accum *= inv;

  assert(!isnan(inv));
  assert(!isnan(accum));

  if (floor(fStart) != floor(fEnd)) {
    // Between two bins.
    t = static_cast<SUFLOAT>((fStart - floor(fStart)) / relBw);

    this->count[j] += 1 - t;
    this->psdAccum[j] += (1 - t) * accum;

    if (j + 1 < SIGDIGGER_SCANNER_SPECTRUM_SIZE) {
      this->count[j + 1] += t;
      this->psdAccum[j + 1] += t * accum;
    }
  } else {
    this->count[j] += 1;
    this->psdAccum[j] += accum;
  }
}

void
SpectrumView::feed(const SUFLOAT *psd, SUFREQ freqMin, SUFREQ freqMax)
{
  SUFREQ fftCount = (freqMax - freqMin) / this->freqRange;

  if (fftCount < SIGDIGGER_SCANNER_SPECTRUM_SIZE)
    this->feedLinearMode(psd, freqMin, freqMax);
  else
    this->feedHistogramMode(psd, freqMin, freqMax);

  this->interpolate();
}

void
SpectrumView::feed(const SUFLOAT *psd, SUFREQ center)
{
  this->feed(
        psd,
        center - this->fftBandwidth / 2,
        center + this->fftBandwidth / 2);
}

void
SpectrumView::feed(SpectrumView const &detail)
{
  this->feed(
        detail.psd,
        detail.freqMin,
        detail.freqMax);
}

void
SpectrumView::reset(void)
{
  memset(this->psd, 0, SIGDIGGER_SCANNER_SPECTRUM_SIZE * sizeof(SUFLOAT));
  memset(this->psdAccum, 0, SIGDIGGER_SCANNER_SPECTRUM_SIZE * sizeof(SUFLOAT));
  memset(this->averaged, 0, SIGDIGGER_SCANNER_SPECTRUM_SIZE * sizeof(SUFLOAT));
  memset(this->count, 0, SIGDIGGER_SCANNER_SPECTRUM_SIZE * sizeof(SUFLOAT));
}

Scanner::Scanner(
    QObject *parent,
    SUFREQ freqMin,
    SUFREQ freqMax,
    Suscan::Source::Config const &cfg) : QObject(parent)
{
  Suscan::AnalyzerParams params;

  if (freqMin > freqMax) {
    SUFREQ tmp = freqMin;
    freqMin = freqMax;
    freqMax = tmp;
  }

  params.channelUpdateInterval = 0;
  params.spectrumAvgAlpha = .001f;
  params.sAvgAlpha = 0.001f;
  params.nAvgAlpha = 0.5;
  params.snr = 2;
  params.windowSize = SIGDIGGER_SCANNER_SPECTRUM_SIZE;

  this->mainView.freqMin = freqMin;
  this->mainView.freqMax = freqMax;

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

  this->mainView.fftRelBw   = ratio;
  this->detailView.fftRelBw = ratio;
}

SpectrumView &
Scanner::getSpectrumView(void)
{
  return this->zoomMode ? this->detailView : this->mainView;
}

SpectrumView const &
Scanner::getSpectrumView(void) const
{
  return this->zoomMode ? this->detailView : this->mainView;
}

void
Scanner::stop(void)
{
  this->analyzer->halt();
}

void
Scanner::setViewRange(SUFREQ freqMin, SUFREQ freqMax)
{
  if (freqMin > freqMax) {
    SUFREQ tmp = freqMin;
    freqMin = freqMax;
    freqMax = tmp;
  }

  if (fs == 0)
      return;

  if (freqMax - freqMin < fs)
    freqMax = freqMin + fs;

  if (freqMax - freqMin >= fs) {
    SUFREQ searchMin, searchMax;
    // Scanner in zoom mode, copy this view back to mainView
    try {
      if (freqMin < this->mainView.freqMin)
        freqMin = this->mainView.freqMin;

      if (freqMax > this->mainView.freqMax)
        freqMax = this->mainView.freqMax;

      searchMin = freqMin - fs / 2;
      searchMax = freqMax + fs / 2;

      if (searchMin < 0)
        searchMin = 0;

      this->analyzer->setHopRange(searchMin, searchMax);

      if (freqMin > this->mainView.freqMin
          || freqMax < this->mainView.freqMax) {
        this->zoomMode = true;
        this->detailView.setRange(freqMin, freqMax);
      } else {
        this->zoomMode = false;
      }
    } catch (Suscan::Exception const &) {
      // Invalid limits, warn?
    }
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
    this->mainView.fftBandwidth =
        this->detailView.fftBandwidth = this->fs;
    this->mainView.setRange(
          this->mainView.freqMin,
          this->mainView.freqMax);
  }

  if (msg.size() == SIGDIGGER_SCANNER_SPECTRUM_SIZE) {
    this->getSpectrumView().feed(
          msg.get(),
          msg.getFrequency());
  }

  emit spectrumUpdated();
}

void
Scanner::onAnalyzerHalted(void)
{
  emit stopped();
}
