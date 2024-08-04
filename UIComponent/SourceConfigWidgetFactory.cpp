//
//    SourceConfigWidget.cpp: description
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

#include "SourceConfigWidgetFactory.h"
#include <Suscan/Library.h>

using namespace SigDigger;

SourceConfigWidget::SourceConfigWidget(
    SourceConfigWidgetFactory *factory,
    QWidget *parent) :
  QWidget(parent), Suscan::FeatureObject(factory)
{

}

bool
SourceConfigWidget::getPreferredRates(QList<int> &) const
{
  return false;
}

void
SourceConfigWidget::activateWidget()
{

}

bool
SourceConfigWidget::deactivateWidget()
{
  return true;
}

void
SourceConfigWidget::notifySingletonChanges()
{

}

void
SourceConfigWidget::getNativeFrequencyLimits(qint64 &min, qint64 &max) const
{
  // The whole radioelectric spectrum works
  min = -300000000000ll;
  max = +300000000000ll;
}

/////////////////////////////// Factory ///////////////////////////////////////
SourceConfigWidgetFactory::SourceConfigWidgetFactory(Suscan::Plugin *plugin) :
  Suscan::FeatureFactory(plugin)
{

}

bool
SourceConfigWidgetFactory::registerGlobally(void)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->registerSourceConfigWidgetFactory(this);
}

bool
SourceConfigWidgetFactory::unregisterGlobally(void)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->unregisterSourceConfigWidgetFactory(this);
}
