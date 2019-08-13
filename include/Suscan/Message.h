//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
#ifndef MESSAGE_H
#define MESSAGE_H

#include <QObject>
#include <functional>

#include <Suscan/Compat.h>

#include <analyzer/msg.h>

namespace Suscan {
  typedef uint32_t RequestId;
  typedef uint32_t InspectorId;
  typedef SUHANDLE Handle;

  class Message {
  private:
    uint32_t type;

    // These constructors are to be called by derivate classes
  protected:
    std::shared_ptr<void> c_message;
    Message(uint32_t type, void *c_message);

  public:
    uint32_t getType(void) const;

    Message(const Message &);
    Message(Message &&);

    Message& operator=(const Message &);
    Message& operator=(Message &&);

    Message(); // Come on
    virtual ~Message(); // And now Message is polymorphic. TADA!!
  };
};

#endif // MESSAGE_H
