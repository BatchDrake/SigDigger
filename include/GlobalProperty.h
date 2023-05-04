//
//    GlobalProperty.h: description
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
#ifndef GLOBALPROPERTY_H
#define GLOBALPROPERTY_H

#include <QObject>
#include <QVariant>

namespace SigDigger{
  class GlobalProperty: public QObject, public QVariant
  {
    Q_OBJECT

    QString m_name;
    QString m_desc;
    bool    m_adjustable = false;

  public:
    GlobalProperty(QString const &, QString const &);
    virtual ~GlobalProperty();

    static GlobalProperty *registerProperty(QString const &, QString const &, QVariant const &);
    static GlobalProperty *lookupProperty(QString const &);

    QString name() const;
    QString desc() const;

    template<typename T>
    void
    setValue(T &&avalue)
    {
      QVariant::setValue(avalue);
      emit changed();
    }

    void
    setValue(const QVariant &value)
    {
      QVariant::setValue(value);
      emit changed();
    }

    void
    setValue(QVariant &&value)
    {
      QVariant::setValue(value);
      emit changed();
    }

    void setAdjustable(bool);
    bool adjustable() const;

  signals:
    void changed();
  };

}

#endif // GLOBALPROPERTY_H
