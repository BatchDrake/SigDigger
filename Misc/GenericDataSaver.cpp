//
//    GenericDataSaver.h: save hight bandwidth data
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

#include "GenericDataSaver.h"
#include <unistd.h>

using namespace SigDigger;

GenericDataWriter::~GenericDataWriter()
{
  // ?
}

GenericDataWorker::GenericDataWorker(GenericDataSaver *instance)
{
  this->instance = instance;
}

void
GenericDataWorker::onCommit(void)
{
  if (!this->failed) {
    QMutexLocker locker(&this->instance->dataMutex);
    struct timeval tv, otv, sub;
    ssize_t dumped;
    size_t allocation = this->instance->allocation;
    std::vector<float _Complex> *thisBuf =
        &this->instance->buffers[1 - this->instance->buffer];
    float _Complex *buffer = thisBuf->data();
    int remaining = static_cast<int>(this->instance->commitedSize);

    locker.unlock();

    gettimeofday(&otv, nullptr);

    printf("Commited %d bytes, dumping...\n", remaining);

    while (remaining > 0) {
      dumped = this->instance->writer->write(
            buffer,
            static_cast<unsigned>(remaining));

      if (dumped < 1) {
        this->failed = true;
        emit error();
        return;
      }

      remaining -= dumped;
      buffer += dumped;
    }

    gettimeofday(&tv, nullptr);

    // Requested allocation does not match buffer size.
    if (thisBuf->size() != allocation) {
      try {
        thisBuf->resize(allocation);
      } catch (std::exception &) {
        this->failed = true;
        emit error();
        return;
      }
    }

    this->instance->ready = true;
    timersub(&tv, &otv, &sub);

    emit writeFinished(static_cast<quint64>(
          sub.tv_usec + sub.tv_sec * 1000000l));
  }
}

GenericDataSaver::GenericDataSaver(
    GenericDataWriter *writer,
    QObject *parent) : QObject(parent), workerObject(this)
{
  this->writer = writer;
  this->setSampleRate(1000000);
  QObject::connect(
        this,
        SIGNAL(commit()),
        &this->workerObject,
        SLOT(onCommit()));

  QObject::connect(
        &this->workerObject,
        SIGNAL(writeFinished(quint64)),
        this,
        SLOT(onWriteFinished(quint64)));

  QObject::connect(
        &this->workerObject,
        SIGNAL(error(void)),
        this,
        SLOT(onError()));

  // Worker object will run somewhere else
  this->workerObject.moveToThread(&this->workerThread);
  this->workerThread.start();
}

GenericDataSaver::~GenericDataSaver()
{
  this->workerThread.quit();
  this->workerThread.wait();

  if (this->writer->canWrite()) {
    QMutexLocker locker(&this->dataMutex);
    this->writer->close();
  }
}

// Protected by mutex
void
GenericDataSaver::doCommit(void)
{
  // It is okay if the worker thread is still dumping data. This is
  // just a way to signal this situation. If it was not ready, we
  // do not flip buffers and keep filling the current one.
  if (this->ready) {
    struct timeval otv = this->lastCommit;
    struct timeval sub;

    gettimeofday(&this->lastCommit, nullptr);
    timersub(&this->lastCommit, &otv, &sub);
    this->writeTime = static_cast<quint64>(
              sub.tv_usec + sub.tv_sec * 1000000l);

    this->buffer = 1 - this->buffer;
    this->commitedSize = this->ptr;
    this->size += this->commitedSize;
    this->ptr = 0;
    this->ready = false;

    emit commit();
  }
}

void
GenericDataSaver::setSampleRate(unsigned int rate)
{
  if (this->rateHint != rate) {
    QMutexLocker locker(&this->dataMutex);

    this->rateHint = rate;
    this->allocation = 3 * rate; // Up to 3 sec of data

    // No data is being written, we can reallocate here
    if (!this->dataWritten) {
      this->buffers[0].resize(this->allocation);
      this->buffers[1].resize(this->allocation);
    }
  }
}

void
GenericDataSaver::write(const float _Complex *data, size_t size)
{
  if (this->writer->canWrite()) {
    QMutexLocker locker(&this->dataMutex);
    size_t totalSize = this->buffers[this->buffer].size();
    size_t avail = totalSize - this->ptr;

    this->dataWritten = true;

    if (size > avail) {
      emit swamped();
      return;
    }

    // Copy data
    memcpy(
      this->buffers[this->buffer].data() + this->ptr,
      data,
      size * sizeof(float _Complex));

    this->ptr += size;

    if (this->ptr > totalSize / 2) {
      // Buffer starts to get filled up, issue commit request
      this->doCommit();
    }
  }
}

quint64
GenericDataSaver::getSize(void) const
{
  return this->size;
}

////////////////////////////////////// Slots //////////////////////////////////
void
GenericDataSaver::onError(void)
{
  if (this->writer->canWrite()) {
    QMutexLocker locker(&this->dataMutex);

    this->writer->close();

    emit stopped();
  }
}

void
GenericDataSaver::onWriteFinished(quint64 usec)
{
  this->commitTime = usec;

  if (this->writeTime > 0) {
    emit dataRate(
          static_cast<qreal>(this->commitTime)
          / static_cast<qreal>(this->writeTime));
  }
}
