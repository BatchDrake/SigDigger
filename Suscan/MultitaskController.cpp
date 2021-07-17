//
//    Misc/MultitaskController.cpp: Handle multiple async tasks
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
#include <Suscan/MultitaskController.h>

using namespace Suscan;

CancellableTaskContext::CancellableTaskContext(
    CancellableTask *task,
    QString const &title)
{
  this->mThread = new QThread;
  this->mTask  = task;
  this->mTitle = title;
  this->mCreationTime = QDateTime::currentDateTime();
  this->mLastUpdate = this->mCreationTime;
  this->connectAll();

  task->moveToThread(this->mThread);
}

void
CancellableTaskContext::connectAll(void)
{
  // Thread start triggers processing
  QObject::connect(
        this->mThread,
        SIGNAL(started()),
        this->mTask,
        SLOT(onWorkRequested()));

  // And thread quit trigger deleteLater()
  QObject::connect(
        this->mThread,
        SIGNAL(finished()),
        this->mThread,
        SLOT(deleteLater()));

  // Task is deleted after finalization
  QObject::connect(
        this->mThread,
        SIGNAL(finished()),
        this->mTask,
        SLOT(deleteLater()));
}

CancellableTaskContext::~CancellableTaskContext()
{
  this->mThread->quit();
}

void
CancellableTaskContext::start(void)
{
  this->mThread->start();
}

void
CancellableTaskContext::assignIndex(int index)
{
  this->mIndex = index;
}

int
CancellableTaskContext::index(void) const
{
  return this->mIndex;
}

CancellableTask *
CancellableTaskContext::task(void) const
{
  return this->mTask;
}

QDateTime
CancellableTaskContext::creationTime(void) const
{
  return this->mCreationTime;
}

qreal
CancellableTaskContext::processingRate(void) const
{
  return this->mRate;
}

void
CancellableTaskContext::setProgress(qreal value, QString message)
{
  QDateTime now = QDateTime::currentDateTime();
  qint64 elapsed = this->mLastUpdate.msecsTo(now);
  qreal delta = value - this->mLastProgressValue;

  this->mLastUpdate          = now;
  this->mLastProgressValue   = value;
  this->mLastProgressMessage = message;
  this->mRate = 1e3 * this->mTask->getDataSize() * (delta / elapsed);
}

QString
CancellableTaskContext::progressMessage(void) const
{
  return this->mLastProgressMessage;
}

qreal
CancellableTaskContext::progressValue(void) const
{
  return this->mLastProgressValue;
}

QString
CancellableTaskContext::title(void) const
{
  return this->mTitle;
}

/////////////////////////// MultitaskController ////////////////////////////////
MultitaskController::MultitaskController(QObject *parent) : QObject(parent)
{
  CancellableTask::assertTypeRegistration();
}

MultitaskController::~MultitaskController()
{
  for (auto p : this->deadList)
    delete p;

  for (auto p : this->taskList)
    delete p;
}

void
MultitaskController::connectNewTask(CancellableTask *task)
{
  connect(
        task,
        SIGNAL(progress(qreal, QString)),
        this,
        SLOT(onProgress(qreal, QString)));

  connect(
        task,
        SIGNAL(done(void)),
        this,
        SLOT(onDone(void)));

  connect(
        task,
        SIGNAL(cancelled(void)),
        this,
        SLOT(onCancelled(void)));

  connect(
        task,
        SIGNAL(error(QString)),
        this,
        SLOT(onError(QString)));

  connect(
        this,
        SIGNAL(cancel(void)),
        task,
        SLOT(onCancelRequested(void)));
}

CancellableTaskContext *
MultitaskController::findTask(CancellableTask *task) const
{
  auto it = this->reverseTaskMap.find(task);

  if (it == this->reverseTaskMap.end()) {
    fprintf(
          stderr,
          "warning: task %p not found in reverse map!\n",
          static_cast<void *>(task));
    return nullptr;
  }

  return *it;
}

void
MultitaskController::repopulateTaskVector(void)
{
  int i = 0;

  this->taskVec.resize(static_cast<int>(this->taskList.size()));

  for (auto p : this->taskList) {
    this->taskVec[i] = p;
    p->assignIndex(i);
    ++i;
  }
}

void
MultitaskController::pushTask(CancellableTask *task, QString const &title)
{
  CancellableTaskContext *ctx = new CancellableTaskContext(task, title);

  this->taskList.push_back(ctx);
  this->reverseTaskMap[task] = ctx;

  // Faster than calling repopulateTaskVector()
  this->taskVec.push_back(ctx);
  ctx->assignIndex(this->taskVec.size() - 1);

  emit taskAdded(task);

  // Start after GUI has been notified
  this->connectNewTask(task);
  ctx->start();
}

void
MultitaskController::getTaskVector(QVector<CancellableTaskContext *> &vec) const
{
  vec = this->taskVec;
}

void
MultitaskController::cancelAll(void)
{
  emit cancel();
}

void
MultitaskController::cancelByIndex(int index)
{
  if (index >= 0 && index < this->taskVec.size())
    this->taskVec[index]->task()->cancel();
}

void
MultitaskController::removeTaskContext(CancellableTaskContext *ctx)
{
  this->taskList.remove(ctx);
  this->deadList.push_back(ctx);
  this->repopulateTaskVector();
}


void
MultitaskController::cleanup(void)
{
  for (auto p : this->deadList) {
    this->reverseTaskMap.remove(p->task());
    delete p;
  }

  this->deadList.clear();
}


////////////////////////////////////// Slots ///////////////////////////////////
void
MultitaskController::onProgress(qreal progress, QString state)
{
  CancellableTaskContext *ctx = this->findTask(
        static_cast<CancellableTask *>(this->sender()));

  if (ctx != nullptr) {
    ctx->setProgress(progress, state);
    emit(taskProgress(ctx->index(), progress, state));
  }
}

void
MultitaskController::onDone(void)
{
  CancellableTaskContext *ctx = this->findTask(
        static_cast<CancellableTask *>(this->sender()));

  if (ctx != nullptr) {
    (void) this->removeTaskContext(ctx);
    emit taskDone(ctx->index());
  }
}

void
MultitaskController::onCancelled(void)
{
  CancellableTaskContext *ctx = this->findTask(
        static_cast<CancellableTask *>(this->sender()));

  if (ctx != nullptr) {
    (void) this->removeTaskContext(ctx);
    emit taskCancelled(ctx->index());
  }
}

void
MultitaskController::onError(QString message)
{
  CancellableTaskContext *ctx = this->findTask(
        static_cast<CancellableTask *>(this->sender()));

  if (ctx != nullptr) {
    (void) this->removeTaskContext(ctx);
    emit taskError(ctx->index(), message);
  }
}
