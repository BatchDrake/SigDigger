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
#include "Inspector.h"
#include "ui_MainWindow.h"
#include "MainSpectrum.h"
#include "InspectorPanel.h"

#include <QDialog>
#include <QTabBar>

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

Inspector *
UIMediator::addInspector(
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
  insp->setRealTime(this->isRealTime);
  insp->setTimeLimits(this->profileStart, this->profileEnd);

  insp->setTunerFrequency(this->ui->spectrum->getCenterFreq());

  index = this->ui->main->mainTab->addTab(
        insp,
        insp->getName());

  connect(
        insp,
        SIGNAL(nameChanged()),
        this,
        SLOT(onInspectorNameChanged()));

  connect(
        insp,
        SIGNAL(closeRequested()),
        this,
        SLOT(onInspectorCloseRequested()));

  connect(
        insp,
        SIGNAL(detachRequested()),
        this,
        SLOT(onInspectorDetachRequested()));

  this->ui->inspectorTable[oId] = insp;
  this->ui->main->mainTab->setCurrentIndex(index);

  return insp;
}

void
UIMediator::unbindInspectorWidget(Inspector *insp)
{
  int index = this->ui->main->mainTab->indexOf(insp);

  // Inspector widgets may be either in the main window's tab or on
  // a QDialog on its own.

  if (index != -1) {
    this->ui->main->mainTab->removeTab(index);
    insp->setParent(nullptr);
  } else {
    auto p = this->ui->floatInspectorTable.find(insp);

    if (p != this->ui->floatInspectorTable.end()) {
      QDialog *d = p->second;
      this->ui->floatInspectorTable.erase(insp);

      // This sets the parent to null
      insp->setParent(nullptr);

      d->setProperty(
            "inspector-ptr",
            QVariant::fromValue<Inspector *>(nullptr));

      d->close();
      d->deleteLater();
    }
  }
}

void
UIMediator::closeInspector(Inspector *insp)
{
  if (insp != nullptr) {
    Suscan::Analyzer *analyzer = insp->getAnalyzer();
    if (analyzer != nullptr) {
      analyzer->closeInspector(insp->getHandle(), 0);
    } else {
      this->unbindInspectorWidget(insp);
      this->ui->inspectorTable.erase(insp->getId());
      delete insp;
    }
  }
}

void
UIMediator::floatInspector(Inspector *insp)
{
  int index = this->ui->main->mainTab->indexOf(insp);

  if (index != -1) {
    QDialog *dialog = new QDialog(this->owner);
    QVBoxLayout *layout = new QVBoxLayout;

    insp->beginReparenting();
    this->unbindInspectorWidget(insp);

    dialog->setProperty(
          "inspector-ptr",
          QVariant::fromValue<Inspector *>(insp));
    dialog->setLayout(layout);
    dialog->setWindowFlags(Qt::Window);

    connect(
          dialog,
          SIGNAL(finished(int)),
          this,
          SLOT(onCloseInspectorWindow()));

    layout->addWidget(insp);

    insp->doneReparenting();
    insp->show();

    dialog->move(this->owner->pos());
    dialog->setWindowTitle("SigDigger - " + insp->getName());

    this->ui->floatInspectorTable[insp] = dialog;
    dialog->show();
  }
}

void
UIMediator::detachAllInspectors()
{
  for (auto p = this->ui->inspectorTable.begin();
       p != this->ui->inspectorTable.end();
       ++p) {
    if (p->second) {
      p->second->setAnalyzer(nullptr);
      p->second = nullptr;
    }
  }

  this->ui->inspectorTable.clear();
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
    this->closeInspector(insp);
  }
}


void
UIMediator::onInspectorMenuRequested(const QPoint &point)
{
  int index;

  if (point.isNull())
    return;

  index = this->ui->main->mainTab->tabBar()->tabAt(point);

  QWidget *widget = this->ui->main->mainTab->widget(index);

  if (widget != nullptr && widget != this->ui->spectrum) {
    Inspector *insp = static_cast<Inspector *>(widget);
    insp->popupContextMenu();
  }
}

void
UIMediator::onInspectorNameChanged(void)
{
  Inspector *insp = static_cast<Inspector *>(QObject::sender());
  int index = this->ui->main->mainTab->indexOf(insp);

  if (index >= 0)
    this->ui->main->mainTab->setTabText(
        index,
        insp->getName());
}

void
UIMediator::onInspectorCloseRequested(void)
{
  Inspector *insp = static_cast<Inspector *>(QObject::sender());
  this->closeInspector(insp);
}

void
UIMediator::onInspectorDetachRequested(void)
{
  Inspector *insp = static_cast<Inspector *>(QObject::sender());
  this->floatInspector(insp);
}

void
UIMediator::onCloseInspectorWindow(void)
{
  QDialog *sender = qobject_cast<QDialog *>(QObject::sender());
  Inspector *insp;

  insp = sender->property("inspector-ptr").value<Inspector *>();

  if (insp != nullptr)
    this->closeInspector(insp);
}
