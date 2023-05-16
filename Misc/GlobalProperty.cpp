//
//    GlobalProperty.cpp: description
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
#include "GlobalProperty.h"
#include <QHash>

using namespace SigDigger;

static QHash<QString, GlobalProperty *> g_propertyHash;

GlobalProperty::GlobalProperty(QString const &name, QString const &desc)
  : QVariant()
{
  m_name = name;
  m_desc = desc;
}

GlobalProperty::~GlobalProperty() { }

void
GlobalProperty::setAdjustable(bool adj)
{
  m_adjustable = adj;
}

bool
GlobalProperty::adjustable() const
{
  return m_adjustable;
}

QString
GlobalProperty::name() const
{
  return m_name;
}

QString
GlobalProperty::desc() const
{
  return m_desc;
}

GlobalProperty *
GlobalProperty::registerProperty(
    QString const &name,
    QString const &desc,
    QVariant const &value)
{
  GlobalProperty *newProperty;

  if (lookupProperty(name) != nullptr)
    return nullptr;

  newProperty = new GlobalProperty(name, desc);
  newProperty->setValue(value);

  g_propertyHash[name] = newProperty;

  return newProperty;
}

GlobalProperty *
GlobalProperty::lookupProperty(QString const &name)
{
  if (g_propertyHash.contains(name))
    return g_propertyHash[name];

  return nullptr;
}

QStringList
GlobalProperty::getProperties()
{
  QStringList list;

  for (auto p: g_propertyHash.keys())
    list.append(p);

  return list;
}
