//
//    Analyzer.h: Analyzer object wrapper
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

#ifndef CPP_ANALYZER_H
#define CPP_ANALYZER_H

#include <QObject>
#include <QThread>

#include <Suscan/Compat.h>
#include <Suscan/Source.h>
#include <Suscan/MQ.h>
#include <Suscan/Message.h>
#include <Suscan/Channel.h>
#include <Suscan/AnalyzerParams.h>

#include <Suscan/Messages/ChannelMessage.h>
#include <Suscan/Messages/InspectorMessage.h>
#include <Suscan/Messages/PSDMessage.h>
#include <Suscan/Messages/SamplesMessage.h>
#include <Suscan/Messages/SourceInfoMessage.h>
#include <Suscan/Messages/StatusMessage.h>
#include <Suscan/Messages/GenericMessage.h>

#include <analyzer/analyzer.h>

namespace Suscan {
  struct Orbit;

  struct AnalyzerSourceInfo {
    bool loan = false;
    struct suscan_source_info local_info;
    struct suscan_source_info *c_info = nullptr;

    AnalyzerSourceInfo() : AnalyzerSourceInfo(&this->local_info)
    {
      suscan_source_info_init(&this->local_info);
    }

    ~AnalyzerSourceInfo()
    {
      if (!this->loan)
        suscan_source_info_finalize(&this->local_info);
    }

    AnalyzerSourceInfo(struct suscan_source_info *ptr, bool loan = false)
    {
      this->loan = loan;

      if (loan) {
        suscan_source_info_init(&this->local_info);
        this->c_info = ptr;
      } else {
        SU_ATTEMPT(
              suscan_source_info_init_copy(&this->local_info, ptr));
        this->c_info = &this->local_info;
      }
    }

    AnalyzerSourceInfo(AnalyzerSourceInfo const &info)
      : AnalyzerSourceInfo(info.c_info)
    {
    }

    // Move assignation
    AnalyzerSourceInfo &
    operator=(AnalyzerSourceInfo &&rv)
    {
      std::swap(this->loan, rv.loan);
      std::swap(this->c_info, rv.c_info);
      std::swap(this->local_info, rv.local_info);

      if (!this->loan)
        this->c_info = &this->local_info;

      return *this;
    }

    // Copy assignation
    AnalyzerSourceInfo &
    operator=(const AnalyzerSourceInfo &rv)
    {
      if (!this->loan)
        suscan_source_info_finalize(&this->local_info);

      SU_ATTEMPT(
            suscan_source_info_init_copy(
              &this->local_info,
              rv.c_info));
      this->loan   = false;
      this->c_info = &this->local_info;

      return *this;
    }

    inline uint64_t
    getPermissions() const
    {
      return this->c_info->permissions;
    }

    inline bool
    testPermission(uint64_t mask) const
    {
      return (getPermissions() & mask) == mask;
    }

    inline SUSCOUNT
    getSampleRate() const
    {
      return this->c_info->source_samp_rate;
    }

    inline SUSCOUNT
    getEffectiveSampleRate() const
    {
      return this->c_info->effective_samp_rate;
    }

    inline SUFLOAT
    getMeasuredSampleRate() const
    {
      return this->c_info->measured_samp_rate;
    }

    inline SUFREQ
    getFrequency() const
    {
      return this->c_info->frequency;
    }

    inline SUFREQ
    getMinFrequency() const
    {
      return this->c_info->freq_min;
    }

    inline SUFREQ
    getMaxFrequency() const
    {
      return this->c_info->freq_max;
    }

    inline SUFREQ
    getLnbFrequency() const
    {
      return this->c_info->lnb;
    }

    inline SUFLOAT
    getBandwidth() const
    {
      return this->c_info->bandwidth;
    }

    inline std::string
    getAntenna() const
    {
      return this->c_info->antenna == nullptr
          ? "N/A"
          : std::string(this->c_info->antenna);
    }

    inline bool
    getDCRemove() const
    {
      return this->c_info->dc_remove != SU_FALSE;
    }

    inline bool
    getIQReverse() const
    {
      return this->c_info->iq_reverse != SU_FALSE;
    }

    inline bool
    getAGC() const
    {
      return this->c_info->agc != SU_FALSE;
    }

    inline float
    getPPM() const
    {
      return this->c_info->ppm;
    }

    inline bool
    isSeekable() const
    {
      return this->c_info->seekable != SU_FALSE;
    }

    inline struct timeval
    getSourceStartTime() const
    {
      return this->c_info->source_start;
    }

    inline struct timeval
    getSourceEndTime() const
    {
      return this->c_info->source_end;
    }


    inline void
    getGainInfo(std::vector<Source::GainDescription> &vec) const
    {
      unsigned int i;
      vec.clear();

      for (i = 0; i < this->c_info->gain_count; ++i)
        vec.push_back(Source::GainDescription(this->c_info->gain_list[i]));
    }

