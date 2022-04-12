//
//    FeatureFactory.cpp: Abstract feature factory
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
#include <FeatureFactory.h>
#include <Plugin.h>

using namespace Suscan;


////////////////////////////// FeatureObject //////////////////////////////////
FeatureObject::FeatureObject(FeatureFactory *factory)
{
  assert(factory != nullptr);
  this->m_factory = factory;

  factory->registerInstance(this);
}

FeatureObject::~FeatureObject()
{
  this->m_factory->unregisterInstance(this);
}

///////////////////////////// FeatureFactory //////////////////////////////////
FeatureFactory::FeatureFactory(Plugin *plugin)
{
  // Construction of a FeatureFactory: notify the plugin about this new
  // factory
  if (plugin == nullptr)
    plugin = Suscan::Plugin::getDefaultPlugin();

  this->m_plugin = plugin;
  plugin->registerFactory(this);
}

FeatureFactory::~FeatureFactory()
{
  // Destruction of a FeatureFactory: notify the plugin about this removal
  assert(this->m_plugin->unregisterFactory(this));
}

void
FeatureFactory::registerInstance(FeatureObject *object)
{
  assert(this->m_plugin != nullptr);

  this->m_refSet.insert(object);
}

void
FeatureFactory::unregisterInstance(FeatureObject *object)
{
  assert(this->m_plugin != nullptr);

  this->m_refSet.remove(object);
}

bool
FeatureFactory::canBeRemoved(void) const
{
  return this->m_refSet.empty();
}
