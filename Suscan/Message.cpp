//
//    Message.cpp: Abstract Suscan analyzer message
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

#include <Suscan/Message.h>

using namespace Suscan;

uint32_t
Message::getType(void) const
{
  return this->type;
}

Message::Message()
{
  this->type = 0;
  this->c_message = nullptr;
}

// The madness below fixes the problem of the lifecycle of objects passed
// to QT signal slots. Again, Message is just a proxy object.
//
// TODO: Would it make sense to have a generic Proxy object to keep track
// of Suscan objects?

// C-Level constructor
Message::Message(uint32_t type, void *c_message)
{
  this->type = type;
  auto deleter = [=](void *ptr) { suscan_analyzer_dispose_message(type, ptr); };
  this->c_message = std::shared_ptr<void>(c_message, deleter);
}

// Move constructor
Message::Message(Message &&rv)
{
  this->type = rv.type;
  this->c_message = std::move(rv.c_message);
}

// Move assignation
Message &
Message::operator=(Message &&rv)
{
  this->type = rv.type;
  this->c_message = std::move(rv.c_message);

  return *this;
}

// Copy constructor
Message::Message(const Message &rv)
{
  this->type = rv.type;
  this->c_message = rv.c_message;
}

// Copy assignation
Message &
Message::operator=(const Message &rv)
{
  this->type = rv.type;
  this->c_message = rv.c_message;

  return *this;
}

// Default destructor
Message::~Message()
{
  // Lalalala
}
