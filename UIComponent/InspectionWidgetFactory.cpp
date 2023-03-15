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
#include <MainSpectrum.h>
#include <QColorDialog>

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

NamedChannelSetIterator &
InspectionWidget::namedChannel()
{
  return m_namedChannel;
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
  QAction *newColor = new QAction("Change &color", this);

  m_colorDialog = new QColorDialog(this);

  this->addAction(newColor);
  this->addSeparator();

  connect(
        this,
        SIGNAL(nameChanged(QString)),
        this,
        SLOT(onNameChanged(QString)));


  connect(
        newColor,
        SIGNAL(triggered()),
        this,
        SLOT(onRequestChangeColor()));

  connect(
        m_colorDialog,
        SIGNAL(colorSelected(const QColor &)),
        this,
        SLOT(onColorSelected(const QColor &)));
}

InspectionWidget::~InspectionWidget()
{
  if (m_mediator != nullptr) {
    if (m_haveNamedChannel)
      m_mediator->getMainSpectrum()->removeChannel(m_namedChannel);
    m_mediator->detachInspectionWidget(this);
  }
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

      //
      // Leverage this to create the named channel
      //

      if (!m_haveNamedChannel) {
        auto spectrum = m_mediator->getMainSpectrum();
        auto fc       = spectrum->getCenterFreq();

        m_namedChannel = spectrum->addChannel(
              QString::fromStdString(this->getLabel()),
              fc + static_cast<qint64>(m_request.channel.fc),
              static_cast<qint32>(m_request.channel.fLow),
              static_cast<qint32>(m_request.channel.fHigh),
              QColor("#00a8ae"),
              QColor("#00a8ae"),
              QColor("#00a8ae"));
        m_haveNamedChannel = true;
      }

      this->attachAnalyzer(analyzer);
    } else {
      m_analyzer = nullptr;

      if (m_haveNamedChannel) {
        auto spectrum = m_mediator->getMainSpectrum();
        spectrum->removeChannel(m_namedChannel);
        m_haveNamedChannel = false;
      }
    }
  }
}

void
InspectionWidget::refreshNamedChannel()
{
  if (m_haveNamedChannel) {
    auto spectrum = m_mediator->getMainSpectrum();
    spectrum->refreshChannel(m_namedChannel);
  }
}

void
InspectionWidget::closeRequested()
{
  // Close requests are handled differently, depending on whether we are
  // attached or not.

  if (m_haveNamedChannel) {
    auto spectrum = m_mediator->getMainSpectrum();
    spectrum->removeChannel(m_namedChannel);
    m_haveNamedChannel = false;
  }

  if (m_analyzer != nullptr)
    m_analyzer->closeInspector(m_request.handle, 0);
  else
    this->deleteLater();
}


////////////////////////////////// Slots ///////////////////////////////////////
void
InspectionWidget::onNameChanged(QString name)
{
  if (m_haveNamedChannel) {
    auto it = this->namedChannel();
    it.value()->name = name;

    this->refreshNamedChannel();
  }
}


void
InspectionWidget::onRequestChangeColor()
{
  m_colorDialog->show();
}

void
InspectionWidget::onColorSelected(const QColor &color)
{
  if (m_haveNamedChannel) {
    auto it = this->namedChannel();
    it.value()->boxColor    = color;
    it.value()->cutOffColor = color;
    it.value()->markerColor = color;

    this->refreshNamedChannel();
  }
}

///////////////////////// InspectionWidgetFactory //////////////////////////////
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
InspectionWidgetFactory::worksWith(QString inspClass) const
{
  return inspClass != "raw";
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

