//
//    ToolBarWidgetFactory.cpp: Widgets that fit in the toolbar
//    Copyright (C) 2024 Gonzalo José Carracedo Carballal
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

#include <Suscan/Library.h>
#include <ToolBarWidgetFactory.h>
#include <QToolBar>

using namespace SigDigger;

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), field)
#define LOAD(field) field = conf.get(STRINGFY(field), field)

void
ToolBarWidgetConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(visible);
}

Suscan::Object &&
ToolBarWidgetConfig::serialize()
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass(className);

  STORE(visible);

  return persist(obj);
}

ToolBarWidget::ToolBarWidget(
    ToolBarWidgetFactory *factory, UIMediator *mediator, QWidget *parent) :
  QWidget(parent), UIComponent(factory, mediator)
{

}

ToolBarWidgetFactory *
ToolBarWidget::factory() const
{
  return static_cast<ToolBarWidgetFactory *>(UIComponent::factory());
}

void
ToolBarWidget::applyConfig()
{
  QToolBar *toolBar = qobject_cast<QToolBar *>(parent());

  if (toolBar != nullptr)
    connect(
          toolBar,
          SIGNAL(visibilityChanged(bool)),
          this,
          SLOT(onVisibilityChanged(bool)));

  toolBar->setVisible(m_config->visible);
}

Suscan::Serializable *
ToolBarWidget::allocConfig()
{
  if (m_config == nullptr)
    m_config = new ToolBarWidgetConfig;

  m_config->className = factory()->name();

  return m_config;
}

void
ToolBarWidget::setToolBarVisible(bool visible)
{
  QToolBar *toolBar = qobject_cast<QToolBar *>(parent());

  if (toolBar != nullptr)
    toolBar->setVisible(visible);

  if (m_config != nullptr)
    m_config->visible = visible;
}

void
ToolBarWidget::onVisibilityChanged(bool visible)
{
  m_config->visible = visible;
}

bool
ToolBarWidgetFactory::registerGlobally()
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->registerToolBarWidgetFactory(this);
}

bool
ToolBarWidgetFactory::unregisterGlobally()
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->unregisterToolBarWidgetFactory(this);
}

ToolBarWidgetFactory::ToolBarWidgetFactory(Suscan::Plugin *plugin)
  : UIComponentFactory(plugin)
{

}
