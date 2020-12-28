//
//    TVProcessorWorker.cpp: Perform TV processing
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

#include <TVProcessorWorker.h>
#include <algorithm>

using namespace SigDigger;

Q_DECLARE_METATYPE(sigutils_tv_processor_params);

static bool typesRegistered = false;

TVProcessorWorker::TVProcessorWorker(QObject *parent) : QObject(parent)
{
  if (!typesRegistered) {
    qRegisterMetaType<sigutils_tv_processor_params>();
    typesRegistered = true;
  }
}

TVProcessorWorker::~TVProcessorWorker()
{
  std::vector<SUFLOAT> *entry = nullptr;

  this->stop();

  while ((entry = popFromList(this->pendingList)) != nullptr)
    delete entry;

  while ((entry = popFromList(this->freeList)) != nullptr)
    delete entry;
}


std::vector<SUFLOAT> *
TVProcessorWorker::popFromList(std::list<std::vector<SUFLOAT> *> &list)
{
  std::vector<SUFLOAT> *entry;

  if (list.empty())
    return nullptr;

  entry = list.front();

  list.pop_front();

  return entry;
}

std::vector<SUFLOAT> *
TVProcessorWorker::allocBuffer(std::vector<SUFLOAT> &buffer)
{
  std::vector<SUFLOAT> *entry;

  this->pendingDataMutex.lock();
  entry = popFromList(this->freeList);
  this->pendingDataMutex.unlock();

  if (entry == nullptr)
    entry = new std::vector<SUFLOAT>;

  *entry = std::move(buffer);

  return entry;
}

void
TVProcessorWorker::putPendingBuffer(std::vector<SUFLOAT> *buffer)
{
  this->pendingDataMutex.lock();
  this->pendingList.push_back(buffer);
  this->pendingDataMutex.unlock();
}

std::vector<SUFLOAT> *
TVProcessorWorker::takePendingBuffer(void)
{
  std::vector<SUFLOAT> *entry;

  this->pendingDataMutex.lock();
  entry = popFromList(this->pendingList);
  this->pendingDataMutex.unlock();

  return entry;
}

void
TVProcessorWorker::disposeBuffer(std::vector<SUFLOAT> *buffer)
{
  this->pendingDataMutex.lock();
  this->freeList.push_back(buffer);
  this->pendingDataMutex.unlock();
}

void
TVProcessorWorker::pushData(std::vector<SUFLOAT> &data)
{
  std::vector<SUFLOAT> *entry = this->allocBuffer(data);

  this->putPendingBuffer(entry);
}

void
TVProcessorWorker::work(const SUFLOAT *samples, SUSCOUNT size)
{
  SUSCOUNT currAck, diff;
  bool frameSent = false;

  if (this->processor != nullptr) {
    if (size > this->maxProcessingBlock) {
      size = this->maxProcessingBlock;
    }

    while (size-- > 0) {
      if (su_tv_processor_feed(this->processor, *samples++)) {
        if (!frameSent) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
          currAck = this->frameAck.loadRelaxed();
#else
          currAck = this->frameAck.load();
#endif // QT_VERSION

          if (currAck > this->frameCount)
            this->frameCount = currAck;

          diff = this->frameCount - currAck;

          if (this->blocked)
            this->blocked = diff > TV_PROCESSOR_WORKER_MIN_NACK_RESTART;
          else
            this->blocked = diff > TV_PROCESSOR_WORKER_MAX_NACK_FRAMES;

          if (!this->blocked) {
            ++this->frameCount;
            emit frame(su_tv_processor_take_frame(this->processor));
            frameSent = true;
          }
        }
      }
    }
  }
}

void
TVProcessorWorker::acknowledgeFrame(void)
{
  if (this->processor != nullptr)
    this->frameAck.fetchAndAddAcquire(1);
}

///////////////////////////////////// Slots ///////////////////////////////////
void
TVProcessorWorker::stop(void)
{
  std::vector<SUFLOAT> *entry;

  if (this->processor != nullptr) {
    su_tv_processor_destroy(this->processor);
    this->processor = nullptr;
    this->frameAck = 0;
    this->frameCount = 0;
    this->blocked = false;
  }

  while ((entry = this->takePendingBuffer()))
    this->disposeBuffer(entry);
}

void
TVProcessorWorker::start(void)
{
  if (this->processor == nullptr) {
    this->processor = su_tv_processor_new(&this->defaultParams);
    if (this->processor == nullptr)
      emit error("TV processor failed to initialize: invalid parameters");
  }
}

void
TVProcessorWorker::process()
{
  if (this->processor != nullptr) {
    std::vector<SUFLOAT> *entry;

    while ((entry = this->takePendingBuffer()) != nullptr) {
      this->work(entry->data(), entry->size());
      this->disposeBuffer(entry);
    }
  }
}

void
TVProcessorWorker::returnFrame(struct sigutils_tv_frame_buffer *frame)
{
  if (this->processor != nullptr)
    su_tv_processor_return_frame(this->processor, frame);
  else
    su_tv_frame_buffer_destroy(frame);
}

void
TVProcessorWorker::setParams(sigutils_tv_processor_params params)
{
  this->defaultParams = params;

  this->maxProcessingBlock =
      static_cast<SUSCOUNT>(
        TV_PROCESSOR_MAX_PENDING_FRAMES * params.line_len * params.frame_lines);
  if (this->processor != nullptr)
    if (!su_tv_processor_set_params(this->processor, &params))
      emit error("TV processor: failed to set processor parameters");
}
