//
//    GenericDataSaver.h: Generic data saver
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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

#ifndef GENERICDATASAVER_H
#define GENERICDATASAVER_H


#include <QObject>
#include <QThread>
#include <QMutex>
#include <vector>
#include <sigutils/types.h>
#include <sigutils/util/compat-time.h>
#include <stdint.h>

namespace SigDigger {
  class GenericDataSaver;

  // Remember: C++ templates are just a convoluted way to define C macros,
  // and unsurprisingly they are full of shortcomings. In particular, C++
  // forbids virtual template functions because it does not know how to
  // handle them.

#define TEMPLATE_FOR_WRITE_NO_MATTER_WHAT(typename_T)       \
  virtual ssize_t write(const typename_T *data, size_t len)

  class GenericDataWriter {
  public:
    virtual bool prepare(void) = 0;
    virtual bool canWrite(void) const = 0;
    virtual ssize_t write(const void *data, size_t len) = 0;

    TEMPLATE_FOR_WRITE_NO_MATTER_WHAT(uint8_t);
    TEMPLATE_FOR_WRITE_NO_MATTER_WHAT(SUFLOAT);
    TEMPLATE_FOR_WRITE_NO_MATTER_WHAT(SUCOMPLEX);

    virtual bool close(void) = 0;
    virtual std::string getError(void) const = 0;
    virtual ~GenericDataWriter();
  };
#undef TEMPLATE_FOR_WRITE_NO_MATTER_WHAT

  class GenericDataWorker : public QObject {
      Q_OBJECT

      bool failed = false;
      bool writerPrepared = false;
      GenericDataSaver *instance;

    private slots:
      void onCommit(void);
      void onPrepare(void);

    public:
      GenericDataWorker(GenericDataSaver *intance);

    signals:
      void prepared(void);
      void writeFinished(quint64 usec);
      void error(QString);
  };

  class GenericDataSaver : public QObject
  {
      Q_OBJECT

      std::vector<uint8_t>buffers[2];
      QString lastError;

      unsigned int rateHint = 0;
      size_t allocation;

      unsigned int buffer = 0;
      unsigned int commitedSize;
      unsigned int ptr = 0;
      GenericDataWriter *writer = nullptr;
      bool bufferReady = true;
      bool dataWritten = false;
      QThread workerThread;
      GenericDataWorker workerObject;

      QMutex dataMutex;

      struct timeval lastCommit;
      quint64 commitTime = 0;
      quint64 writeTime = 0;
      quint64 size = 0;

      // Private methods
      void doCommit(void);

    public:
      explicit GenericDataSaver(
          GenericDataWriter *writer,
          QObject *parent = nullptr);
      ~GenericDataSaver();

      // Public methods
      void setBufferSize(unsigned int size);
      void setSampleRate(unsigned int i);
      template<typename T> void write(const T *, size_t size);
      QString getLastError(void) const;
      quint64 getSize(void) const;

      // Friend classes
      friend class GenericDataWorker;

    signals:
      void prepare(void);
      void commit(void);

      void ready(void);
      void stopped(void);
      void swamped(void);
      void dataRate(qreal);

    public slots:
      void onPrepared(void);
      void onError(QString);
      void onWriteFinished(quint64 usec);
  };

  extern template void GenericDataSaver::write<SUCOMPLEX>(const SUCOMPLEX *, size_t);
  extern template void GenericDataSaver::write<SUFLOAT>(const SUFLOAT *, size_t);
  extern template void GenericDataSaver::write<uint8_t>(const uint8_t *, size_t);
}


#endif // GENERICDATASAVER_H
