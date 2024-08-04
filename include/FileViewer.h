//
//    FileViewer.h: File viewer
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

#ifndef FILEVIEWER_H
#define FILEVIEWER_H

#include <QObject>

class QFileDialog;

namespace SigDigger {
  class FileViewer : public QObject
  {
    Q_OBJECT

    QFileDialog *m_dialog = nullptr;

  public:
    explicit FileViewer(QObject *parent = nullptr);
    ~FileViewer();

    void processFile(QString file);

  signals:

  public slots:
    void onFileOpened();
  };
};

#endif // FILEVIEWER_H
