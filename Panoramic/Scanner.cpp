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
  reset();
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

  reset();
}

void
SpectrumView::interpolate()
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
          this->psdCount[i] = SIGDIGGER_SCANNER_COUNT_RESET;
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
      this->psd[j + zero_pos] = left;
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
    feedLinearMode(psd, count, psdSize, freqMin, freqMax, adjustSides);
  else
    feedHistogramMode(psd, psdSize, freqMin, freqMax);

  interpolate();
}

void
SpectrumView::feed(
    const SUFLOAT *psd,
    const SUFLOAT *count,
    SUSCOUNT psdSize,
    SUFREQ center,
    bool adjustSides)
{
  feed(
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
  feed(
        detail.psdAccum,
        detail.psdCount,
        detail.spectrumSize,
        detail.freqMin,
        detail.freqMax,
        false);
}

void
SpectrumView::reset()
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
    bool noHop,
    Suscan::Source::Config const &cfg) : QObject(parent)
{
  unsigned int targSampRate = cfg.getSampleRate();
  Suscan::AnalyzerParams params;

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

  m_freqMin = freqMin;
  m_freqMax = freqMax;

  // choose an FFT size to achieve the required frequency resolution
  m_fftSize = nextPow2(targSampRate / SIGDIGGER_SCANNER_FREQ_RESOLUTION);

  params.channelUpdateInterval = 0;
  params.spectrumAvgAlpha = .001f;
  params.sAvgAlpha = 0.001f;
  params.nAvgAlpha = 0.5;
  params.snr = 2;
  params.windowSize = m_fftSize;

  params.mode = Suscan::AnalyzerParams::Mode::WIDE_SPECTRUM;

  if (noHop) {
    SUFREQ centreFreq = (initFreqMin + initFreqMax) / 2;
    params.minFreq = params.maxFreq = centreFreq;
    getSpectrumView().setRange(centreFreq - targSampRate / 2,
                                     centreFreq + targSampRate / 2);
  } else {
    params.minFreq = initFreqMin;
    params.maxFreq = initFreqMax;
    getSpectrumView().setRange(initFreqMin, initFreqMax);
  }

  m_analyzer = new Suscan::Analyzer(params, cfg);

  connect(
        m_analyzer,
        SIGNAL(halted()),
        this,
        SLOT(onAnalyzerHalted()));

  connect(
        m_analyzer,
        SIGNAL(eos()),
        this,
        SLOT(onAnalyzerHalted()));

  connect(
        m_analyzer,
        SIGNAL(read_error()),
        this,
        SLOT(onAnalyzerHalted()));

  connect(
        m_analyzer,
        SIGNAL(psd_message(const Suscan::PSDMessage &)),
        this,
        SLOT(onPSDMessage(const Suscan::PSDMessage &)));
}

Scanner::~Scanner()
{
  stop();
}

void
Scanner::setRelativeBw(float ratio)
{
  if (ratio > 1)
    ratio = 1;
  else if (ratio < 2.f / m_fftSize)
    ratio = 2.f / m_fftSize;

  m_views[0].fftRelBw = m_views[1].fftRelBw = ratio;

  if (m_analyzer)
    m_analyzer->setRelBandwidth(ratio);
}

SpectrumView &
Scanner::getSpectrumView()
{
  return m_views[m_view];
}

SpectrumView const &
Scanner::getSpectrumView() const
{
  return m_views[m_view];
}

void
Scanner::stop()
{
  if (m_analyzer) {
    delete m_analyzer;
    m_analyzer = nullptr;
  }
}

void
Scanner::flip()
{
  m_view = 1 - m_view;
  getSpectrumView().reset();
}

void
Scanner::setStrategy(Suscan::Analyzer::SweepStrategy strategy)
{
  if (m_analyzer)
    m_analyzer->setSweepStrategy(strategy);
}

void
Scanner::setPartitioning(Suscan::Analyzer::SpectrumPartitioning partitioning)
{
  if (m_analyzer)
    m_analyzer->setSpectrumPartitioning(partitioning);
}

void
Scanner::setGain(QString const &name, float value)
{
  if (m_analyzer)
    m_analyzer->setGain(name.toStdString(), value);
}

unsigned int
Scanner::getFs() const
{
  return m_fs;
}

void
Scanner::setViewRange(SUFREQ freqMin, SUFREQ freqMax, bool noHop)
{
  SUFREQ searchMin, searchMax;

  if (freqMin > freqMax) {
    SUFREQ tmp = freqMin;
    freqMin = freqMax;
    freqMax = tmp;
  }

  if (!noHop) {
    searchMin = freqMin;
    searchMax = freqMax;
  } else {
    searchMin = searchMax = .5 * (freqMin + freqMax);
    if (m_fs != 0) {
      freqMin = searchMin - m_fs / 2;
      freqMax = searchMax + m_fs / 2;
    }
  }

  if (searchMin < m_freqMin)
    searchMin = m_freqMin;

  if (searchMax > m_freqMax)
    searchMax = m_freqMax;

  // Scanner in zoom mode, copy this view back to mainView
  try {
    // Limits adjusted.
    if (std::fabs(getSpectrumView().freqMin - freqMin) > 1 ||
        std::fabs(getSpectrumView().freqMax - freqMax) > 1) {
      SpectrumView &previous = getSpectrumView();
      flip();
      getSpectrumView().setRange(freqMin, freqMax);
      getSpectrumView().feed(previous);
      emit spectrumUpdated();
    }

    if (m_analyzer)
      m_analyzer->setHopRange(searchMin, searchMax);
  } catch (Suscan::Exception const &) {
    // Invalid limits, warn?
  }
}

void
Scanner::setRttMs(unsigned int rtt)
{
  m_rtt = rtt;

  if (m_fs > 0 && m_analyzer)
    m_analyzer->setBufferingSize(rtt * m_fs / 1000);
}

////////////////////////////// Slots /////////////////////////////////////
void
Scanner::onPSDMessage(const Suscan::PSDMessage &msg)
{
  if (!m_fsGuessed) {
    m_fs = msg.getSampleRate();
    m_analyzer->setBufferingSize(m_rtt * m_fs / 1000);
    m_analyzer->setBandwidth(m_fs);
    m_fsGuessed = true;
    m_views[0].fftBandwidth = m_views[1].fftBandwidth = m_fs;
  }

  if (msg.size() == m_fftSize) {
    getSpectrumView().feed(
          msg.get(),
          nullptr,
          m_fftSize,
          msg.getFrequency());
  }

  emit spectrumUpdated();
}

void
Scanner::onAnalyzerHalted()
{
  emit stopped();
}
