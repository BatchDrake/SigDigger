//
//    GenericInspectorFactory.h: description
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
#ifndef GENERICINSPECTORFACTORY_H
#define GENERICINSPECTORFACTORY_H

#include <InspectionWidgetFactory.h>

namespace SigDigger{
  class GenericInspectorFactory : public InspectionWidgetFactory
  {
  public:
    GenericInspectorFactory(Suscan::Plugin *);

    const char *name() const override;

    InspectionWidget *make(
        Suscan::AnalyzerRequest const &,
        UIMediator *) override;
  };
}

#endif // GENERICINSPECTORFACTORY_H
