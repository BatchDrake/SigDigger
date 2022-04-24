//
//    TabWidgetFactory.cpp: Factory for tab widgets
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
#include "TabWidgetFactory.h"
#include <Suscan/Library.h>
#include <UIMediator.h>

#include <QMenu>
#include <QAction>

using namespace SigDigger;

TabWidget::TabWidget(
    TabWidgetFactory *factory, UIMediator *mediator, QWidget *parent) :
  QWidget(parent), UIComponent(factory, mediator)
{
  m_inspectorMenu = new QMenu(this);
  m_renameInspectorTab = new QAction("&Rename...", this);
  m_detachInspectorTab = new QAction("&Detach to a separate window", this);
  m_closeInspectorTab = new QAction("&Close", this);

  m_inspectorMenu->addAction(m_renameInspectorTab);
  m_inspectorMenu->addAction(m_detachInspectorTab);
  m_inspectorMenu->addSeparator();
  m_inspectorMenu->addAction(m_closeInspectorTab);

  connect(
        m_renameInspectorTab,
        SIGNAL(triggered()),
        this,
        SLOT(onInspectorRename()));

  connect(
        m_closeInspectorTab,
        SIGNAL(triggered()),
        this,
        SIGNAL(closeRequested()));

  connect(
        m_detachInspectorTab,
        SIGNAL(triggered()),
        this,
        SIGNAL(detachRequested()));
}

TabWidget::~TabWidget()
{
  if (m_mediator != nullptr)
    (void) m_mediator->closeTabWidget(this);
}

void
TabWidget::popupMenu()
{
  m_inspectorMenu->popup(QCursor::pos());
}

void
TabWidget::floatStart()
{
  // NO-OP
}

void
TabWidget::floatEnd()
{
  // NO-OP
}

void
TabWidget::closeRequested()
{
  // Mark it for removal. Its destruction will triger a bunch of things.
  this->deleteLater();
}

void
TabWidget::onPopupMenuClose()
{
  this->closeRequested();
}

void
TabWidget::onRename()
{
  // TODO
}

void
TabWidget::onDetach()
{
  m_mediator->floatTabWidget(this);
}

bool
TabWidgetFactory::registerGlobally(void)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->registerTabWidgetFactory(this);
}

bool
TabWidgetFactory::unregisterGlobally(void)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->unregisterTabWidgetFactory(this);
}

TabWidgetFactory::TabWidgetFactory(Suscan::Plugin *plugin)
  : UIComponentFactory(plugin)
{

}
