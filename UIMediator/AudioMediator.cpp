//
//    AudioMediator.cpp: Mediate audio panel events
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
UIMediator::connectAudioPanel(void)
{
  connect(
        this->ui->audioPanel,
        SIGNAL(changed(void)),
        this,
        SLOT(onAudioChanged(void)));

  connect(
        this->ui->audioPanel,
        SIGNAL(volumeChanged(float)),
        this,
        SIGNAL(audioVolumeChanged(float)));

  connect(
        this->ui->audioPanel,
        SIGNAL(recordStateChanged(bool)),
        this,
        SIGNAL(audioRecordStateChanged(void)));

  connect(
        this->ui->audioPanel,
        SIGNAL(setCorrection(Suscan::Orbit)),
        this,
        SIGNAL(audioSetCorrection(Suscan::Orbit)));

  connect(
        this->ui->audioPanel,
        SIGNAL(disableCorrection(void)),
        this,
        SIGNAL(audioDisableCorrection(void)));
}

void
UIMediator::onAudioChanged(void)
{
  MainSpectrum::Skewness skewness = MainSpectrum::SYMMETRIC;

  if (this->ui->audioPanel->getEnabled()) {
    switch (this->ui->audioPanel->getDemod()) {
      case AM:
      case FM:
        skewness = MainSpectrum::SYMMETRIC;
        break;

      case USB:
        skewness = MainSpectrum::UPPER;
        break;

      case LSB:
        skewness = MainSpectrum::LOWER;
        break;
    }
  }

  this->ui->spectrum->setFilterSkewness(skewness);

  emit audioChanged();
}
