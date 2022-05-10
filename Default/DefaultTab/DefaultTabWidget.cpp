//
//    DefaultTabWidget.cpp: description
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
#include "DefaultTabWidgetFactory.h"
#include "DefaultTabWidget.h"
#include "ui_DefaultTabWidget.h"

using namespace SigDigger;

void
DefaultTabWidgetConfig::deserialize(Suscan::Object const &)
{

}

Suscan::Object &&
DefaultTabWidgetConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("DefaultTabWidgetConfig");

  return this->persist(obj);
}


DefaultTabWidget::DefaultTabWidget(
    DefaultTabWidgetFactory *factory,
    UIMediator *mediator,
    QWidget *parent) :
  TabWidget(factory, mediator, parent),
  ui(new Ui::DefaultTabWidget)
{
  ui->setupUi(this);
}

DefaultTabWidget::~DefaultTabWidget()
{
  delete ui;
}

std::string
DefaultTabWidget::getLabel() const
{
  return "Example tab";
}

Suscan::Serializable *
DefaultTabWidget::allocConfig(void)
{
  return new DefaultTabWidgetConfig();
}

void
DefaultTabWidget::applyConfig(void)
{

}
