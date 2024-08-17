//
//    ToolWidgetFactory.h: Tool widget factory
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
#ifndef TOOLWIDGETFACTORY_H
#define TOOLWIDGETFACTORY_H

#include <UIComponentFactory.h>
#include <QWidget>

namespace SigDigger {
  class UIMediator;
  class ToolWidgetFactory;

  class ToolWidget : public QWidget, public UIComponent {
    Q_OBJECT

  protected:
    ToolWidget(ToolWidgetFactory *, UIMediator *, QWidget *parent = nullptr);

    void registerAction(QAction *action, const char *slot);
  };

  class ToolWidgetFactory : public UIComponentFactory {
  public:
    virtual ToolWidget *make(UIMediator *) = 0;

    // Overriden methods
    bool registerGlobally(void) override;
    bool unregisterGlobally(void) override;

    ToolWidgetFactory(Suscan::Plugin *);

    virtual std::string getTitle() const = 0; // Returns the title in the side panel
  };
}

#endif // TOOLWIDGETFACTORY_H
