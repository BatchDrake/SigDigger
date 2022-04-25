//
//    InspectorCtl.cpp: Generic inspector control
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

#include "InspectorCtl.h"

using namespace SigDigger;

InspectorCtl::InspectorCtl(
    QWidget *parent,
    Suscan::Config *config) : QFrame(parent)
{
  this->config = config;
  this->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  this->setLineWidth(0);
}

void
InspectorCtl::registerWidget(const QWidget *widget, const char *signal)
{
  this->connect(
        widget,
        signal,
        this,
        SLOT(onWidgetActivated()));
}

Suscan::Config *
InspectorCtl::getConfig(void) const
{
  return this->config;
}

void
InspectorCtl::setSampleRate(float rate)
{
  this->mSampleRate = rate;
}

float
InspectorCtl::sampleRate(void) const
{
  return this->mSampleRate;
}

void
InspectorCtl::refreshEntry(std::string const &name, qreal val)
{
  const Suscan::FieldValue *fv;

  SU_ATTEMPT(fv = this->getConfig()->get(name));

  if (fabs(fv->getFloat() - static_cast<SUFLOAT>(val)) > 1e-5f) {
    this->getConfig()->set(name, static_cast<SUFLOAT>(val));
    this->dirty = true;
  }
}

void
InspectorCtl::refreshEntry(std::string const &name, bool val)
{
  const Suscan::FieldValue *fv;

  SU_ATTEMPT(fv = this->getConfig()->get(name));

  if (fv->getBoolean() != val) {
    this->getConfig()->set(name, val);
    this->dirty = true;
  }
}

void
InspectorCtl::refreshEntry(std::string const &name, uint64_t val)
{
  const Suscan::FieldValue *fv;

  SU_ATTEMPT(fv = this->getConfig()->get(name));

  if (fv->getUint64() != val) {
    this->getConfig()->set(name, val);
    this->dirty = true;
  }
}

void
InspectorCtl::onWidgetActivated(void)
{
  // Protect widget update from nested signal activations
  if (!this->refreshing) {
    this->refreshing = true;
    this->dirty = false;
    this->parseConfig();
    if (this->dirty) {
      this->refreshUi();
      emit changed();
    }
    this->refreshing = false;
  }
}
