//
//    FeatureFactory.h: abstract feature factory
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
#ifndef SUSCAN_FEATUREFACTORY_H
#define SUSCAN_FEATUREFACTORY_H

#include <QSet>

namespace Suscan {
  class FeatureFactory;
  class Plugin;

  // Feature object: registers and unregisters itself from the factory
  class FeatureObject {
    FeatureFactory *m_factory = nullptr;

  protected:
    FeatureObject(FeatureFactory *factory);

  public:
    ~FeatureObject();
  };

  // Feature factory: registers abd unregisters itself from the plugin
  // and globally

  class FeatureFactory {
    QSet<FeatureObject *> m_refSet;
    Plugin *m_plugin = nullptr;

    void registerInstance(FeatureObject *object);
    void unregisterInstance(FeatureObject *object);

    friend class FeatureObject;

  public:
    virtual const char *name(void) const = 0;

    bool canBeRemoved(void) const;

    virtual bool registerGlobally(void) = 0;
    virtual bool unregisterGlobally(void) = 0;

    FeatureFactory(Plugin *);
    virtual ~FeatureFactory();
  };
}

#endif // FEATUREFACTORY_H
