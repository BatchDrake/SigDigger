//
//    include/FACSyncCracker.h: Fast Autocorrelation cracker
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

#ifndef FACSYNCCRACKER_H
#define FACSYNCCRACKER_H

#include <QObject>
#include <Decider.h>
#include <Suscan/Library.h>

namespace SigDigger {
  class FACSyncWorker : public QObject {
      Q_OBJECT

      SU_FFTW(_complex) *buffer = nullptr;
      SU_FFTW(_plan) direct = nullptr;
      SU_FFTW(_plan) reverse = nullptr;
      std::vector<SUFLOAT> fac;

    public:
      FACSyncWorker(const std::vector<Symbol> &symbols);
      ~FACSyncWorker(void);

      std::vector<SUFLOAT> const &get(void) const;

    public slots:
      void run(void);

    signals:
      void started(void);
      void done(void);
  };

  class FACSyncCracker : public QObject
  {
      Q_OBJECT

    public:
      enum State {
        ACQUIRING,
        RUNNING,
        FOUND
      };

    private:

      State state = ACQUIRING;
      std::vector<Symbol> buffer;
      std::vector<SUFLOAT> fac;
      unsigned int p = 0;
      float sigmas = 5;
      FACSyncWorker *worker = nullptr;
      QThread *workerThread = nullptr;

      // Computed members
      int delay = -1;
      std::vector<Symbol> syncSeq;
      int syncSeqStart;

      void findSync(void);
      void guess(void);

    public:
      explicit FACSyncCracker(QObject *parent = nullptr, size_t len = 65536);
      ~FACSyncCracker(void);
      void setBufferSize(size_t len);
      void restart(void);
      void feed(const Symbol *buffer, size_t len);
      int get(void) const;
      std::vector<Symbol> syncSymbols(void) const;

    signals:
      void stateChanged(int state);
      void progress(qreal);
      void full(void);

    public slots:
      void onStarted(void);
      void onDone(void);
  };
};

#endif // FACSYNCCRACKER_H
