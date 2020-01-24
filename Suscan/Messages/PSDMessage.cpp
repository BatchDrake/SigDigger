//
//    PSDMessage.cpp: PSD message implementation
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

#include <Suscan/Messages/PSDMessage.h>

using namespace Suscan;

PSDMessage::PSDMessage() : Message() { }

PSDMessage::PSDMessage(struct suscan_analyzer_psd_msg *msg) :
  Message(SUSCAN_ANALYZER_MESSAGE_TYPE_PSD, msg)
{
  unsigned int i;
  SUSCOUNT half_size{msg->psd_size / 2};
  SUFLOAT tmp;
  this->message = msg;

  for (i = 0; i < half_size; ++i) {
    tmp = msg->psd_data[i + half_size];
    msg->psd_data[i + half_size] = SU_POWER_DB(msg->psd_data[i]);
    msg->psd_data[i] = SU_POWER_DB(tmp);
  }
}

SUSCOUNT
PSDMessage::size(void) const
{
  const struct suscan_analyzer_psd_msg *msg
      = static_cast<struct suscan_analyzer_psd_msg *>(this->c_message.get());
  return msg->psd_size;
}

unsigned int
PSDMessage::getSampleRate(void) const
{
  const struct suscan_analyzer_psd_msg *msg
      = static_cast<struct suscan_analyzer_psd_msg *>(this->c_message.get());
  return static_cast<unsigned int>(msg->samp_rate);
}

SUFREQ
PSDMessage::getFrequency(void) const
{
  const struct suscan_analyzer_psd_msg *msg
      = static_cast<struct suscan_analyzer_psd_msg *>(this->c_message.get());

  return msg->fc;
}

const SUFLOAT *
PSDMessage::get(void) const
{
  const struct suscan_analyzer_psd_msg *msg
      = static_cast<struct suscan_analyzer_psd_msg *>(this->c_message.get());
  return msg->psd_data;
}
