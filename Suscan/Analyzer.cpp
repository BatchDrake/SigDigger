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
#include <Suscan/Library.h>
#include <Suscan/Analyzer.h>
#include <SuWidgetsHelpers.h>

Q_DECLARE_METATYPE(Suscan::Message);
Q_DECLARE_METATYPE(Suscan::ChannelMessage);
Q_DECLARE_METATYPE(Suscan::InspectorMessage);
Q_DECLARE_METATYPE(Suscan::PSDMessage);
Q_DECLARE_METATYPE(Suscan::SamplesMessage);
Q_DECLARE_METATYPE(Suscan::SourceInfoMessage);
Q_DECLARE_METATYPE(Suscan::StatusMessage);
Q_DECLARE_METATYPE(Suscan::GenericMessage);
Q_DECLARE_METATYPE(Suscan::EstimatorId);
Q_DECLARE_METATYPE(Suscan::AnalyzerParams);
Q_DECLARE_METATYPE(Suscan::OrbitReport);
Q_DECLARE_METATYPE(Suscan::Orbit);

using namespace Suscan;

// Orbit
void
Orbit::debug() const
{
  printf("SAT NAME: %s\n", this->c_info->name);
  printf("  Epoch:    %d + %g\n", this->c_info->ep_year, this->c_info->ep_day);
  printf("  MM:       %g rev / day\n", this->c_info->rev);
  printf("  dMM/dt:   %g rev / day²\n", this->c_info->drevdt);
  printf("  d²MM/dt²: %g rev / day³\n", this->c_info->d2revdt2);
  printf("  B*:       %g\n", this->c_info->bstar);
  printf("  Incl:     %gº\n", SU_RAD2DEG(this->c_info->eqinc));
  printf("  Ecc:      %g\n", this->c_info->ecc);
  printf("  Mnan:     %gº\n", SU_RAD2DEG(this->c_info->mnan));
  printf("  Argp:     %gº\n", SU_RAD2DEG(this->c_info->argp));
  printf("  RAAN:     %gº\n", SU_RAD2DEG(this->c_info->ascn));
  printf("  S. axis:  %gº\n", this->c_info->smjaxs);
  printf("  Norb:     %ld\n", this->c_info->norb);
  printf("  Satno:    %d\n", this->c_info->satno);
}

