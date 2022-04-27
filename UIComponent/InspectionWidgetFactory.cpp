//
//    InspectionWidgetFactory.cpp: description
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
#include "InspectionWidgetFactory.h"
#include <Suscan/Library.h>
#include <UIMediator.h>

using namespace SigDigger;

int
InspectionWidget::state() const
{
  return m_state;
}

Suscan::Analyzer *
InspectionWidget::analyzer() const
{
  return m_analyzer;
}

Suscan::AnalyzerRequest const &
InspectionWidget::request() const
{
  return m_request;
}

Suscan::Config const &
InspectionWidget::config() const
{
  return m_config;
}


InspectionWidget::InspectionWidget(
    InspectionWidgetFactory *factory,
    Suscan::AnalyzerRequest const &request,
    UIMediator *mediator,
    QWidget *parent) :
  TabWidget(factory, mediator, parent),
  m_request(request),
  m_config(m_request.config)
{

}

InspectionWidget::~InspectionWidget()
{
  if (m_mediator != nullptr)
    m_mediator->detachInspectionWidget(this);
}

void
InspectionWidget::attachAnalyzer(Suscan::Analyzer *)
{
  // NO-OP
}

void
InspectionWidget::detachAnalyzer()
{
  // NO-OP
}

void
InspectionWidget::inspectorMessage(Suscan::InspectorMessage const &)
{
  // NO-OP
}

void
InspectionWidget::samplesMessage(Suscan::SamplesMessage const &)
{
 // NO-OP
}

// Overriden methods

//
// InspectionWidgets are widgets that can be in two states: attached (i.e.
// they keep a reference to an existing analyzer) or detached (no reference).
// Any inspector can transition to detached, but transition to attached
// can happen only once. The rationale is that since inspection widgets are
// bound to an existing inspector inside the analyzer object, setState with
// a new analyzer should not reuse the existing inspection tabs.
//
// This, of course, may change in the future. Maybe adding a context menu
// entry saying "reattach this inspection widget"
//

void
InspectionWidget::setState(int state, Suscan::Analyzer *analyzer)
{
  bool prevAttached = m_analyzer != nullptr;

  m_state = state;

  if (m_analyzer != analyzer) {
    if (prevAttached)
      this->detachAnalyzer();

    if (analyzer != nullptr && !m_onceAttached) {
      m_analyzer = analyzer;
      m_onceAttached = true;
      this->attachAnalyzer(analyzer);
    }
  }
}

void
InspectionWidget::closeRequested()
{
  // Close requests are handled differently, depending on whether we are
  // attached or not.

  if (m_analyzer != nullptr)
    m_analyzer->closeInspector(m_request.handle, 0);
  else
    this->deleteLater();
}


const char *
InspectionWidgetFactory::description() const
{
  return this->name();
}

TabWidget *
InspectionWidgetFactory::make(UIMediator *)
{
  throw Suscan::Exception("Cannot make InspectionWidgets directly from make()");
}

// Overriden methods
bool
InspectionWidgetFactory::registerGlobally()
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->registerInspectionWidgetFactory(this);
}

bool
InspectionWidgetFactory::unregisterGlobally()
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->unregisterInspectionWidgetFactory(this);
}

InspectionWidgetFactory::InspectionWidgetFactory(Suscan::Plugin *plugin)
  : TabWidgetFactory(plugin)
{

}
