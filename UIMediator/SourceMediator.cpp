//
//    SpectrumMediator.cpp: Coordinate source panel signals
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
UIMediator::connectSourcePanel(void)
{
  connect(
        this->ui->sourcePanel,
        SIGNAL(toggleRecord(void)),
        this,
        SLOT(onToggleRecord(void)));

  connect(
        this->ui->sourcePanel,
        SIGNAL(throttleConfigChanged(void)),
        this,
        SLOT(onThrottleConfigChanged(void)));

  connect(
        this->ui->sourcePanel,
        SIGNAL(gainChanged(QString, float)),
        this,
        SLOT(onGainChanged(QString, float)));

  connect(
        this->ui->sourcePanel,
        SIGNAL(toggleDCRemove(void)),
        this,
        SLOT(onToggleDCRemove(void)));

  connect(
        this->ui->sourcePanel,
        SIGNAL(toggleIQReverse(void)),
        this,
        SLOT(onToggleIQReverse(void)));

  connect(
        this->ui->sourcePanel,
        SIGNAL(toggleAGCEnabled(void)),
        this,
        SLOT(onToggleAGCEnabled(void)));

  connect(
        this->ui->sourcePanel,
        SIGNAL(antennaChanged(QString)),
        this,
        SLOT(onAntennaChanged(QString)));

  connect(
        this->ui->sourcePanel,
        SIGNAL(bandwidthChanged(void)),
        this,
        SLOT(onBandwidthChanged(void)));
}

void
UIMediator::onToggleRecord(void)
{
  emit toggleRecord();
}

void
UIMediator::onThrottleConfigChanged(void)
{
  this->ui->spectrum->setThrottling(this->ui->sourcePanel->isThrottleEnabled());
  emit throttleConfigChanged();
}

void
UIMediator::onGainChanged(QString name, float val)
{
  emit gainChanged(name, val);
}

void
UIMediator::onToggleDCRemove(void)
{
  emit toggleDCRemove();
}

void
UIMediator::onToggleIQReverse(void)
{
  emit toggleIQReverse();
}

void
UIMediator::onToggleAGCEnabled(void)
{
  emit toggleAGCEnabled();
}

void
UIMediator::onAntennaChanged(QString name)
{
  this->appConfig->profile.setAntenna(this->ui->sourcePanel->getAntenna());
  emit antennaChanged(name);
}

void
UIMediator::onBandwidthChanged(void)
{
  this->appConfig->profile.setBandwidth(this->ui->sourcePanel->getBandwidth());
  emit bandwidthChanged();
}
