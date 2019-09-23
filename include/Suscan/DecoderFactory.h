//
//    DecoderFactory.h: Create decoders on demand
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

#ifndef DECODERFACTORY_H
#define DECODERFACTORY_H

#include <string>

class QObject;
class QWidget;
class ThrottleControl;

namespace Suscan {
  struct DecoderObjects;
  class DecoderFactory;

  class Decoder {
    DecoderObjects *objs = nullptr;

  public:
    DecoderObjects *getDecoderObjects(void) const
    {
      return this->objs;
    }

    virtual ~Decoder(void);

    friend class DecoderFactory;
  };

  class DecoderUI {
    DecoderObjects *objs = nullptr;

  public:
    DecoderObjects *getDecoderObjects(void) const
    {
      return this->objs;
    }

    virtual QWidget *asWidget(void) = 0;
    virtual void setThrottleControl(ThrottleControl *ptr) = 0;
    virtual ~DecoderUI(void);

    friend class DecoderFactory;
  };

  struct DecoderObjects {
    class DecoderFactory *factory = nullptr;
    class Decoder *decoder = nullptr;
    class DecoderUI *ui = nullptr;
    void *userData = nullptr;

    DecoderObjects(Decoder *decoder, DecoderUI *ui)
      : decoder(decoder), ui(ui) {}

    ~DecoderObjects(void);
  };

  class DecoderFactory
  {
  protected:
    DecoderObjects *makeFromObjects(Decoder *decoder, DecoderUI *ui);

  public:
    DecoderFactory();
    virtual ~DecoderFactory();

    virtual std::string getName(void) const = 0;
    virtual std::string getDescription(void) const = 0;
    virtual DecoderObjects *make(QWidget *parent = nullptr) = 0;
  };
}

#endif // DECODERFACTORY_H
