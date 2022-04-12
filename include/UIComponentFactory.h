//
//    UIComponentFactory.h: Factory for UI components
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
#ifndef UICOMPONENTFACTORY_H
#define UICOMPONENTFACTORY_H

#include <FeatureFactory.h>
#include <Suscan/Analyzer.h>
#include <PersistentWidget.h>

namespace SigDigger {
  class UIMediator;
  class UIComponentFactory;

  class UIComponent : public Suscan::FeatureObject, PersistentObject {
    class UIMediator *m_mediator = nullptr;

  protected:
    UIComponent(UIComponentFactory *, UIMediator *);

  public:
    virtual void setState(int, Suscan::Analyzer *);
    virtual void setProfile(Suscan::Source::Config &);

    ~UIComponent() override;
  };

  class UIComponentFactory : public Suscan::FeatureFactory
  {
  public:
    UIComponentFactory(Suscan::Plugin *);
  };
}

#endif // UICOMPONENTFACTORY_H
