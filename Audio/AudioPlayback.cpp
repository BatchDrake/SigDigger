//
//    AudioPlayback.cpp: Put samples in the soundcard
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

#include <iostream>
#include "AudioPlayback.h"
#include <stdexcept>
#include <util/compat-mman.h>
#include <QCoreApplication>

#ifdef SIGDIGGER_HAVE_ALSA
#  include "AlsaPlayer.h"
#elif defined(SIGDIGGER_HAVE_PORTAUDIO)
#  include "PortAudioPlayer.h"
#endif // SIGIDGGER_HAVE_ALSA

using namespace SigDigger;

#define ATTEMPT(expr, what) \
  if ((err = expr) < 0)  \
    throw std::runtime_error("Failed to " + std::string(what) + ": " + \
      std::string(snd_strerror(err)))


///////////////////////////// Playback worker /////////////////////////////////
unsigned int
PlaybackWorker::calcBufferSizeForRate(unsigned int rate)
{
  return
      qBound(
        static_cast<unsigned int>(SIGDIGGER_AUDIO_BUFFER_SIZE_MIN),
        static_cast<unsigned int>(
          SIGDIGGER_AUDIO_BUFFER_DELAY_MS
          * 1e-3f
          * static_cast<float>(rate)),
        static_cast<unsigned int>(SIGDIGGER_AUDIO_BUFFER_SIZE));

}

PlaybackWorker::PlaybackWorker(
    AudioBufferList *instance,
    std::string const &dev,
    unsigned int sampRate)
{
  this->device     = dev;
  this->sampRate   = sampRate;
  this->instance   = instance;
  this->bufferSize = calcBufferSizeForRate(sampRate);
}

unsigned int
PlaybackWorker::getBufferSize(void) const
{
  return this->bufferSize;
}

void
PlaybackWorker::setGain(float vol)
{
  this->gain = vol;
}

void
PlaybackWorker::play(void)
{
  float *buffer;
  unsigned int i;

  while (this->player != nullptr && (buffer = this->instance->next()) != nullptr) {
    bool ok;

    for (i = 0; i < this->bufferSize; ++i)
      buffer[i] *= this->gain;

    ok = this->player->write(buffer, this->bufferSize);

    // Done with this buffer, mark as free.
    this->instance->release();

    if (!ok) {
      this->stopPlayback();
      emit error("Playback error");
    }

    // Process pending events
    QCoreApplication::processEvents();
  }

  if (this->player != nullptr)
    emit starving();
}

void
PlaybackWorker::startPlayback()
{
  if (this->player == nullptr) {
    // Reset buffer list
    this->instance->clear();

    try {
  #ifdef SIGDIGGER_HAVE_ALSA
    this->player = new AlsaPlayer(
          this->device,
          this->sampRate,
          this->bufferSize);
  #elif defined(SIGDIGGER_HAVE_PORTAUDIO)
    this->player = new PortAudioPlayer(
          this->device,
          this->sampRate,
          this->bufferSize);
  #else
    throw std::runtime_error(
        "Cannot create audio playback object: audio support disabled at compile time");
  #endif // SIGDIGGER_HAVE_ALSA

      emit ready();
    }  catch (std::runtime_error &e) {
      this->player = nullptr;
      emit error(QString::fromStdString(e.what()));
    }
  }
}

void
PlaybackWorker::stopPlayback()
{
  if (this->player != nullptr) {
    delete this->player;
    this->player = nullptr;
  }
}

void
PlaybackWorker::setSampleRate(unsigned int rate)
{
  if (this->sampRate != rate) {
    this->sampRate = rate;
    this->bufferSize = calcBufferSizeForRate(rate);
    if (this->player != nullptr) {
      this->stopPlayback();
      this->startPlayback();
    }
  }
}

void
PlaybackWorker::halt(void)
{
  this->stopPlayback();
  emit finished();
}

////////////////////////////////// Audio buffer ///////////////////////////////
AudioBuffer::AudioBuffer()
{
  float *buf;

  // Ladies and gentlemen, behold the COBOL
  if ((buf = static_cast<float *>(mmap(
         nullptr,
         SIGDIGGER_AUDIO_BUFFER_ALLOC,
         PROT_READ | PROT_WRITE,
         MAP_PRIVATE | MAP_ANONYMOUS,
         -1,
         0))) == reinterpret_cast<float *>(-1)) {
    throw std::runtime_error(
          "Failed to allocate "
          + std::to_string(SIGDIGGER_AUDIO_BUFFER_ALLOC)
          + " of buffer bytes for soundcard.");
  }

  this->data = buf;
}

