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
#include <InspectionWidgetFactory.h>
#include "ui_MainWindow.h"
#include "MainSpectrum.h"

#include <QDialog>
#include <QTabBar>

using namespace SigDigger;

void
UIMediator::connectRequestTracker()
{
  connect(
        m_requestTracker,
        SIGNAL(opened(Suscan::AnalyzerRequest const &)),
        this,
        SLOT(onOpened(Suscan::AnalyzerRequest const &)));

  connect(
        m_requestTracker,
        SIGNAL(cancelled(Suscan::AnalyzerRequest const &)),
        this,
        SLOT(onCancelled(Suscan::AnalyzerRequest const &)));

  connect(
        m_requestTracker,
        SIGNAL(error(Suscan::AnalyzerRequest const &, const std::string &)),
        this,
        SLOT(onError(Suscan::AnalyzerRequest const &, const std::string &)));
}

void
UIMediator::connectAnalyzer()
{
  connect(
        m_analyzer,
        SIGNAL(inspector_message(const Suscan::InspectorMessage &)),
        this,
        SLOT(onInspectorMessage(const Suscan::InspectorMessage &)));

  connect(
        m_analyzer,
        SIGNAL(samples_message(const Suscan::SamplesMessage &)),
        this,
        SLOT(onInspectorSamples(const Suscan::SamplesMessage &)));
}

void
UIMediator::detachAllInspectors()
{
  // Detaching all inspectors mean that we do not care about them any longer,
  // they are just dangling tab widgets with no relation with the current
  // analyzer whatsoever

  for (auto p : m_inspectors)
    p->setState(UIMediator::HALTED, nullptr);

  m_inspectors.clear();
  m_inspTable.clear();
}

bool
UIMediator::openInspectorTab(
    const char *factoryName,
    const char *inspClass,
    Suscan::Channel channel,
    bool precise,
    Suscan::Handle handle)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  // The factory must exists
  if (s->findInspectionWidgetFactory(factoryName) == nullptr) {
    SU_WARNING("Invalid inspection tab factory: %s\n", factoryName);
    return false;
  }

  // The analyzer must be initialized
  if (m_analyzer == nullptr) {
    SU_WARNING(
          "Cannot make inspector from %s: analyzer not present\n",
          factoryName);
    return false;
  }

  this->setUIBusy(true);

  // And the opening procedure must succeed
  return m_requestTracker->requestOpen(
        inspClass,
        channel,
        QVariant::fromValue<QString>(factoryName),
        precise,
        handle);
}

void
UIMediator::detachInspectionWidget(InspectionWidget *widget)
{
  Suscan::InspectorId id = widget->request().inspectorId;

  if (m_inspTable.contains(id) && m_inspTable[id] == widget)
    m_inspTable.remove(id);

  if (m_inspectors.contains(widget))
    m_inspectors.removeAt(m_inspectors.indexOf(widget));
}

SUFREQ
UIMediator::getCurrentCenterFreq() const
{
  return this->ui->spectrum->getCenterFreq();
}

bool
UIMediator::isLive() const
{
  return this->appConfig->profile.getType() == SUSCAN_SOURCE_TYPE_SDR;
}

void
UIMediator::onInspectorMessage(Suscan::InspectorMessage const &msg)
{
  if (m_inspTable.contains(msg.getInspectorId())) {
    InspectionWidget *widget = m_inspTable[msg.getInspectorId()];

    // Deletion triggers detachAllInspectors()
    if (msg.getKind() == SUSCAN_ANALYZER_INSPECTOR_MSGKIND_CLOSE)
      widget->deleteLater();
    else
      widget->inspectorMessage(msg);
  }
}

void
UIMediator::onInspectorSamples(Suscan::SamplesMessage const &msg)
{
  if (m_inspTable.contains(msg.getInspectorId())) {
    InspectionWidget *widget = m_inspTable[msg.getInspectorId()];
    widget->samplesMessage(msg);
  }
}

void
UIMediator::onOpened(Suscan::AnalyzerRequest const &request)
{
  QString factoryName = request.data.value<QString>();
  InspectionWidgetFactory *factory;
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  this->setUIBusy(false);

  if ((factory = s->findInspectionWidgetFactory(factoryName)) == nullptr) {
    QMessageBox::critical(
          this,
          "Cannot open inspector",
          "Analyzer acknowledged the opening of the inspector, but "
          "the " + factoryName + " widget factory does not exist.");
    if (m_analyzer != nullptr)
      m_analyzer->closeInspector(request.handle);
  } else {
    InspectionWidget *widget = factory->make(request, this);

    widget->setColorConfig(appConfig->colors);
    m_inspectors.push_back(widget);
    m_inspTable[request.inspectorId] = widget;

    this->addTabWidget(widget);
  }
}

void
UIMediator::onCancelled(Suscan::AnalyzerRequest const &)
{
  this->setUIBusy(false);
}

void
UIMediator::onError(Suscan::AnalyzerRequest const &r, std::string const &error)
{
  this->setUIBusy(false);

  QMessageBox::critical(
        this,
        "Cannot open inspector",
        "Failed to open inspector (class "
        + QString::fromStdString(r.inspClass)
        + "). " + QString::fromStdString(error));
}
