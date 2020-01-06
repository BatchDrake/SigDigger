//
//    AnalyzerParams.cpp: Analyzer parameters
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

#include <Suscan/AnalyzerParams.h>

using namespace Suscan;

void
AnalyzerParams::updateExposedParams(void)
{
  this->channelUpdateInterval = this->c_params.channel_update_int;
  this->psdUpdateInterval     = this->c_params.psd_update_int;
  this->spectrumAvgAlpha      = this->c_params.detector_params.alpha;
  this->sAvgAlpha             = this->c_params.detector_params.beta;
  this->nAvgAlpha             = this->c_params.detector_params.gamma;
  this->snr                   = this->c_params.detector_params.snr;
  this->windowSize            = this->c_params.detector_params.window_size;
  this->minFreq               = this->c_params.min_freq;
  this->maxFreq               = this->c_params.max_freq;
  this->mode                  =
      static_cast<enum Mode>(this->c_params.mode);
  this->windowFunction        =
      static_cast<enum WindowFunction>(this->c_params.detector_params.window);
}

AnalyzerParams::AnalyzerParams(void) : Serializable()
{
  this->updateExposedParams();
}

AnalyzerParams::AnalyzerParams(struct suscan_analyzer_params const &params)
{
  this->c_params = params;
  this->updateExposedParams();
}

struct suscan_analyzer_params const &
AnalyzerParams::getCParams(void)
{
  this->c_params.channel_update_int     = this->channelUpdateInterval;
  this->c_params.psd_update_int         = this->psdUpdateInterval;
  this->c_params.min_freq               = this->minFreq;
  this->c_params.max_freq               = this->maxFreq;
  this->c_params.detector_params.alpha  = this->spectrumAvgAlpha;
  this->c_params.detector_params.beta   = this->sAvgAlpha;
  this->c_params.detector_params.gamma  = this->nAvgAlpha;
  this->c_params.detector_params.snr    = this->snr;
  this->c_params.detector_params.window_size = this->windowSize;
  this->c_params.detector_params.window =
      static_cast<enum sigutils_channel_detector_window>(this->windowFunction);
  this->c_params.mode                   =
      static_cast<enum suscan_analyzer_mode>(this->mode);

  return this->c_params;
}

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

void
AnalyzerParams::deserialize(Object const &conf)
{
  std::string wFunc;
  std::string mode;

  LOAD(channelUpdateInterval);
  LOAD(psdUpdateInterval);
  LOAD(spectrumAvgAlpha);
  LOAD(sAvgAlpha);
  LOAD(nAvgAlpha);
  LOAD(snr);
  LOAD(windowSize);
  LOAD(minFreq);
  LOAD(maxFreq);

  wFunc = conf.get("windowFunction", std::string("none"));

  if (wFunc == "none")
    this->windowFunction = WindowFunction::NONE;
  else if (wFunc == "hamming")
    this->windowFunction = WindowFunction::HAMMING;
  else if (wFunc == "hann")
    this->windowFunction = WindowFunction::HANN;
  else if (wFunc == "flat-top")
    this->windowFunction = WindowFunction::FLAT_TOP;
  else if (wFunc == "blackmann-harris")
    this->windowFunction = WindowFunction::BLACKMANN_HARRIS;

  mode = conf.get("mode", std::string("channel"));
  if (mode == "channel")
    this->mode = Mode::CHANNEL;
  else if (mode == "wide-spectrum")
    this->mode = Mode::WIDE_SPECTRUM;
}

Object &&
AnalyzerParams::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("AnalyzerParams");

  STORE(channelUpdateInterval);
  STORE(psdUpdateInterval);
  STORE(spectrumAvgAlpha);
  STORE(sAvgAlpha);
  STORE(nAvgAlpha);
  STORE(snr);
  STORE(windowSize);
  STORE(minFreq);
  STORE(maxFreq);

  switch (this->windowFunction) {
    case NONE:
      obj.set("windowFunction", std::string("none"));
      break;

    case HAMMING:
      obj.set("windowFunction", std::string("hamming"));
      break;

    case HANN:
      obj.set("windowFunction", std::string("hann"));
      break;

    case FLAT_TOP:
      obj.set("windowFunction", std::string("flat-top"));
      break;

    case BLACKMANN_HARRIS:
      obj.set("windowFunction", std::string("blackmann-harris"));
      break;
  }

  switch (this->mode) {
    case CHANNEL:
      obj.set("mode", std::string("channel"));
      break;

    case WIDE_SPECTRUM:
      obj.set("mode", std::string("wide-spectrum"));
      break;
  }

  return this->persist(obj);
}