AudioBuffer::~AudioBuffer()
{
  if (this->data != nullptr)
    munmap(this->data, SIGDIGGER_AUDIO_BUFFER_ALLOC);
}

////////////////////////////// AudioBufferList /////////////////////////////////
AudioBufferList::AudioBufferList(unsigned int num) : listMutex(QMutex::Recursive)
{
  unsigned int i;

  this->allocation.resize(num);

  for (i = 0; i < num; ++i) {
    AudioBuffer *buffer = &this->allocation[i];
    buffer->next = this->freeList;
    this->freeList = buffer;
  }

  this->freeLen = this->totalLen = num;
}

void
AudioBufferList::reset(void)
{
  QMutexLocker(&this->listMutex);

  if (this->current != nullptr) {
    AudioBuffer *buffer = this->current;
    this->current = nullptr;

    buffer->next = this->freeList;
    this->freeList = buffer;
  }

  if (this->playBuffer != nullptr) {
    AudioBuffer *buffer = this->playBuffer;
    this->playBuffer = nullptr;

    buffer->next = this->freeList;
    this->freeList = buffer;
  }

  // Moving the current playlist is actually pretty easy

  if (this->playListHead != nullptr) {
    this->playListTail->next = this->freeList;
    this->freeList = this->playListHead;

    this->playListHead = this->playListTail = nullptr;
  }

  this->freeLen = this->totalLen;
  this->playListLen = 0;
}

float *
AudioBufferList::reserve(void)
{
  QMutexLocker(&this->listMutex);

  // You cannot reserve a buffer before committing int
  if (this->current != nullptr) {
    std::cerr << "Invalid reserve(), please call commit() first!" << std::endl;
    return nullptr;
  } else if (this->freeList == nullptr) {
    // std::cerr << "Free audio buffer list exhausted!" << std::endl;
    return nullptr;
  }

  this->current = this->freeList;
  this->freeList = this->freeList->next;
  --this->freeLen;

  return this->current->data;
}

void
AudioBufferList::commit(void)
{
  QMutexLocker(&this->listMutex);

  // You cannot commit if the current buffer is null
  if (this->current == nullptr) {
    std::cerr << "Calling commit() with no reserve() is forbidden." << std::endl;
  } else {
    AudioBuffer *buffer = this->current;
    this->current = nullptr;

    buffer->prev = nullptr;
    buffer->next = this->playListHead;
    if (this->playListHead != nullptr)
      this->playListHead->prev = buffer;

    this->playListHead = buffer;

    if (this->playListTail == nullptr)
      this->playListTail = buffer;
    ++this->playListLen;
  }
}

float *
AudioBufferList::next(void)
{
  QMutexLocker(&this->listMutex);

  if (this->playBuffer != nullptr) {
    std::cerr << "Invalid next(), please call release() first!" << std::endl;
    return nullptr;
  } else if (this->playListTail == nullptr) {
    // Starving
    return nullptr;
  } else {
    AudioBuffer *buffer = this->playListTail;
    this->playBuffer = buffer;

    this->playListTail = buffer->prev;

    if (this->playListTail == nullptr)
      this->playListHead = nullptr;
    --this->playListLen;

    return buffer->data;
  }
}

void
AudioBufferList::release(void)
{
  QMutexLocker(&this->listMutex);

  if (this->playBuffer == nullptr) {
    std::cerr << "Invalid release(), please call next() first!" << std::endl;
  } else {
    AudioBuffer *buffer = this->playBuffer;
    this->playBuffer = nullptr;

    buffer->next = this->freeList;
    this->freeList = buffer;
    ++this->freeLen;
  }
}

void
AudioBufferList::clear(void)
{
  QMutexLocker(&this->listMutex);

  // You cannot commit if the current buffer is null
  if (this->current != nullptr) {
    AudioBuffer *buffer = this->current;
    this->current = nullptr;

    buffer->prev = nullptr;
    buffer->next = this->playListHead;
    if (this->playListHead != nullptr)
      this->playListHead->prev = buffer;

    this->playListHead = buffer;

    if (this->playListTail == nullptr)
      this->playListTail = buffer;
    ++this->playListLen;
  }

  while (next())
    release();
}

//////////////////////////////// AudioBuffer ///////////////////////////////////
AudioPlayback::AudioPlayback(std::string const &dev, unsigned int rate)
  : bufferList(SIGDIGGER_AUDIO_BUFFER_NUM)
{
  this->device = dev;
  this->sampRate = rate;
  this->bufferSize = PlaybackWorker::calcBufferSizeForRate(rate);
  this->startWorker();
}