// Async thread
void
Analyzer::AsyncThread::run()
{
  void *data = nullptr;
  uint32_t type = 0;
  bool running = true;

  // FIXME: Capture allocation exceptions!
  do {
    type = -1;
    data = this->owner->read(type);

    switch (type) {
      case SUSCAN_ANALYZER_MESSAGE_TYPE_SOURCE_INFO:
      case SUSCAN_ANALYZER_MESSAGE_TYPE_INSPECTOR:
      case SUSCAN_ANALYZER_MESSAGE_TYPE_PSD:
      case SUSCAN_ANALYZER_MESSAGE_TYPE_SAMPLES:
      case SUSCAN_ANALYZER_MESSAGE_TYPE_SOURCE_INIT:
      case SUSCAN_ANALYZER_MESSAGE_TYPE_INTERNAL:
      case SUSCAN_ANALYZER_MESSAGE_TYPE_PARAMS:
        emit message(type, data);
        break;

      // Exit conditions
      case SUSCAN_WORKER_MSG_TYPE_HALT:
      case SUSCAN_ANALYZER_MESSAGE_TYPE_EOS:
      case SUSCAN_ANALYZER_MESSAGE_TYPE_READ_ERROR:
        running = false;
        suscan_analyzer_dispose_message(type, data);
        break;

      default:
        // Everything else is disposed
        suscan_analyzer_dispose_message(type, data);
        data = nullptr;
    }
  } while (running);

  // Emit exit reason
  emit message(type, data);
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
Analyzer::registerBaseBandFilter(
    suscan_analyzer_baseband_filter_func_t func,
    void *privdata,
    int64_t prio)
{
  SU_ATTEMPT(suscan_analyzer_register_baseband_filter_with_prio(
               this->instance,
               func,
               privdata,
               prio));
}

void
Analyzer::setGain(std::string const &name, SUFLOAT value)
{
  SU_ATTEMPT(suscan_analyzer_set_gain(this->instance, name.c_str(), value));
}

void
Analyzer::seek(struct timeval const &tv)
{
  SU_ATTEMPT(suscan_analyzer_seek(this->instance, &tv));
}

void
Analyzer::setHistorySize(SUSCOUNT size)
{
  SU_ATTEMPT(suscan_analyzer_set_history_size(this->instance, size));
}

void
Analyzer::replay(bool replay)
{
  SU_ATTEMPT(suscan_analyzer_replay(this->instance, replay));
}


void
Analyzer::setAntenna(std::string const &name)
{
  SU_ATTEMPT(suscan_analyzer_set_antenna(this->instance, name.c_str()));
}

void
Analyzer::setSweepStrategy(SweepStrategy strategy)
{
  SU_ATTEMPT(suscan_analyzer_set_sweep_stratrgy(
               this->instance,
               static_cast<enum suscan_analyzer_sweep_strategy>(strategy)));
}

void
Analyzer::setSpectrumPartitioning(SpectrumPartitioning partitioning)
{
  SU_ATTEMPT(suscan_analyzer_set_spectrum_partitioning(
               this->instance,
               static_cast<enum suscan_analyzer_spectrum_partitioning>(partitioning)));
}

void
Analyzer::setBandwidth(SUFLOAT value)
{
  SU_ATTEMPT(suscan_analyzer_set_bw(this->instance, value));
}

void
Analyzer::setPPM(SUFLOAT value)
{
  SU_ATTEMPT(suscan_analyzer_set_ppm(this->instance, value));
}

void
Analyzer::setFrequency(SUFREQ freq)
{
  SU_ATTEMPT(suscan_analyzer_set_freq(this->instance, freq, lastLnbFreq));
  lastFreq = freq;
}

void
Analyzer::setFrequency(SUFREQ freq, SUFREQ lnb)
{
  SU_ATTEMPT(suscan_analyzer_set_freq(this->instance, freq, lnb));
  lastLnbFreq = lnb;
  lastFreq = freq;
}

void
Analyzer::setParams(AnalyzerParams &params)
{
  SU_ATTEMPT(
        suscan_analyzer_set_params_async(
          this->instance,
          &params.getCParams(),
          0));
}

void
Analyzer::setDCRemove(bool remove)
{
  SU_ATTEMPT(
        suscan_analyzer_set_dc_remove(
          this->instance,
          remove ? SU_TRUE : SU_FALSE));
}

void
Analyzer::setIQReverse(bool remove)
{
  SU_ATTEMPT(
        suscan_analyzer_set_iq_reverse(
          this->instance,
          remove ? SU_TRUE : SU_FALSE));
}

void
Analyzer::setAGC(bool enabled)
{
  SU_ATTEMPT(
        suscan_analyzer_set_agc(this->instance, enabled ? SU_TRUE : SU_FALSE));

}

void
Analyzer::setHopRange(SUFREQ min, SUFREQ max)
{
  SU_ATTEMPT(
        suscan_analyzer_set_hop_range(this->instance, min, max));
}

void
Analyzer::setRelBandwidth(SUFLOAT rel_bw)
{
  SU_ATTEMPT(
        suscan_analyzer_set_rel_bandwidth(this->instance, rel_bw));
}

void
Analyzer::setBufferingSize(SUSCOUNT len)
{
  SU_ATTEMPT(suscan_analyzer_set_buffering_size(this->instance, len));
}

SUFREQ
Analyzer::getFrequency() const
{
  return lastFreq;
}

SUFREQ
Analyzer::getLnbFrequency() const
{
  return lastLnbFreq;
}

SUSCOUNT
Analyzer::getSampleRate() const
{
  return suscan_analyzer_get_samp_rate(this->instance);
}

SUSCOUNT
Analyzer::getMeasuredSampleRate() const
{
  return static_cast<SUSCOUNT>(
        suscan_analyzer_get_measured_samp_rate(this->instance));
}

struct timeval
Analyzer::getSourceTimeStamp() const
{
  struct timeval tv;

  suscan_analyzer_get_source_time(this->instance, &tv);

  return tv;
}

Suscan::AnalyzerSourceInfo
Analyzer::getSourceInfo() const
{
  return Suscan::AnalyzerSourceInfo(
        suscan_analyzer_get_source_info(this->instance),
        true);
}

void
Analyzer::halt()
{
  suscan_analyzer_req_halt(this->instance);
}

// Signal slots
void
Analyzer::captureMessage(quint32 type, void *data)
{
  auto asSrcInfo = static_cast<struct suscan_source_info *>(data);

  switch (type) {
    // Data messages
    case SUSCAN_ANALYZER_MESSAGE_TYPE_SOURCE_INFO:
      lastFreq = asSrcInfo->frequency;
      lastLnbFreq = asSrcInfo->lnb;
      emit source_info_message(SourceInfoMessage(asSrcInfo));
      break;

    case SUSCAN_ANALYZER_MESSAGE_TYPE_INSPECTOR:
      emit inspector_message(InspectorMessage(static_cast<struct suscan_analyzer_inspector_msg *>(data)));
      break;

    case SUSCAN_ANALYZER_MESSAGE_TYPE_PSD:
      emit psd_message(PSDMessage(static_cast<struct suscan_analyzer_psd_msg *>(data)));
      break;

    case SUSCAN_ANALYZER_MESSAGE_TYPE_SAMPLES:
      emit samples_message(SamplesMessage(static_cast<struct suscan_analyzer_sample_batch_msg *>(data)));
      break;

    case SUSCAN_ANALYZER_MESSAGE_TYPE_INTERNAL:
    case SUSCAN_ANALYZER_MESSAGE_TYPE_SOURCE_INIT:
      emit status_message(StatusMessage(static_cast<struct suscan_analyzer_status_msg *>(data)));
      break;

    case SUSCAN_ANALYZER_MESSAGE_TYPE_PARAMS:
      emit analyzer_params(AnalyzerParams(*static_cast<struct suscan_analyzer_params *>(data)));
      break;

    // Exit conditions. These have no data.
    case SUSCAN_WORKER_MSG_TYPE_HALT:
      emit halted();
      break;

    case SUSCAN_ANALYZER_MESSAGE_TYPE_EOS:
      emit eos();
      break;

    case SUSCAN_ANALYZER_MESSAGE_TYPE_READ_ERROR:
      emit read_error();
      break;

    default:
      // Everything else is disposed
      suscan_analyzer_dispose_message(type, data);
  }
}

bool Analyzer::registered = false; // Yes, C++!

void
Analyzer::assertTypeRegistration()
{
  if (!Analyzer::registered) {
    qRegisterMetaType<Suscan::Message>();
    qRegisterMetaType<Suscan::GenericMessage>();
    qRegisterMetaType<Suscan::PSDMessage>();
    qRegisterMetaType<Suscan::InspectorMessage>();
    qRegisterMetaType<Suscan::SamplesMessage>();
    qRegisterMetaType<Suscan::SourceInfoMessage>();
    qRegisterMetaType<Suscan::StatusMessage>();
    qRegisterMetaType<Suscan::AnalyzerParams>();
    qRegisterMetaType<Suscan::OrbitReport>();
    qRegisterMetaType<Suscan::Orbit>();

    Analyzer::registered = true;
  }
}

uint32_t
Analyzer::allocateRequestId()
{
  return ++this->requestId;
}

uint32_t
Analyzer::allocateInspectorId()
{
  return ++this->inspectorId;
}

void
Analyzer::open(
    std::string const &inspClass,
    Channel const &ch,
    RequestId id)
{
  struct sigutils_channel c_ch =
      sigutils_channel_INITIALIZER;

  c_ch.fc   = SCAST(SUFREQ, ch.fc);
  c_ch.ft   = SCAST(SUFREQ, ch.ft);
  c_ch.f_lo = SCAST(SUFREQ, ch.fLow);
  c_ch.f_hi = SCAST(SUFREQ, ch.fHigh);
  c_ch.bw   = SCAST(SUFLOAT, ch.fHigh - ch.fLow);

  SU_ATTEMPT(
        suscan_analyzer_open_async(
          this->instance,
          inspClass.c_str(),
          &c_ch,
          id));
}

void
Analyzer::openPrecise(
    std::string const &inspClass,
    Channel const &ch,
    RequestId id)
{
  struct sigutils_channel c_ch =
      sigutils_channel_INITIALIZER;

  c_ch.fc   = SCAST(SUFREQ, ch.fc);
  c_ch.ft   = SCAST(SUFREQ, ch.ft);
  c_ch.f_lo = SCAST(SUFREQ, ch.fLow);
  c_ch.f_hi = SCAST(SUFREQ, ch.fHigh);
  c_ch.bw   = SCAST(SUFLOAT, ch.fHigh - ch.fLow);

  SU_ATTEMPT(
        suscan_analyzer_open_ex_async(
          this->instance,
          inspClass.c_str(),
          &c_ch,
          SU_TRUE,
          -1,
          id));
}

void
Analyzer::openEx(
    std::string const &inspClass,
    Channel const &ch,
    bool precise,
    Handle parent,
    RequestId id)
{
  struct sigutils_channel c_ch =
      sigutils_channel_INITIALIZER;

  c_ch.fc   = SCAST(SUFREQ, ch.fc);
  c_ch.ft   = SCAST(SUFREQ, ch.ft);
  c_ch.f_lo = SCAST(SUFREQ, ch.fLow);
  c_ch.f_hi = SCAST(SUFREQ, ch.fHigh);
  c_ch.bw   = SCAST(SUFLOAT, ch.fHigh - ch.fLow);

  SU_ATTEMPT(
        suscan_analyzer_open_ex_async(
          this->instance,
          inspClass.c_str(),
          &c_ch,
          precise ? SU_TRUE : SU_FALSE,
          parent,
          id));
}

void
Analyzer::setInspectorConfig(Handle handle, Config const &cfg, RequestId id)
{
  SU_ATTEMPT(
        suscan_analyzer_set_inspector_config_async(
          this->instance,
          handle,
          cfg.getInstance(),
          id));
}

void
Analyzer::setInspectorId(Handle handle, InspectorId id, RequestId req_id)
{
  SU_ATTEMPT(
        suscan_analyzer_set_inspector_id_async(
          this->instance,
          handle,
          id,
          req_id));
}

void
Analyzer::setInspectorFreq(Handle handle, SUFREQ freq, RequestId)
{
  SU_ATTEMPT(
        suscan_analyzer_set_inspector_freq_overridable(
          this->instance,
          handle,
          freq));
}

void
Analyzer::setInspectorBandwidth(Handle handle, SUFREQ bw, RequestId)
{
  SU_ATTEMPT(
        suscan_analyzer_set_inspector_bandwidth_overridable(
          this->instance,
          handle,
          bw));
}

void
Analyzer::setInspectorWatermark(Handle handle, SUSCOUNT wm, RequestId req_id)
{
  SU_ATTEMPT(
        suscan_analyzer_set_inspector_watermark_async(
          this->instance,
          handle,
          wm,
          req_id));
}

void
Analyzer::setSpectrumSource(Handle handle, unsigned int src, RequestId id)
{
  SU_ATTEMPT(
        suscan_analyzer_inspector_set_spectrum_async(
          this->instance,
          handle,
          src,
          id));

}

void
Analyzer::setInspectorEnabled(
    Handle handle,
    EstimatorId eid,
    bool enabled,
    RequestId id)
{
  SU_ATTEMPT(
        suscan_analyzer_inspector_estimator_cmd_async(
          this->instance,
          handle,
          eid,
          enabled,
          id));
}

void
Analyzer::setInspectorDopplerCorrection(
    Handle handle,
    Orbit const &orbit,
    RequestId id)
{
  SU_ATTEMPT(
        suscan_analyzer_inspector_set_tle_async(
          this->instance,
          handle,
          &orbit.getCOrbit(),
          id));
}

void
Analyzer::disableDopplerCorrection(
    Handle handle,
    RequestId id)
{
  SU_ATTEMPT(
        suscan_analyzer_inspector_set_tle_async(
          this->instance,
          handle,
          nullptr,
          id));
}

void
Analyzer::closeInspector(Handle handle, RequestId id)
{
  SU_ATTEMPT(suscan_analyzer_close_async(this->instance, handle, id));
}

// Object construction and destruction
Analyzer::Analyzer(AnalyzerParams &params, Source::Config const &config)
{
  this->requestId   = SCAST(uint32_t, rand() ^ (rand() << 16));
  this->inspectorId = SCAST(uint32_t, rand() ^ (rand() << 16));

  assertTypeRegistration();

  SU_ATTEMPT(this->instance = suscan_analyzer_new(
        &params.getCParams(),
        config.instance,
        &mq.mq));

  this->asyncThread = new AsyncThread(this);

  connect(
        this->asyncThread,
        SIGNAL(message(quint32, void *)),
        this,
        SLOT(captureMessage(quint32, void *)),
        Qt::QueuedConnection);

  this->asyncThread->start();
}

Analyzer::~Analyzer()
{
  if (this->instance != nullptr) {
    this->halt(); // Halt while thread is still running, so the thread can be aware of it
    if (this->asyncThread != nullptr) {
      this->asyncThread->quit();
      this->asyncThread->wait();
      delete this->asyncThread;
      this->asyncThread = nullptr;
    }
    // Async thread is safely destroyed, proceed to destroy instance
    suscan_analyzer_destroy(this->instance);
    this->instance = nullptr;
  }
}

