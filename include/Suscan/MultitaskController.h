//
//    include/MultitaskController.h: Description
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
#ifndef MULTITASKCONTROLLER_H
#define MULTITASKCONTROLLER_H

#include <QObject>
#include <Suscan/CancellableTask.h>
#include <list>
#include <QMap>
#include <QVector>
#include <QThread>
#include <QDateTime>

namespace Suscan {
  //
  // CancellableTaskContext acquires ownership on
  // CancellableTask and the thread in which runs
  //

  class CancellableTaskContext {
      QThread *mThread = nullptr;
      CancellableTask *mTask = nullptr;
      QDateTime mCreationTime;
      QDateTime mLastUpdate;
      qreal mRate = 0; // Processing rate
      QString mTitle;
      QString mLastProgressMessage;
      qreal mLastProgressValue = 0;
      int mIndex = -1;

      void connectAll(void);

    public:
      CancellableTaskContext(CancellableTask *, QString const &);
      ~CancellableTaskContext();

      void start(void);
      CancellableTask *task(void) const;
      QString title(void) const;

      void assignIndex(int);
      int index(void) const;

      void setProgress(qreal, QString);
      QString progressMessage(void) const;
      qreal progressValue(void) const;
      QDateTime creationTime(void) const;
      qreal processingRate(void) const;
  };

  class MultitaskController : public QObject
  {
      Q_OBJECT

      std::list<CancellableTaskContext *> taskList;
      std::list<CancellableTaskContext *> deadList;
      QVector<CancellableTaskContext *> taskVec;
      QMap<CancellableTask *, CancellableTaskContext *> reverseTaskMap;

      CancellableTaskContext *findTask(CancellableTask *) const;
      void connectNewTask(CancellableTask *);
      void repopulateTaskVector(void);

      void removeTaskContext(CancellableTaskContext *);

    public:
      explicit MultitaskController(QObject *parent = nullptr);
      ~MultitaskController();

      void pushTask(
          CancellableTask *,
          QString const &);
      void getTaskVector(QVector<CancellableTaskContext *> &) const;
      void cancelAll(void);
      void cancelByIndex(int);
      void cleanup(void);

    signals:
      void taskAdded(CancellableTask *);
      void taskRemoved(CancellableTask *);

      void taskProgress(int, qreal, QString);
      void taskDone(int);
      void taskCancelled(int);
      void taskError(int, QString);

      void cancel(void);

    public slots:
      void onProgress(qreal, QString);
      void onDone(void);
      void onCancelled(void);
      void onError(QString);
  };
}

#endif // MULTITASKCONTROLLER_H
