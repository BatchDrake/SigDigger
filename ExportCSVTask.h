//
//    ExportCSVTask.h: Export several arrays as CSV
//    Copyright (C) 2024 Gonzalo José Carracedo Carballal
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
#ifndef EXPORTCSVTASK_H
#define EXPORTCSVTASK_H

#include <Suscan/CancellableTask.h>
#include <QElapsedTimer>
#include <sigutils/matfile.h>
#include <iomanip>
#include <fstream>
#include "SigDiggerHelpers.h"

namespace SigDigger {
  class ExportCSVTask : public Suscan::CancellableTask
  {
      Q_OBJECT

      std::ofstream m_of;
      QElapsedTimer m_timer;
      QString       m_path;
      SUSCOUNT      m_size;

      std::map<std::string, std::vector<SUCOMPLEX>> m_columns;

      QString m_lastError;
      bool m_cancelFlag = false;

      void breathe(quint64);
      bool openCSV();
      bool exportToCSV();

    public:
      ExportCSVTask(
          QString const &path,
          unsigned int array_count,
          const char **names,
          const SUCOMPLEX **data,
          SUSCOUNT data_len,
          int,
          int);
      ~ExportCSVTask() override;
      bool attemptOpen(void);

      bool work(void) override;
      void cancel(void) override;

      QString getLastError(void) const;
  };
}

#endif // EXPORTCSVTASK_H
