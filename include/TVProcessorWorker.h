//
//    TVProcessorWorker.h: Perform TV processing
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

#ifndef TVPROCESSORWORKER_H
#define TVPROCESSORWORKER_H

#include <QObject>
#include <vector>
#include <sigutils/types.h>
#include <sigutils/tvproc.h>
#include <QAtomicInteger>
#include <QMutex>
#include <list>

#define TV_PROCESSOR_WORKER_MAX_NACK_FRAMES   100
#define TV_PROCESSOR_WORKER_MIN_NACK_RESTART   50
#define TV_PROCESSOR_MAX_PENDING_FRAMES       120

namespace SigDigger {
  class TVProcessorWorker : public QObject
  {
    Q_OBJECT

    struct sigutils_tv_processor_params defaultParams;
    su_tv_processor_t *processor = nullptr;

    QMutex pendingDataMutex;
    std::list<std::vector<SUFLOAT> *> freeList;
    std::list<std::vector<SUFLOAT> *> pendingList;

    bool blocked = false;
    SUSCOUNT frameCount = 0;
    SUSCOUNT maxProcessingBlock = 0;

    QAtomicInteger<SUSCOUNT> frameAck = 0;

    std::vector<SUFLOAT> *allocBuffer(std::vector<SUFLOAT> &);
    void putPendingBuffer(std::vector<SUFLOAT> *);
    std::vector<SUFLOAT> *takePendingBuffer(void);
    void disposeBuffer(std::vector<SUFLOAT> *);

    void work(const SUFLOAT *samples, SUSCOUNT size);

    static std::vector<SUFLOAT> *popFromList(
        std::list<std::vector<SUFLOAT> *> &list);

  public:
    explicit TVProcessorWorker(QObject *parent = nullptr);
    ~TVProcessorWorker();

    void acknowledgeFrame(void);
    void pushData(std::vector<SUFLOAT> &data);

    //
    // FIXME: assume that signals may be lost.
    //
  signals:
    void error(QString);
    void frame(struct sigutils_tv_frame_buffer *);
    void paramsChanged(sigutils_tv_processor_params);

  public slots:
    void stop(void);
    void start(void);
    void returnFrame(struct sigutils_tv_frame_buffer *);
    void process();
    void setParams(sigutils_tv_processor_params);
  };
}

#endif // TVPROCESSORWORKER_H
