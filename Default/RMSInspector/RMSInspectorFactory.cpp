//
//    RMSInspectorFactory.cpp: description
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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
#include "RMSInspectorFactory.h"
#include "RMSInspector.h"

using namespace SigDigger;

RMSInspectorFactory::RMSInspectorFactory(Suscan::Plugin *plugin) :
  InspectionWidgetFactory(plugin)
{

}

const char *
RMSInspectorFactory::name() const
{
  return "RMSInspectorFactory";
}

const char *
RMSInspectorFactory::description() const
{
  return "RMS channel inspector";
}

bool
RMSInspectorFactory::worksWith(QString inspClass) const
{
  return inspClass == "raw";
}


InspectionWidget *
RMSInspectorFactory::make(
    Suscan::AnalyzerRequest const &request,
    UIMediator *mediator)
{
  return new RMSInspector(this, request, mediator, nullptr);
}
