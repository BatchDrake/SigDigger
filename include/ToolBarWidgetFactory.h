//
//    ToolBarWidgetFactory.h: Widgets that fit in the toolbar
//    Copyright (C) 2024 Gonzalo José Carracedo Carballal
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


#ifndef TOOLBARWIDGETFACTORY_H
#define TOOLBARWIDGETFACTORY_H

#include <UIComponentFactory.h>
#include <QWidget>

namespace SigDigger {
  class UIMediator;
  class ToolBarWidgetFactory;

  struct ToolBarWidgetConfig : public Suscan::Serializable {
    std::string className;
    bool visible = false;

    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize() override;
  };

  class ToolBarWidget : public QWidget, public UIComponent {
    Q_OBJECT

    bool                  m_visible = false;
    ToolBarWidgetConfig  *m_config  = nullptr;

  protected:
    ToolBarWidget(ToolBarWidgetFactory *, UIMediator *, QWidget *parent = nullptr);

  public:
    ToolBarWidgetFactory *factory() const;
    virtual void applyConfig() override;
    virtual Suscan::Serializable *allocConfig() override;

    void setToolBarVisible(bool);

  public slots:
    void onVisibilityChanged(bool);
  };

  class ToolBarWidgetFactory : public UIComponentFactory {
  public:
    virtual ToolBarWidget *make(UIMediator *) = 0;

    // Overriden methods
    bool registerGlobally() override;
    bool unregisterGlobally() override;

    ToolBarWidgetFactory(Suscan::Plugin *);
  };
}

#endif // TOOLBARWIDGETFACTORY_H
