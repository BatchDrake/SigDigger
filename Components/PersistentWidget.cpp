//
//    PersistentWidget.cpp: widgets whose config can be saved
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

#include "PersistentWidget.h"

using namespace SigDigger;

PersistentWidget::PersistentWidget(QWidget *parent) : QWidget(parent)
{
}

void
PersistentObject::assertConfig(void)
{
  if (this->config == nullptr)
    this->config = this->allocConfig();
}

void
PersistentObject::loadSerializedConfig(Suscan::Object const &object)
{
  this->assertConfig();

  this->config->deserialize(object);

  this->applyConfig();
}

Suscan::Object &&
PersistentObject::getSerializedConfig(void)
{
  if (this->config == nullptr) {
    fprintf(
          stderr,
          "Warning! Persistent object has not declared any configuration object.\n");
    fprintf(
          stderr,
          "This is most certainly a bug. Make sure that assertConfig() has been\n");
    fprintf(
          stderr,
          "called at least once and that allocConfig() is properly implemented.\n");

    hollowObject = Suscan::Object(SUSCAN_OBJECT_TYPE_OBJECT);
    return std::move(hollowObject);
  }

  return this->config->serialize();
}

PersistentObject::~PersistentObject()
{
  if (this->config != nullptr)
    delete this->config;
}

