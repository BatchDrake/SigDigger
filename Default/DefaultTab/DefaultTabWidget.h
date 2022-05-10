//
//    DefaultTabWidget.h: description
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
#ifndef DEFAULTTABWIDGET_H
#define DEFAULTTABWIDGET_H

#include <QWidget>

#include <TabWidgetFactory.h>

namespace Ui {
  class DefaultTabWidget;
}

namespace SigDigger {
  class DefaultTabWidgetFactory;

  struct DefaultTabWidgetConfig : public Suscan::Serializable {
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize() override;
  };

  class DefaultTabWidget : public TabWidget {
    Q_OBJECT

  public:
    DefaultTabWidget(
        DefaultTabWidgetFactory *,
        UIMediator *,
        QWidget *parent = nullptr);
    ~DefaultTabWidget() override;

    virtual std::string getLabel() const override;

    virtual Suscan::Serializable *allocConfig(void) override;
    virtual void applyConfig(void) override;

  private:
    Ui::DefaultTabWidget *ui = nullptr;
  };
}

#endif // DEFAULTTABWIDGET_H
