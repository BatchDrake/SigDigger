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

#include <sys/time.h>

namespace SigDigger {
  class GenericDataSaver;

  class GenericDataWriter {
  public:
    virtual bool canWrite(void) const = 0;
    virtual ssize_t write(const float _Complex *data, size_t len) = 0;
    virtual bool close(void) = 0;
    virtual ~GenericDataWriter();
  };

  class GenericDataWorker : public QObject {
      Q_OBJECT

      bool failed = false;
      GenericDataSaver *instance;

    private slots:
      void onCommit(void);

    public:
      GenericDataWorker(GenericDataSaver *intance);

    signals:
      void writeFinished(quint64 usec);
      void error(void);
  };

  class GenericDataSaver : public QObject
  {
      Q_OBJECT

      std::vector<float _Complex>buffers[2];

      unsigned int rateHint;
      size_t allocation;

      unsigned int buffer = 0;
      unsigned int commitedSize;
      unsigned int ptr = 0;
      GenericDataWriter *writer = nullptr;
      bool ready = true;
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
      void setSampleRate(unsigned int i);
      void write(const float _Complex *data, size_t size);
      quint64 getSize(void) const;

      // Friend classes
      friend class GenericDataWorker;

    signals:
      void commit(void);
      void stopped(void);
      void swamped(void);
      void dataRate(qreal);

    public slots:
      void onError(void);
      void onWriteFinished(quint64 usec);
  };
}


#endif // GENERICDATASAVER_H
