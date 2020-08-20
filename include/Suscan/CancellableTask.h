//
//    CancellableTask.h: Cancellable asynchronous task
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
#ifndef CANCELLABLETASK_H
#define CANCELLABLETASK_H

#include <QObject>
#include <QThread>

namespace Suscan {
  class CancellableTask : public QObject
  {
    Q_OBJECT

    qreal prog;
    QString status;
    quint64 dataSize = 0;

  protected:
    void setDataSize(quint64);
    void setProgress(qreal progress);
    void setStatus(QString status);

  public:
    explicit CancellableTask(QObject *parent = nullptr);
    virtual ~CancellableTask(void);

    virtual bool work(void) = 0;
    virtual void cancel(void) = 0;

    QString
    getStatus(void) const
    {
      return status;
    }

    qreal
    getProgress(void) const
    {
      return prog;
    }

    quint64
    getDataSize(void) const
    {
      return this->dataSize;
    }

    static void assertTypeRegistration(void);

  public slots:
    void onWorkRequested(void);
    void onCancelRequested(void);

  signals:
    void progress(qreal, QString);
    void done(void);
    void cancelled(void);
    void error(QString);
  };

  class CancellableController : public QObject
  {
    Q_OBJECT

    QThread worker;
    CancellableTask *task = nullptr;

    bool cancelledState = false;
    bool doneReceived = false;

    void connectTask(void);

    QString name = "(no name)";

    void deleteTask(void);

  public:
    explicit CancellableController(QObject *parent = nullptr);
    ~CancellableController();

    bool process(QString const &name, CancellableTask *task);
    bool cancel(void);

    QString
    getName(void) const
    {
      return this->name;
    }

    const CancellableTask *
    getTask(void) const
    {
      return this->task;
    }

  signals:
    void cancelling(void);
    void progress(qreal, QString);
    void done(void);
    void cancelled(void);
    void error(QString);

    // Queued signals
    void queuedWork(void);
    void queuedCancel(void);

  public slots:
    void onDone(void);
    void onCancelled(void);
    void onError(QString);
    void onProgress(qreal, QString);
  };
}

#endif // CANCELLABLETASK_H
