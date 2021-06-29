//
//    StatusMessage.h: Status message
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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
#ifndef MESSAGES_STATUS_MESSAGE_H
#define MESSAGES_STATUS_MESSAGE_H

#include <Suscan/Compat.h>
#include <Suscan/Message.h>

#include <analyzer/analyzer.h>

namespace Suscan {
  class StatusMessage: public Message {
  private:
    struct suscan_analyzer_status_msg *message = nullptr; // Convenience reference

  public:
    int getCode(void) const;
    QString getMessage(void) const;

    StatusMessage();
    StatusMessage(struct suscan_analyzer_status_msg *msg);
  };
};

#endif // MESSAGES_STATUS_MESSAGE_H
