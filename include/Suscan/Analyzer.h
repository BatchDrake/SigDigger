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

  private:
    suscan_analyzer_t *instance;
    std::unique_ptr<AsyncThread> asyncThread;
    MQ mq;

    static bool registered;
    static void assertTypeRegistration(void);

  signals:
    void psd_message(const Suscan::PSDMessage &message);
    void read_error(void);
    void eos(void);
    void halted(void);

  public slots:
    void captureMessage(const Suscan::Message &message);

  public:
    SUSCOUNT getSampleRate(void) const;

    void *read(uint32_t &type);
    void registerBaseBandFilter(suscan_analyzer_baseband_filter_func_t, void *);
    void setFrequency(SUFREQ freq);
    void setThrottle(unsigned int throttle);
    void halt(void);

    Analyzer(
        struct suscan_analyzer_params const& params,
        Source::Config const& config);
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
    void message(const Suscan::Message &);
  };

};

#endif // CPP_ANALYZER_H
