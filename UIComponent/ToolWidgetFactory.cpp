//
//    ToolWidgetFactory.cpp: Tool widget factory
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
#include "ToolWidgetFactory.h"
#include <Suscan/Library.h>

using namespace SigDigger;

ToolWidget::ToolWidget(
    ToolWidgetFactory *factory, UIMediator *mediator, QWidget *parent) :
  QWidget(parent), UIComponent(factory, mediator)
{

}

bool
ToolWidgetFactory::registerGlobally(void)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->registerToolWidgetFactory(this);
}

bool
ToolWidgetFactory::unregisterGlobally(void)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->unregisterToolWidgetFactory(this);
}

ToolWidgetFactory::ToolWidgetFactory(Suscan::Plugin *plugin)
  : UIComponentFactory(plugin)
{

}
