//
//    Default/Audio/AudioWidgetFactory.cpp: description
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
#include "AudioWidgetFactory.h"
#include "AudioWidget.h"

using namespace SigDigger;

const char *
AudioWidgetFactory::name() const
{
  return "AudioWidget";
}

ToolWidget *
AudioWidgetFactory::make(UIMediator *mediator)
{
  return new AudioWidget(this, mediator);
}

AudioWidgetFactory::AudioWidgetFactory(Suscan::Plugin *plugin) :
  ToolWidgetFactory(plugin) { }

const char *
AudioWidgetFactory::desc() const
{
  return "Audio preview";
}

std::string
AudioWidgetFactory::getTitle() const
{
  return desc();
}
