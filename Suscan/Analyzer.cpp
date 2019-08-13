//
//    Analyzer.cpp: Analyzer wrapper implementation
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

#include <iostream>

#include <QMetaType>
#include <Suscan/Analyzer.h>

Q_DECLARE_METATYPE(Suscan::Message);
Q_DECLARE_METATYPE(Suscan::ChannelMessage);
Q_DECLARE_METATYPE(Suscan::InspectorMessage);
Q_DECLARE_METATYPE(Suscan::PSDMessage);
Q_DECLARE_METATYPE(Suscan::SamplesMessage);
Q_DECLARE_METATYPE(Suscan::GenericMessage);

using namespace Suscan;

// Async thread
void
Analyzer::AsyncThread::run()
{
  void *data = nullptr;
  uint32_t type;
  bool running = true;

  // FIXME: Capture allocation exceptions!
  do {
    data = this->owner->read(type);

    switch (type) {
      // Data messages
      case SUSCAN_ANALYZER_MESSAGE_TYPE_CHANNEL:
        emit message(ChannelMessage(static_cast<struct suscan_analyzer_channel_msg *>(data)));
        break;

      case SUSCAN_ANALYZER_MESSAGE_TYPE_INSPECTOR:
        emit message(InspectorMessage(static_cast<struct suscan_analyzer_inspector_msg *>(data)));
        break;

      case SUSCAN_ANALYZER_MESSAGE_TYPE_PSD:
        emit message(PSDMessage(static_cast<struct suscan_analyzer_psd_msg *>(data)));
        break;

      case SUSCAN_ANALYZER_MESSAGE_TYPE_SAMPLES:
        emit message(SamplesMessage(static_cast<struct suscan_analyzer_sample_batch_msg *>(data)));
        break;

      // Exit conditions
      case SUSCAN_WORKER_MSG_TYPE_HALT:
      case SUSCAN_ANALYZER_MESSAGE_TYPE_EOS:
      case SUSCAN_ANALYZER_MESSAGE_TYPE_READ_ERROR:
        running = false;
        break;

      default:
        // Everything else is disposed
        suscan_analyzer_dispose_message(type, data);
        data = nullptr;
    }
  } while (running);

  // Emit exit reason
  emit message(GenericMessage(type, data));
}

Analyzer::AsyncThread::AsyncThread(Analyzer *owner)
{
  this->owner = owner;
}

// Analyzer object
void *
Analyzer::read(uint32_t &type)
{
  return suscan_analyzer_read(this->instance, &type);
}

void
Analyzer::setThrottle(unsigned int throttle)
{
  suscan_analyzer_set_throttle_async(
        this->instance,
        throttle,
        0);
}

void
Analyzer::registerBaseBandFilter(suscan_analyzer_baseband_filter_func_t func, void *privdata)
{
  SU_ATTEMPT(suscan_analyzer_register_baseband_filter(this->instance, func, privdata));
}

void
Analyzer::setFrequency(SUFREQ freq)
{
  SU_ATTEMPT(suscan_analyzer_set_freq(this->instance, freq));
}

SUSCOUNT
Analyzer::getSampleRate(void) const
{
  return suscan_analyzer_get_samp_rate(this->instance);
}

void
Analyzer::halt(void)
{
  suscan_analyzer_req_halt(this->instance);
}

// Signal slots
void
Analyzer::captureMessage(const Suscan::Message &msg)
{
  switch (msg.getType()) {
    case SUSCAN_ANALYZER_MESSAGE_TYPE_PSD:
      emit psd_message(static_cast<const Suscan::PSDMessage &>(msg));
      break;

    case SUSCAN_ANALYZER_MESSAGE_TYPE_EOS:
      emit eos();
      break;

    case SUSCAN_WORKER_MSG_TYPE_HALT:
      emit halted();
      break;

    case SUSCAN_ANALYZER_MESSAGE_TYPE_READ_ERROR:
      emit read_error();
      break;
  }
}

bool Analyzer::registered = false; // Yes, C++!

void
Analyzer::assertTypeRegistration(void)
{
  if (!Analyzer::registered) {
    qRegisterMetaType<Suscan::Message>();
    qRegisterMetaType<Suscan::GenericMessage>();
    qRegisterMetaType<Suscan::PSDMessage>();
    Analyzer::registered = true;
  }
}

// Object construction and destruction
Analyzer::Analyzer(
    struct suscan_analyzer_params const& params,
    Source::Config const& config)
{
  assertTypeRegistration();

  SU_ATTEMPT(this->instance = suscan_analyzer_new(
        &params,
        config.instance,
        &mq.mq));

  this->asyncThread = std::make_unique<AsyncThread>(this);

  connect(
        this->asyncThread.get(),
        SIGNAL(message(const Suscan::Message &)),
        this,
        SLOT(captureMessage(const Suscan::Message &)),
        Qt::QueuedConnection);

  this->asyncThread.get()->start();
}

Analyzer::~Analyzer()
{
  if (this->instance != nullptr) {
    this->halt(); // Halt while thread is still running, so the thread can be aware of it
    suscan_analyzer_destroy(this->instance);
  }
}

