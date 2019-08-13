//
//    ChannelMessage.h: Channel Message
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
#ifndef MESSAGES_CHANNEL_MESSAGE_H
#define MESSAGES_CHANNEL_MESSAGE_H

#include <Suscan/Compat.h>
#include <Suscan/Message.h>

#include <analyzer/analyzer.h>

namespace Suscan {
  class ChannelMessage: public Message {
  private:
    struct suscan_analyzer_channel_msg *message = nullptr; // Convenience reference

  public:
    ChannelMessage();
    ChannelMessage(struct suscan_analyzer_channel_msg *msg);
  };
};

#endif // MESSAGES_CHANNEL_MESSAGE_H
