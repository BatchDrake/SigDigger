//
//    Tasks/ExportCSVTask.cpp: Export several arrays as CSV
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

#include <ExportCSVTask.h>
#include <QCoreApplication>

using namespace SigDigger;

#define SIGDIGGER_EXPORT_CSV_BREATHE_INTERVAL_MS 100
#define SIGDIGGER_EXPORT_CSV_BREATHE_BLOCK_SIZE  0x10000

void
ExportCSVTask::breathe(quint64 i)
{
  if (m_timer.elapsed() > SIGDIGGER_EXPORT_CSV_BREATHE_INTERVAL_MS) {
    m_timer.restart();
    emit progress(
        static_cast<qreal>(i) / static_cast<qreal>(m_size - 1),
        "Saving data");
    QCoreApplication::processEvents();
  }
}

bool
ExportCSVTask::exportToCSV()
{
  unsigned int c = 0;

  for (auto &p : m_columns) {
    if (c++ > 0)
      m_of << ",";

    m_of << p.first;
  }

  m_of << "\n";


  for (SUSCOUNT i = 0; !m_cancelFlag && i < m_size; ++i) {
    c = 0;

    for (auto &p : m_columns) {
      if (c++ > 0)
        m_of << ",";

      m_of
         << SU_C_REAL(p.second[i]) << "+"
         << SU_C_IMAG(p.second[i]) << "i";
    }


    if (i % SIGDIGGER_EXPORT_CSV_BREATHE_BLOCK_SIZE == 0)
      breathe(i);
  }

  m_of << "];\n";

  return true;
}

bool
ExportCSVTask::work()
{
  bool ok = false;

  m_timer.start();

  ok = exportToCSV();

  if (ok) {
    if (m_cancelFlag)
      emit cancelled();
    else
      emit done();
  }

  return false;
}

void
ExportCSVTask::cancel()
{
  m_cancelFlag = true;
}


QString
ExportCSVTask::getLastError() const
{
  return m_lastError;
}

bool
ExportCSVTask::openCSV()
{
  m_of = std::ofstream(m_path.toStdString().c_str(), std::ofstream::binary);

  if (!m_of.is_open()) {
    m_lastError =
        "Cannot open "
        + m_path
        + ": "
        + QString(strerror(errno));
    return false;
  }

  return true;
}

bool
ExportCSVTask::attemptOpen()
{
  return openCSV();
}

ExportCSVTask::~ExportCSVTask()
{
}

ExportCSVTask::ExportCSVTask(
    QString const &path,
    unsigned int array_count,
    const char **names,
    const SUCOMPLEX **data,
    SUSCOUNT data_len,
    int start,
    int end)
{
  m_path = path;
  
  if (start < 0)
    start = 0;
  if (end > static_cast<int>(data_len))
    end = static_cast<int>(data_len);

  m_size = end - start;

  for (unsigned i = 0; i < array_count; ++i) {
    m_columns[names[i]].resize(static_cast<unsigned>(m_size));
    m_columns[names[i]].assign(data[i] + start, data[i] + end);
  }
}
