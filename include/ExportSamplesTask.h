//
//    include/ExportSamplesTask.h: Description
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
#ifndef EXPORTSAMPLESTASK_H
#define EXPORTSAMPLESTASK_H

#include <CancellableTask.h>
#include <QElapsedTimer>
#include <sigutils/matfile.h>
#include <iomanip>
#include <fstream>
#include "SigDiggerHelpers.h"

namespace SigDigger {
  class ExportSamplesTask : public CancellableTask
  {
      Q_OBJECT

      std::ofstream of;
      SNDFILE *sfp = nullptr;
      su_mat_file_t *mf = nullptr;

      QElapsedTimer timer;
      QString path;
      QString format;
      std::vector<SUCOMPLEX> data;
      qreal fs;
      int start;
      int end;

      QString lastError;

      bool openMat5(void);
      bool openMatlab(void);
      bool openWav(void);

      bool exportToMat5(void);
      bool exportToMatlab(void);
      bool exportToWav(void);

      bool cancelFlag = false;

      void breathe(quint64);

    public:
      ExportSamplesTask(
          QString const &path,
          QString const &format,
          const SUCOMPLEX *data,
          size_t length,
          qreal fs,
          int start,
          int end);
      ~ExportSamplesTask() override;
      bool attemptOpen(void);

      bool work(void) override;
      void cancel(void) override;

      QString getLastError(void) const;
  };
}

#endif // EXPORTSAMPLESTASK_H
