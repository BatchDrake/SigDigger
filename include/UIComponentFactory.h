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

namespace Suscan {
  struct Location;
}

class QAction;

namespace SigDigger {
  class UIMediator;
  class UIComponentFactory;
  class ColorConfig;

  class UIComponent : public Suscan::FeatureObject, public PersistentObject {
    QList<QAction *> m_toolBarActions;

  protected:
    UIMediator *m_mediator = nullptr;

    void registerAction(QAction *);
    UIComponent(UIComponentFactory *, UIMediator *);

  public:
    UIMediator *mediator() const;

    virtual void setState(int, Suscan::Analyzer *);
    virtual void setProfile(Suscan::Source::Config &);
    virtual void setColorConfig(ColorConfig const &);
    virtual void setQth(Suscan::Location const &);
    virtual void setTimeStamp(struct timeval const &);

    QList<QAction *> const &actions() const;

    virtual ~UIComponent() override;
  };

  class UIComponentFactory : public Suscan::FeatureFactory
  {
  public:
    UIComponentFactory(Suscan::Plugin *);
  };
}

#endif // UICOMPONENTFACTORY_H