void
AudioPlayback::startWorker()
{
  this->worker = new PlaybackWorker(
        &this->bufferList,
        this->device,
        this->sampRate);

  this->workerThread = new QThread();

  this->worker->moveToThread(this->workerThread);

  connect(
        this->worker,
        SIGNAL(error(QString)),
        this,
        SLOT(onError(QString)));

  connect(
        this->worker,
        SIGNAL(starving()),
        this,
        SLOT(onStarving()));

  connect(
        this->worker,
        SIGNAL(ready()),
        this,
        SLOT(onReady()));

  // On restart, play
  connect(
        this,
        SIGNAL(restart()),
        this->worker,
        SLOT(play()));

  connect(
        this,
        SIGNAL(startPlayback()),
        this->worker,
        SLOT(startPlayback()));

  connect(
        this,
        SIGNAL(stopPlayback()),
        this->worker,
        SLOT(stopPlayback()));

  connect(
        this,
        SIGNAL(sampleRate(unsigned int)),
        this->worker,
        SLOT(setSampleRate(unsigned int)));

  connect(
        this,
        SIGNAL(gain(float)),
        this->worker,
        SLOT(setGain(float)));

  this->workerThread->start();
}

void
AudioPlayback::onError(QString desc)
{
  this->running = false;
  emit error(desc);
}

void
AudioPlayback::onStarving(void)
{
  if (this->bufferList.getPlayListLen() < SIGDIGGER_AUDIO_BUFFERING_WATERMARK) {
    this->completed = 0;
    this->buffering = true;
    std::cout << "AudioPlayback: reached watermark, buffering again..." << std::endl;
  } else {
    emit restart();
  }
}

AudioPlayback::~AudioPlayback()
{
  emit halt();

  if (this->workerThread != nullptr) {
    this->workerThread->quit();
    this->workerThread->wait();
    delete this->workerThread;

    if (this->worker != nullptr)
      delete this->worker;
  }
}

unsigned int
AudioPlayback::getSampleRate(void) const
{
  return this->sampRate;
}

void
AudioPlayback::cancelPlayBack(void)
{
  this->bufferSize = PlaybackWorker::calcBufferSizeForRate(this->sampRate);
  this->ptr = 0;
  this->ready = false;
  this->current_buffer = nullptr;
  this->completed = 0;
}

void
AudioPlayback::setSampleRate(unsigned int rate)
{
  if (this->sampRate != rate) {
    this->sampRate = rate;
    this->cancelPlayBack();
    emit sampleRate(rate);
  }
}

float
AudioPlayback::getVolume(void) const
{
  return this->volume;
}

void
AudioPlayback::setVolume(float vol)
{
  SUFLOAT gainVal;

  if (vol < -60)
    gainVal = 0;
  else
    gainVal = SU_MAG_RAW(vol);

  this->volume = vol;
  emit gain(gainVal);
}

void
AudioPlayback::start(void)
{
  if (!this->running) {
    emit startPlayback();
    this->buffering = true;
    this->running = true;
  }
}

void
AudioPlayback::stop(void)
{
  if (this->running) {
    this->cancelPlayBack();
    emit stopPlayback();
    this->running = false;
  }
}

void
AudioPlayback::write(const SUCOMPLEX *samples, SUSCOUNT size)
{
  unsigned int bufferSize = this->bufferSize;

  while (size > 0 && this->running && this->ready) {
    SUSCOUNT chunk = size;
    SUSCOUNT remaining;
    float *start;

    // No current buffer, try to allocate
    if (this->current_buffer == nullptr) {
      this->ptr = 0;
      if ((this->current_buffer = this->bufferList.reserve()) == nullptr) {
        // Somehow the playback thread is slow...
        return;
      }
    }

    start = this->current_buffer + this->ptr;

    if (chunk > bufferSize - this->ptr)
      chunk = bufferSize - this->ptr;
    remaining = chunk;

    while (remaining-- > 0)
      *start++ = SU_C_REAL(*samples++);

    this->ptr += chunk;
    size -= chunk;

    // Buffer full, send to playback thread.
    if (this->ptr == bufferSize) {
      this->current_buffer = nullptr;
      this->bufferList.commit();

      // If buffering, we wait until we have SIGDIGGER_AUDIO_BUFFER_MIN
      // buffers full. When that happens, we restart the thread.
      if (this->buffering) {
        if (++this->completed == SIGDIGGER_AUDIO_BUFFER_MIN) {
          emit restart();
          this->buffering = false;
        }
      }
    }
  }
}

void
AudioPlayback::onReady(void)
{
  this->ready = true;
}
