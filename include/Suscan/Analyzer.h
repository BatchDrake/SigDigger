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
  struct AnalyzerSourceInfo {
    bool loan = false;
    struct suscan_analyzer_source_info local_info;
    struct suscan_analyzer_source_info *c_info = nullptr;

    AnalyzerSourceInfo()
    {
      suscan_analyzer_source_info_init(&this->local_info);
    }

    ~AnalyzerSourceInfo()
    {
      if (!this->loan)
        suscan_analyzer_source_info_finalize(&this->local_info);
    }

    AnalyzerSourceInfo(struct suscan_analyzer_source_info *ptr, bool loan = false)
    {
      this->loan = loan;

      if (loan) {
        suscan_analyzer_source_info_init(&this->local_info);
        this->c_info = ptr;
      } else {
        SU_ATTEMPT(
              suscan_analyzer_source_info_init_copy(&this->local_info, ptr));
        this->c_info = &this->local_info;
      }
    }

    inline SUSCOUNT
    getSampleRate(void) const
    {
      return this->c_info->source_samp_rate;
    }

    inline SUSCOUNT
    getEffectiveSampleRate(void) const
    {
      return this->c_info->effective_samp_rate;
    }

    inline SUFLOAT
    getMeasuredSampleRate(void) const
    {
      return this->c_info->measured_samp_rate;
    }

    inline SUFREQ
    getFrequency(void) const
    {
      return this->c_info->frequency;
    }

    inline SUFREQ
    getMinFrequency(void) const
    {
      return this->c_info->freq_min;
    }

    inline SUFREQ
    getMaxFrequency(void) const
    {
      return this->c_info->freq_max;
    }

    inline SUFREQ
    getLnbFrequency(void) const
    {
      return this->c_info->lnb;
    }

    inline SUFLOAT
    getBandwidth(void) const
    {
      return this->c_info->bandwidth;
    }

    inline std::string
    getAntenna(void) const
    {
      return this->c_info->antenna == nullptr
          ? "N/A"
          : std::string(this->c_info->antenna);
    }

    inline bool
    getDCRemove(void) const
    {
      return this->c_info->dc_remove != SU_FALSE;
    }

    inline bool
    getIQReverse(void) const
    {
      return this->c_info->iq_reverse != SU_FALSE;
    }

    inline bool
    getAGC(void) const
    {
      return this->c_info->agc != SU_FALSE;
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

  struct Orbit {
    bool loan = false;
    orbit_t local_info;
    const orbit_t *c_info = nullptr;

    Orbit()
    {
      memset(&local_info, 0, sizeof(orbit_t));
      this->c_info = &this->local_info;
    }

    Orbit(const orbit_t *ptr, bool loan = false)
    {
      this->loan = loan;

      if (loan) {
        this->c_info = ptr;
      } else {
        this->local_info = *ptr;
        this->local_info.name = strdup(ptr->name);
        this->c_info = &this->local_info;
      }
    }

    Orbit(Orbit const &orbit) : Orbit(orbit.c_info) { }

    ~Orbit()
    {
      if (!this->loan)
        free(local_info.name);
    }

    orbit_t const &
    getCOrbit(void) const
    {
      return *this->c_info;
    }

    void debug(void) const;
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
    MQ mq;

    static bool registered;
    static void assertTypeRegistration(void);

  signals:
    void psd_message(const Suscan::PSDMessage &message);
    void inspector_message(const Suscan::InspectorMessage &message);
    void samples_message(const Suscan::SamplesMessage &message);
    void status_message(const Suscan::StatusMessage &message);
    void source_info_message(const Suscan::SourceInfoMessage &message);
    void analyzer_params(const Suscan::AnalyzerParams &params);
    void read_error(void);
    void eos(void);
    void halted(void);

  public slots:
    void captureMessage(quint32 type, void *data);

  public:
    SUSCOUNT getSampleRate(void) const;
    SUSCOUNT getMeasuredSampleRate(void) const;
    struct timeval getSourceTimeStamp(void) const;

    void *read(uint32_t &type);
    void registerBaseBandFilter(suscan_analyzer_baseband_filter_func_t, void *);
    void setFrequency(SUFREQ freq, SUFREQ lnbFreq = 0);
    void setGain(std::string const &name, SUFLOAT val);
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
    void setBufferingSize(SUSCOUNT len);
    void halt(void);

    // Analyzer asynchronous requests
    void open(std::string const &inspClass, Channel const &ch, RequestId id);
    void openPrecise(std::string const &inspClass, Channel const &ch, RequestId id);

    void setInspectorConfig(Handle handle, Config const &cfg, RequestId id);
    void setInspectorId(Handle handle, InspectorId id, RequestId req_id);
    void setInspectorFreq(Handle handle, SUFREQ fc, RequestId req_id);
    void setInspectorBandwidth(Handle handle, SUFREQ bw, RequestId req_id);
    void setInspectorWatermark(Handle handle, SUSCOUNT watermark, RequestId id);
    void setSpectrumSource(Handle handle, unsigned int source, RequestId id);
    void setInspectorEnabled(Handle handle, EstimatorId eid, bool, RequestId id);
    void setInspectorDopplerCorrection(Handle handle, Orbit const &, RequestId id);
    void disableDopplerCorrection(Handle handle, RequestId id);
    void closeInspector(Handle handle, RequestId id);

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
