//
//    Spectrum.cpp: Coordinate source panel signals
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

#include "UIMediator.h"

using namespace SigDigger;

void
UIMediator::connectFftPanel(void)
{
  connect(
        this->ui->fftPanel,
        SIGNAL(paletteChanged(void)),
        this,
        SLOT(onPaletteChanged(void)));

  connect(
        this->ui->fftPanel,
        SIGNAL(rangesChanged(void)),
        this,
        SLOT(onRangesChanged(void)));

  connect(
        this->ui->fftPanel,
        SIGNAL(averagerChanged(void)),
        this,
        SLOT(onAveragerChanged(void)));

  connect(
        this->ui->fftPanel,
        SIGNAL(fftSizeChanged(void)),
        this,
        SLOT(onFftSizeChanged(void)));

  connect(
        this->ui->fftPanel,
        SIGNAL(refreshRateChanged(void)),
        this,
        SLOT(onRefreshRateChanged(void)));

  connect(
        this->ui->fftPanel,
        SIGNAL(timeSpanChanged(void)),
        this,
        SLOT(onTimeSpanChanged(void)));

  connect(
        this->ui->fftPanel,
        SIGNAL(windowFunctionChanged(void)),
        this,
        SLOT(onWindowFunctionChanged(void)));

  connect(
        this->ui->fftPanel,
        SIGNAL(timeStampsChanged(void)),
        this,
        SLOT(onTimeStampsChanged(void)));

  connect(
        this->ui->fftPanel,
        SIGNAL(bookmarksChanged(void)),
        this,
        SLOT(onBookmarksButtonChanged(void)));
}

void
UIMediator::onPaletteChanged(void)
{
  this->ui->spectrum->setPaletteGradient(
        this->ui->fftPanel->getPaletteGradient());
}

void
UIMediator::onRangesChanged(void)
{
  if (!this->settingRanges) {
    this->settingRanges = true;
    this->ui->spectrum->setPandapterRange(
          this->ui->fftPanel->getPandRangeMin(),
          this->ui->fftPanel->getPandRangeMax());

    this->ui->spectrum->setWfRange(
          this->ui->fftPanel->getWfRangeMin(),
          this->ui->fftPanel->getWfRangeMax());

    this->ui->spectrum->setPanWfRatio(this->ui->fftPanel->getPanWfRatio());
    this->ui->spectrum->setZoom(this->ui->fftPanel->getFreqZoom());

    this->ui->spectrum->setPeakDetect(this->ui->fftPanel->getPeakDetect());
    this->ui->spectrum->setPeakHold(this->ui->fftPanel->getPeakHold());
    this->settingRanges = false;
  }
}

void
UIMediator::onAveragerChanged(void)
{
  this->averager.setAlpha(this->ui->fftPanel->getAveraging());
}

void
UIMediator::onFftSizeChanged(void)
{
  this->appConfig->analyzerParams.windowSize = this->ui->fftPanel->getFftSize();
  emit analyzerParamsChanged();
}

void
UIMediator::onRefreshRateChanged(void)
{
  this->appConfig->analyzerParams.psdUpdateInterval = 1.f / this->ui->fftPanel->getRefreshRate();
  this->ui->spectrum->setExpectedRate(
        static_cast<int>(this->ui->fftPanel->getRefreshRate()));
  emit analyzerParamsChanged();
}

void
UIMediator::onTimeSpanChanged(void)
{
  this->ui->spectrum->setTimeSpan(this->ui->fftPanel->getTimeSpan());
}

void
UIMediator::onTimeStampsChanged(void)
{
  this->ui->spectrum->setTimeStamps(this->ui->fftPanel->getTimeStamps());
}

void
UIMediator::onBookmarksButtonChanged(void)
{
  this->ui->spectrum->setBookmarks(this->ui->fftPanel->getBookmarks());
}

void
UIMediator::onWindowFunctionChanged(void)
{
  this->appConfig->analyzerParams.windowFunction =
      this->ui->fftPanel->getWindowFunction();

  emit analyzerParamsChanged();
}
