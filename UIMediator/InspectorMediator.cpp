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
#include <SuWidgetsHelpers.h>

using namespace SigDigger;


Inspector *
UIMediator::lookupInspector(Suscan::InspectorId handle) const
{
  Inspector *entry = nullptr;

  try {
    entry = this->ui->inspectorTable.at(handle);
  } catch (std::out_of_range &) { }

  return entry;
}

QString
UIMediator::getInspectorTabTitle(Suscan::InspectorMessage const &msg)
{
  QString result = " in "
      + SuWidgetsHelpers::formatQuantity(
        msg.getChannel().fc + msg.getChannel().ft,
        "Hz");

  if (msg.getClass() == "psk")
    return "PSK inspector" + result;
  else if (msg.getClass() == "fsk")
    return "FSK inspector" + result;
  else if (msg.getClass() == "ask")
    return "ASK inspector" + result;

  return "Generic inspector" + result;
}

Inspector *
UIMediator::addInspectorTab(
    Suscan::InspectorMessage const &msg,
    Suscan::InspectorId &oId)
{

  int index;
  Inspector *insp = new Inspector(
        this->ui->main->mainTab,
        msg,
        *this->appConfig);

  oId = this->ui->lastId++;

  insp->setId(oId);
  insp->setRealTime(
        this->appConfig->profile.getInterface() == SUSCAN_SOURCE_LOCAL_INTERFACE
        && this->appConfig->profile.getType() == SUSCAN_SOURCE_TYPE_SDR);
  insp->setTunerFrequency(this->ui->spectrum->getCenterFreq());

  index = this->ui->main->mainTab->addTab(
        insp,
        UIMediator::getInspectorTabTitle(msg));

  this->ui->inspectorTable[oId] = insp;
  this->ui->main->mainTab->setCurrentIndex(index);

  return insp;
}

void
UIMediator::closeInspectorTab(Inspector *insp)
{
  if (insp != nullptr) {
    Suscan::Analyzer *analyzer = insp->getAnalyzer();
    if (analyzer != nullptr) {
      analyzer->closeInspector(insp->getHandle(), 0);
    } else {
      this->ui->main->mainTab->removeTab(
            this->ui->main->mainTab->indexOf(insp));
      this->ui->inspectorTable.erase(insp->getId());
      delete insp;
    }
  }
}

void
UIMediator::detachAllInspectors()
{
  for (auto p = this->ui->inspectorTable.begin();
       p != this->ui->inspectorTable.end();
       ++p) {
    p->second->setAnalyzer(nullptr);
    p->second = nullptr;
  }
}

void
UIMediator::resetRawInspector(qreal fs)
{
  this->ui->inspectorPanel->resetRawInspector(fs);
}

void
UIMediator::feedRawInspector(const SUCOMPLEX *data, size_t size)
{
  this->ui->inspectorPanel->feedRawInspector(data, size);
}

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

  connect(
        this->ui->inspectorPanel,
        SIGNAL(startRawCapture()),
        this,
        SLOT(onOpenRawInspector()));

  connect(
        this->ui->inspectorPanel,
        SIGNAL(stopRawCapture()),
        this,
        SLOT(onCloseRawInspector()));
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

void
UIMediator::onOpenRawInspector(void)
{
  emit requestOpenRawInspector();
}

void
UIMediator::onCloseRawInspector(void)
{
  emit requestCloseRawInspector();
}

void
UIMediator::onCloseInspectorTab(int ndx)
{
  QWidget *widget = this->ui->main->mainTab->widget(ndx);

  if (widget != nullptr && widget != this->ui->spectrum) {
    Inspector *insp = static_cast<Inspector *>(widget);
    this->closeInspectorTab(insp);
  }
}
