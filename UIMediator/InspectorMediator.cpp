//
//    InspectorMediator.cpp: Coordinate inspector panel signals
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
UIMediator::connectInspectorPanel(void)
{
  connect(
        this->ui->inspectorPanel,
        SIGNAL(bandwidthChanged(int)),
        this,
        SLOT(onInspBandwidthChanged()));

  connect(
        this->ui->inspectorPanel,
        SIGNAL(requestOpenInspector(QString)),
        this,
        SLOT(onOpenInspector()));
}

void
UIMediator::onInspBandwidthChanged(void)
{
  this->ui->spectrum->setFilterBandwidth(
        this->ui->inspectorPanel->getBandwidth());
  this->appConfig->bandwidth =
      static_cast<unsigned int>(this->ui->spectrum->getBandwidth());
  emit channelBandwidthChanged(this->ui->inspectorPanel->getBandwidth());
}

void
UIMediator::onOpenInspector(void)
{
  emit requestOpenInspector();
}
