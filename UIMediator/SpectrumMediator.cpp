//
//    Spectrum.cpp: Coordinate spectrum signals
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

#include "UIMediator.h"

using namespace SigDigger;

void
UIMediator::connectSpectrum(void)
{
  connect(
        this->ui->spectrum,
        SIGNAL(bandwidthChanged(void)),
        this,
        SLOT(onBandwidthChanged(void)));

  connect(
        this->ui->spectrum,
        SIGNAL(frequencyChanged(qint64)),
        this,
        SLOT(onFrequencyChanged(qint64)));

  connect(
        this->ui->spectrum,
        SIGNAL(loChanged(qint64)),
        this,
        SLOT(onLoChanged(qint64)));

  connect(
        this->ui->spectrum,
        SIGNAL(rangeChanged(float, float)),
        this,
        SLOT(onRangeChanged(float, float)));

  connect(
        this->ui->spectrum,
        SIGNAL(zoomChanged(float)),
        this,
        SLOT(onZoomChanged(float)));
}


void
UIMediator::onBandwidthChanged(void)
{
  this->ui->inspectorPanel->setBandwidth(this->ui->spectrum->getBandwidth());
  this->appConfig->bandwidth =
      static_cast<unsigned int>(this->ui->spectrum->getBandwidth());
  emit bandwidthChanged(this->ui->spectrum->getBandwidth());
}

void
UIMediator::onFrequencyChanged(qint64)
{
  qint64 freq = this->ui->spectrum->getCenterFreq()
      + this->ui->spectrum->getLoFreq();
  this->ui->inspectorPanel->setDemodFrequency(freq);
  this->appConfig->profile.setFreq(freq);

  emit frequencyChanged(this->ui->spectrum->getCenterFreq());
}

void
UIMediator::onLoChanged(qint64)
{
  qint64 freq = this->ui->spectrum->getCenterFreq()
      + this->ui->spectrum->getLoFreq();
  this->ui->inspectorPanel->setDemodFrequency(freq);

  this->appConfig->loFreq = static_cast<int>(this->ui->spectrum->getLoFreq());
  emit loChanged(this->ui->spectrum->getLoFreq());
}

void
UIMediator::onRangeChanged(float min, float max)
{
  this->ui->spectrum->setPandapterRange(min, max);
  this->ui->fftPanel->setPandRangeMin(static_cast<int>(min));
  this->ui->fftPanel->setPandRangeMax(static_cast<int>(max));

  if (this->ui->fftPanel->getRangeLock()) {
    this->ui->spectrum->setWfRange(min, max);
    this->ui->fftPanel->setWfRangeMin(static_cast<int>(min));
    this->ui->fftPanel->setWfRangeMax(static_cast<int>(max));
  }
}

void
UIMediator::onZoomChanged(float level)
{
  this->ui->fftPanel->setFreqZoom(static_cast<int>(level));
}
