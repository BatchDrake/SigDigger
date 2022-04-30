//
//    include/UIListenerFactory.h: description
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
#ifndef UILISTENERFACTORY_H
#define UILISTENERFACTORY_H

#include <UIComponentFactory.h>
#include <QObject>

namespace SigDigger {
  class UIMediator;
  class UIListenerFactory;

  class UIListener : public QObject, public UIComponent {
    Q_OBJECT

  protected:
    UIListener(UIListenerFactory *, UIMediator *, QObject *parent = nullptr);
  };

  class UIListenerFactory : public UIComponentFactory {
  public:
    virtual UIListener *make(UIMediator *) = 0;

    // Overriden methods
    bool registerGlobally(void) override;
    bool unregisterGlobally(void) override;

    UIListenerFactory(Suscan::Plugin *);

    virtual std::string getTitle() const = 0; // Returns the title in the side panel
  };
}

#endif // UILISTENERFACTORY_H
