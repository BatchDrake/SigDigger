//
//    SourceConfigWidget.h: description
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
#ifndef SOURCECONFIGWIDGET_H
#define SOURCECONFIGWIDGET_H

#include <FeatureFactory.h>
#include <Suscan/Source.h>
#include <QWidget>
#include <QList>
#include <analyzer/source/info.h>

namespace SigDigger {
  class SourceConfigWidgetFactory;
  class UIMediator;

  class SourceConfigWidget : public QWidget, public Suscan::FeatureObject
  {
    Q_OBJECT


  public:
    explicit SourceConfigWidget(
        SourceConfigWidgetFactory *,
        QWidget *parent = nullptr);

    // To set the config reference. Used as primary storage for settings
    virtual void setConfigRef(Suscan::Source::Config &) = 0;

    // Permissions that may be true depending on config
    virtual uint64_t getCapabilityMask() const = 0;

    // On show page
    virtual void activateWidget();

    // On load profile
    virtual bool deactivateWidget();

    // On singleton changes
    virtual void notifySingletonChanges();

    // May return false if it does not have preferred rates
    virtual bool getPreferredRates(QList<int> &) const;
    virtual void getNativeFrequencyLimits(qint64 &, qint64 &) const;

  signals:
    // To be emitted when the config changes
    void changed();
  };

  class SourceConfigWidgetFactory : public Suscan::FeatureFactory {
  public:
    virtual SourceConfigWidget *make() = 0;

    // Overriden methods
    virtual bool registerGlobally();
    virtual bool unregisterGlobally();

    SourceConfigWidgetFactory(Suscan::Plugin *);
  };
}

#endif // SOURCECONFIGWIDGET_H
