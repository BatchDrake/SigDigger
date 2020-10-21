//
//    StatusMessage.cpp: Status message implementation
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

#include <Suscan/Messages/StatusMessage.h>

using namespace Suscan;

StatusMessage::StatusMessage() : Message() { }

StatusMessage::StatusMessage(struct suscan_analyzer_status_msg *msg) :
  Message(SUSCAN_ANALYZER_MESSAGE_TYPE_INTERNAL, msg)
{
  this->message = msg;
}

int
StatusMessage::getCode(void) const
{
  return this->message->code;
}

QString
StatusMessage::getMessage(void) const
{
  return QString(
        this->message->err_msg == nullptr
        ? ""
        : this->message->err_msg);
}