    inline void
    getAntennaList(std::vector<std::string> &vec) const
    {
      unsigned int i;
      vec.clear();

      for (i = 0; i < this->c_info->antenna_count; ++i)
        vec.push_back(this->c_info->antenna_list[i]);
    }
  };

  class Analyzer: public QObject {
    Q_OBJECT

    class AsyncThread;

  public:
    enum SweepStrategy {
      STOCHASTIC = SUSCAN_ANALYZER_SWEEP_STRATEGY_STOCHASTIC,
      PROGRESSIVE = SUSCAN_ANALYZER_SWEEP_STRATEGY_PROGRESSIVE
    };

    enum SpectrumPartitioning {
      DISCRETE   = SUSCAN_ANALYZER_SPECTRUM_PARTITIONING_DISCRETE,
      CONTINUOUS = SUSCAN_ANALYZER_SPECTRUM_PARTITIONING_CONTINUOUS
    };

  private:
    suscan_analyzer_t *instance = nullptr;
    AsyncThread *asyncThread = nullptr;
    uint32_t requestId = 0;
    uint32_t inspectorId = 0;
    SUFREQ lastFreq = 0;
    SUFREQ lastLnbFreq = 0;
    MQ mq;

    static bool registered;
    static void assertTypeRegistration();

  signals:
    void psd_message(const Suscan::PSDMessage &message);
    void inspector_message(const Suscan::InspectorMessage &message);
    void samples_message(const Suscan::SamplesMessage &message);
    void status_message(const Suscan::StatusMessage &message);
    void source_info_message(const Suscan::SourceInfoMessage &message);
    void analyzer_params(const Suscan::AnalyzerParams &params);
    void read_error();
    void eos();
    void halted();

  public slots:
    void captureMessage(quint32 type, void *data);

  public:
    uint32_t allocateRequestId();
    uint32_t allocateInspectorId();

    SUSCOUNT getSampleRate() const;
    SUSCOUNT getMeasuredSampleRate() const;
    SUFREQ   getFrequency() const;
    SUFREQ   getLnbFrequency() const;

    struct timeval getSourceTimeStamp() const;
    Suscan::AnalyzerSourceInfo getSourceInfo() const;

    void *read(uint32_t &type);
    void registerBaseBandFilter(suscan_analyzer_baseband_filter_func_t, void *);
    void registerBaseBandFilter(suscan_analyzer_baseband_filter_func_t, void *, int64_t);

    void setFrequency(SUFREQ freq, SUFREQ lnbFreq);
    void setFrequency(SUFREQ freq);
    void setGain(std::string const &name, SUFLOAT val);
    void seek(struct timeval const &tv);
    void setHistorySize(SUSCOUNT);
    void replay(bool);
    void setSweepStrategy(SweepStrategy);
    void setSpectrumPartitioning(SpectrumPartitioning);
    void setAntenna(std::string const &name);
    void setBandwidth(SUFLOAT val);
    void setPPM(SUFLOAT val);
    void setThrottle(unsigned int throttle);
    void setParams(AnalyzerParams &params);
    void setDCRemove(bool remove);
    void setIQReverse(bool reverse);
    void setAGC(bool enabled);
    void setHopRange(SUFREQ min, SUFREQ max);
    void setRelBandwidth(SUFLOAT rel_bw);
    void setBufferingSize(SUSCOUNT len);
    void halt();

    // Analyzer asynchronous requests
    void open(std::string const &inspClass, Channel const &ch, RequestId id = 0);
    void openPrecise(std::string const &inspClass, Channel const &ch, RequestId id = 0);
    void openEx(
        std::string const &inspClass,
        Channel const &ch,
        bool precise,
        Handle parent,
        RequestId id = 0);

    void setInspectorConfig(Handle handle, Config const &cfg, RequestId id = 0);
    void setInspectorId(Handle handle, InspectorId id, RequestId req_id = 0);
    void setInspectorFreq(Handle handle, SUFREQ fc, RequestId req_id = 0);
    void setInspectorBandwidth(Handle handle, SUFREQ bw, RequestId req_id = 0);
    void setInspectorWatermark(Handle handle, SUSCOUNT watermark, RequestId id = 0);
    void setSpectrumSource(Handle handle, unsigned int source, RequestId id = 0);
    void setInspectorEnabled(Handle handle, EstimatorId eid, bool, RequestId id = 0);
    void setInspectorDopplerCorrection(Handle handle, Orbit const &, RequestId id = 0);
    void disableDopplerCorrection(Handle handle, RequestId id = 0);
    void closeInspector(Handle handle, RequestId id = 0);

    // Constructors
    Analyzer(AnalyzerParams &params, Source::Config const& config);
    ~Analyzer();
  };

  // FIXME: DON'T DERIVE QTHREAD, QTHREAD IS ACTUALLY A THREAD CONTROLLER
  class Analyzer::AsyncThread: public QThread
  {
    Q_OBJECT

  private:
    Analyzer *owner;
    void run() override;

  public:
    AsyncThread(Analyzer *);

  signals:
    void message(quint32 type, void *data);
  };

};

#endif // CPP_ANALYZER_H
