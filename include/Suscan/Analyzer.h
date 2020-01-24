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
#include <Suscan/Messages/GenericMessage.h>

#include <analyzer/analyzer.h>

namespace Suscan {
  class Analyzer: public QObject
  {
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
    void read_error(void);
    void eos(void);
    void halted(void);

  public slots:
    void captureMessage(quint32 type, void *data);

  public:
    SUSCOUNT getSampleRate(void) const;
    SUSCOUNT getMeasuredSampleRate(void) const;

    void *read(uint32_t &type);
    void registerBaseBandFilter(suscan_analyzer_baseband_filter_func_t, void *);
    void setFrequency(SUFREQ freq, SUFREQ lnbFreq = 0);
    void setGain(std::string const &name, SUFLOAT val);
    void setSweepStrategy(SweepStrategy);
    void setSpectrumPartitioning(SpectrumPartitioning);
    void setAntenna(std::string const &name);
    void setBandwidth(SUFLOAT val);
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
