//
//    InspectorMessage.cpp: Inspector message implementation
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

#include <Suscan/Messages/InspectorMessage.h>

using namespace Suscan;

InspectorMessage::InspectorMessage() : Message() {
  this->message = nullptr;
}

InspectorMessage::InspectorMessage(struct suscan_analyzer_inspector_msg *msg) :
  Message(SUSCAN_ANALYZER_MESSAGE_TYPE_INSPECTOR, msg),
  config(msg->kind == SUSCAN_ANALYZER_INSPECTOR_MSGKIND_GET_CONFIG ||
         msg->kind == SUSCAN_ANALYZER_INSPECTOR_MSGKIND_SET_CONFIG
         ? msg->config
         : nullptr)
{
  unsigned int i;

  this->message = msg;

  switch (msg->kind) {
    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_GET_CONFIG:
    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_SET_CONFIG:
    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_OPEN:
      this->sources.resize(static_cast<unsigned>(msg->spectsrc_count));
      for (i = 0; i < static_cast<unsigned>(msg->spectsrc_count); ++i) {
        auto name = msg->spectsrc_list[i];
        auto spectStrc = suscan_spectsrc_class_lookup(name);

        this->sources[i].name = name;
        this->sources[i].desc = spectStrc == nullptr ? name : spectStrc->desc;
      }

      this->estimators.resize(static_cast<unsigned>(msg->estimator_count));
      for (i = 0; i < static_cast<unsigned>(msg->estimator_count); ++i) {
        auto name = msg->estimator_list[i];
        auto estimator = suscan_estimator_class_lookup(name);

        this->estimators[i].name  = name;
        this->estimators[i].desc  = estimator == nullptr ? name : estimator->desc;
        this->estimators[i].field = estimator == nullptr ? "" : estimator->field;
        this->estimators[i].id    = i;
      }
      break;

    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_ORBIT_REPORT:
      this->report = OrbitReport(&msg->orbit_report, true);
      break;

     default:
      ;
  }
}

SUFLOAT *
InspectorMessage::getSpectrumData(void) const
{
  if (this->message == nullptr)
    return nullptr;

  return this->message->spectrum_data;
}

SUSCOUNT
InspectorMessage::getSpectrumLength(void) const
{
  if (this->message == nullptr)
    return 0;

  return this->message->spectrum_size;
}

SUSCOUNT
InspectorMessage::getSpectrumRate(void) const
{
  if (this->message == nullptr)
    return 0;

  return this->message->samp_rate;
}

unsigned int
InspectorMessage::getBasebandRate(void) const
{
  return this->message->fs;
}

SUFLOAT
InspectorMessage::getEquivSampleRate(void) const
{
  return this->message->equiv_fs;
}

SUFLOAT
InspectorMessage::getBandwidth(void) const
{
  return this->message->bandwidth;
}

SUFLOAT
InspectorMessage::getLo(void) const
{
  return this->message->lo;
}

Suscan::EstimatorId
InspectorMessage::getEstimatorId(void) const
{
  return this->message->estimator_id;
}

uint32_t
InspectorMessage::getSpectrumSourceId(void) const
{
  return this->message->spectsrc_id;
}

SUFLOAT
InspectorMessage::getEstimation(void) const
{
  return this->message->value;
}

enum suscan_analyzer_inspector_msgkind
InspectorMessage::getKind(void) const
{
  if (this->message == nullptr)
    return SUSCAN_ANALYZER_INSPECTOR_MSGKIND_WRONG_HANDLE;

  return this->message->kind;
}


RequestId
InspectorMessage::getRequestId(void) const
{
  if (this->message == nullptr)
    return 99999999;

  return this->message->req_id;
}

RequestId
InspectorMessage::getInspectorId(void) const
{
  if (this->message == nullptr)
    return 99999999;

  return this->message->inspector_id;
}

suscan_config_t const *
InspectorMessage::getCConfig(void) const
{
  return this->message->config;
}

Handle
InspectorMessage::getHandle(void) const
{
  if (this->message == nullptr)
    return 99999999;

  return static_cast<Handle>(this->message->handle);
}

std::string
InspectorMessage::getClass(void) const
{
  if (this->message == nullptr)
    return "null";

  return this->message->class_name;
}

std::vector<SpectrumSource> const &
InspectorMessage::getSpectrumSources(void) const
{
  return this->sources;
}

std::vector<Estimator> const &
InspectorMessage::getEstimators(void) const
{
  return this->estimators;
}

Channel
InspectorMessage::getChannel(void) const
{
  Channel ch;

  if (this->message != nullptr) {
    ch.fc = this->message->channel.fc;
    ch.bw = this->message->channel.bw;
    ch.ft = this->message->channel.ft;
  }

  return ch;
}

OrbitReport const &
InspectorMessage::getOrbitReport(void) const
{
  return this->report;
}

bool
InspectorMessage::isTLEEnabled(void) const
{
  if (this->message != nullptr)
    return this->message->tle_enable;

  return false;
}

std::string
InspectorMessage::getSignalName() const
{
  if (this->message != nullptr)
    return this->message->signal_name;

  return "";
}

SUDOUBLE
InspectorMessage::getSignalValue() const
{
  if (this->message != nullptr)
    return this->message->signal_value;

  return std::nan("");
}

