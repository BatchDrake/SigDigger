//
//    InspToolWidgetFactory.h: description
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
#ifndef InspToolWidgetFACTORY_H
#define InspToolWidgetFACTORY_H

#include <ToolWidgetFactory.h>

namespace SigDigger {
  class InspToolWidgetFactory : public ToolWidgetFactory
  {
  public:
    // FeatureFactory overrides
    const char *name() const override;
    const char *desc() const override;

    // ToolWidgetFactory overrides
    ToolWidget *make(UIMediator *) override;
    std::string getTitle() const override;

    InspToolWidgetFactory(Suscan::Plugin *);
  };
}

#endif // InspToolWidgetFACTORY_H
