//
//    InspToolWidgetFactory.cpp: description
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
//
#include "InspToolWidgetFactory.h"
#include "InspToolWidget.h"

using namespace SigDigger;

const char *
InspToolWidgetFactory::name() const
{
  return "InspToolWidget";
}

ToolWidget *
InspToolWidgetFactory::make(UIMediator *mediator)
{
  return new InspToolWidget(this, mediator);
}

InspToolWidgetFactory::InspToolWidgetFactory(Suscan::Plugin *plugin) :
  ToolWidgetFactory(plugin) { }

const char *
InspToolWidgetFactory::desc() const
{
  return "Channel inspection";
}

std::string
InspToolWidgetFactory::getTitle() const
{
  return desc();
}
