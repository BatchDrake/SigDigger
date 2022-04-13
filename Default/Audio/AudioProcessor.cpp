//
//    AudioProcessor.cpp: Audio processor
//    Copyright (C) 2022 Gonzalo José Carracedo Carballal
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
#include "AudioProcessor.h"
#include "AudioPlayback.h"

using namespace SigDigger;

AudioProcessor::AudioProcessor(UIMediator *, QObject *parent)
  : QObject(parent)
{
  this->m_playBack = new AudioPlayback("default");
}

void
AudioProcessor::connectAnalyzer()
{
  // TODO
}

void
AudioProcessor::setAnalyzer(Suscan::Analyzer *)
{
  // TODO
}

void
AudioProcessor::setVolume(float)
{
  // TODO
}

void
AudioProcessor::setAudioCorrection(Suscan::Orbit const &)
{
  // TODO
}

void
AudioProcessor::setCorrectionEnabled(bool)
{
  // TODO
}

void
AudioProcessor::setDemod(AudioDemod)
{
  // TODO
}

void
AudioProcessor::setSampleRate(unsigned)
{
  // TODO
}

void
AudioProcessor::setCutOff(float)
{
  // TODO
}

void
AudioProcessor::setDemodFreq(SUFREQ)
{
  // TODO
}

void
AudioProcessor::startRecording(QString)
{
  // TODO
}

void
AudioProcessor::stopRecording(void)
{
  // TODO
}
