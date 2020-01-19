//
//    DeviceDialogMediator.cpp: Mediate device dialog mediator
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
UIMediator::connectPanoramicDialog(void)
{
  connect(
        this->ui->panoramicDialog,
        SIGNAL(start(void)),
        this,
        SLOT(onPanoramicSpectrumStart(void)));

  connect(
        this->ui->panoramicDialog,
        SIGNAL(stop(void)),
        this,
        SLOT(onPanoramicSpectrumStop(void)));

  connect(
        this->ui->panoramicDialog,
        SIGNAL(detailChanged(quint64, quint64, bool)),
        this,
        SLOT(onPanoramicSpectrumDetailChanged(quint64, quint64, bool)));

  connect(
        this->ui->panoramicDialog,
        SIGNAL(frameSkipChanged(void)),
        this,
        SIGNAL(panSpectrumSkipChanged(void)));

  connect(
        this->ui->panoramicDialog,
        SIGNAL(relBandwidthChanged(void)),
        this,
        SIGNAL(panSpectrumRelBwChanged(void)));

  connect(
        this->ui->panoramicDialog,
        SIGNAL(reset(void)),
        this,
        SIGNAL(panSpectrumReset(void)));

  connect(
        this->ui->panoramicDialog,
        SIGNAL(strategyChanged(QString)),
        this,
        SIGNAL(panSpectrumStrategyChanged(QString)));

  connect(
        this->ui->panoramicDialog,
        SIGNAL(partitioningChanged(QString)),
        this,
        SIGNAL(panSpectrumPartitioningChanged(QString)));

  connect(
        this->ui->panoramicDialog,
        SIGNAL(gainChanged(QString, float)),
        this,
        SIGNAL(panSpectrumGainChanged(QString, float)));
}

void
UIMediator::onPanoramicSpectrumStart(void)
{
  emit panSpectrumStart();
}

void
UIMediator::onPanoramicSpectrumStop(void)
{
  emit panSpectrumStop();
}

void
UIMediator::onPanoramicSpectrumDetailChanged(quint64 min, quint64 max, bool noHop)
{
  emit panSpectrumRangeChanged(min, max, noHop);
}
