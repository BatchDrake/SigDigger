//
//    PersistentWidget.h: widgets whose config can be saved
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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

#ifndef PERSISTENTWIDGET_H
#define PERSISTENTWIDGET_H

#include <QWidget>
#include <Suscan/Serializable.h>

namespace SigDigger {
  class PersistentObject {
    Suscan::Object hollowObject;
    Suscan::Serializable *config = nullptr;

  public:
    virtual ~PersistentObject();

    Suscan::Serializable *
    getConfig(void) const
    {
      return this->config;
    }

    void assertConfig(void);
    void loadSerializedConfig(Suscan::Object const &object);
    Suscan::Object &&getSerializedConfig(void);

    virtual Suscan::Serializable *allocConfig(void) = 0;
    virtual void applyConfig(void) = 0;

  };

  class PersistentWidget : public QWidget, public PersistentObject
  {
      Q_OBJECT

    public:
      explicit PersistentWidget(QWidget *parent = nullptr);

    signals:

    public slots:
  };
};

#endif // PERSISTENTWIDGET_H
