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
#include <sys/mman.h>

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
PlaybackWorker::PlaybackWorker(
    AudioBufferList *instance,
    GenericAudioPlayer *player)
{
  this->player   = player;
  this->instance = instance;
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

  while (!this->halting && (buffer = this->instance->next()) != nullptr) {
    bool ok;

    for (i = 0; i < SIGDIGGER_AUDIO_BUFFER_SIZE; ++i)
      buffer[i] *= this->gain;

    ok = this->player->write(buffer, SIGDIGGER_AUDIO_BUFFER_SIZE);

    // Done with this buffer, mark as free.
    this->instance->release();

    if (!ok)
      emit error();
  }

  if (this->halting)
    emit finished();
  else
    emit starving();
}

void
PlaybackWorker::halt(void)
{
  this->halting = true;
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
AudioBufferList::AudioBufferList(unsigned int num)
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

//////////////////////////////// AudioBuffer ///////////////////////////////////
AudioPlayback::AudioPlayback(std::string const &dev, unsigned int rate)
  : bufferList(SIGDIGGER_AUDIO_BUFFER_NUM)
{
#ifdef SIGDIGGER_HAVE_ALSA
  this->player = new AlsaPlayer(dev, rate, SIGDIGGER_AUDIO_BUFFER_SIZE);
#elif defined(SIGDIGGER_HAVE_PORTAUDIO)
  this->player = new PortAudioPlayer(dev, rate, SIGDIGGER_AUDIO_BUFFER_SIZE);
#else
  throw std::runtime_error(
      "Cannot create audio playback object: audio support disabled at compile time");
#endif // SIGDIGGER_HAVE_ALSA

  this->sampRate = rate;

  this->startWorker();
}

void
AudioPlayback::startWorker()
{
  this->worker = new PlaybackWorker(&this->bufferList, this->player);
  this->workerThread = new QThread();

  this->worker->moveToThread(this->workerThread);

  connect(
        this->worker,
        SIGNAL(error()),
        this,
        SLOT(onError()));

  connect(
        this->worker,
        SIGNAL(starving()),
        this,
        SLOT(onStarving()));

  // On restart, play
  connect(
        this,
        SIGNAL(restart()),
        this->worker,
        SLOT(play()));

  // On halt, stop
  connect(
        this,
        SIGNAL(halt()),
        this->worker,
        SLOT(halt()));

  // Worker finished, call thread quit
  connect(
        this->worker,
        SIGNAL(finished()),
        this->workerThread,
        SLOT(quit()));

  // When both threads and worker finish, delete them later
  connect(
        this->worker,
        SIGNAL(finished()),
        this->worker,
        SLOT(deleteLater()));

  connect(
        this->workerThread,
        SIGNAL(finished()),
        this->workerThread,
        SLOT(deleteLater()));

  this->workerThread->start();
}

void
AudioPlayback::onError(void)
{
  this->failed = true;
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
  }

  if (this->worker != nullptr)
    delete this->worker;

  if (this->player != nullptr)
    delete this->player;
}

unsigned int
AudioPlayback::getSampleRate(void) const
{
  return this->sampRate;
}

float
AudioPlayback::getVolume(void) const
{
  return this->volume;
}

void
AudioPlayback::setVolume(float vol)
{
  SUFLOAT gain;

  if (vol < -60)
    gain = 0;
  else
    gain = SU_MAG_RAW(vol);

  this->volume = vol;
  this->worker->setGain(gain);
}

void
AudioPlayback::write(const SUCOMPLEX *samples, SUSCOUNT size)
{
  while (size > 0 && !failed) {
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

    if (chunk > SIGDIGGER_AUDIO_BUFFER_SIZE - this->ptr)
      chunk = SIGDIGGER_AUDIO_BUFFER_SIZE - this->ptr;
    remaining = chunk;

    while (remaining-- > 0)
      *start++ = SU_C_REAL(*samples++);

    this->ptr += chunk;
    size -= chunk;

    // Buffer full, send to playback thread.
    if (this->ptr == SIGDIGGER_AUDIO_BUFFER_SIZE) {
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
