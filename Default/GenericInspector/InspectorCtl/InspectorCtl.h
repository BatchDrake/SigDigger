//
//    InspectorCtl.h: Generic inspector control
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


#ifndef INSPECTORCTL_H
#define INSPECTORCTL_H

#include <QFrame>
#include <Suscan/Config.h>

namespace SigDigger {
  class InspectorCtl : public QFrame {
      Q_OBJECT

      bool refreshing = false;
      bool dirty = false;
      float mSampleRate = 0;
      Suscan::Config *config = nullptr;

    protected:

    public:
      Suscan::Config *getConfig(void) const;
      void registerWidget(const QWidget *widget, const char *signal);
      InspectorCtl(QWidget *parent, Suscan::Config *config);

      void setSampleRate(float);
      float sampleRate(void) const;

      void refreshEntry(std::string const &, qreal);
      void refreshEntry(std::string const &, bool);
      void refreshEntry(std::string const &, uint64_t);

      bool
      getBoolean(std::string const &name) const
      {
        const Suscan::FieldValue *val;

        if ((val = this->getConfig()->get(name)) == nullptr)
          throw Suscan::Exception("No such boolean property: " + name);

        return val->getBoolean();
      }

      qreal
      getFloat(std::string const &name) const
      {
        const Suscan::FieldValue *val;

        if ((val = this->getConfig()->get(name)) == nullptr)
          throw Suscan::Exception("No such float property: " + name);

        return static_cast<qreal>(val->getFloat());
      }

      uint64_t
      getUint64(std::string const &name) const
      {
        const Suscan::FieldValue *val;

        if ((val = this->getConfig()->get(name)) == nullptr)
          throw Suscan::Exception("No such uint64 property: " + name);

        return val->getUint64();
      }

      virtual bool applicable(QString const &key) = 0;
      virtual void refreshUi(void) = 0;
      virtual void parseConfig(void) = 0;

    signals:
      void changed(void);

    public slots:
      void onWidgetActivated(void);
  };
}
#endif // INSPECTORCTL_H
