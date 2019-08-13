//
//    AudioPlayback.cpp: Put samples in the soundcard
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

#include <iostream>
#include "AudioPlayback.h"
#include <stdexcept>
#include <alsa/asoundlib.h>
#include <sys/mman.h>

#define ATTEMPT(expr, what) \
  if ((err = expr) < 0)  \
    throw std::runtime_error("Failed to " + std::string(what) + ": " + std::string(snd_strerror(err)))


using namespace SigDigger;

///////////////////////////// Playback worker /////////////////////////////////
PlaybackWorker::PlaybackWorker(AudioBufferList *instance, snd_pcm_t *pcm)
{
  this->pcm      = pcm;
  this->instance = instance;
}

void
PlaybackWorker::play(void)
{
  float *buffer;

  while (!this->halting && (buffer = this->instance->next()) != nullptr) {
    long err;
    err = snd_pcm_writei(this->pcm, buffer, SIGDIGGER_AUDIO_BUFFER_SIZE);

    if (err == -EPIPE) {
      snd_pcm_prepare(this->pcm);
      err = snd_pcm_writei(this->pcm, buffer, SIGDIGGER_AUDIO_BUFFER_SIZE);
    }

    // Done with this buffer, mark as free.
    this->instance->release();

    if (err < 0)
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
         0,
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
  if (data != nullptr)
    munmap(data, SIGDIGGER_AUDIO_BUFFER_ALLOC);
}

////////////////////////////// AudioBufferList /////////////////////////////////
AudioBufferList::AudioBufferList(unsigned int num)
{
  unsigned int i;

  for (i = 0; i < num; ++i) {
    AudioBuffer *buffer = new AudioBuffer();

    buffer->next = this->freeList;
    this->freeList = buffer;
  }

  this->freeLen = this->totalLen = num;
}

AudioBufferList::~AudioBufferList()
{
  AudioBuffer *buffer, *next;

  buffer = this->freeList;
  while (buffer != nullptr) {
    next = buffer->next;
    delete buffer;
    buffer = next;
  }

  buffer = this->playListHead;
  while (buffer != nullptr) {
    next = buffer->next;
    delete buffer;
    buffer = next;
  }

  if (this->current != nullptr)
    delete this->current;

  if (this->playBuffer != nullptr)
    delete this->playBuffer;
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
      this->playListTail->prev = buffer;

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
  int err;
  snd_pcm_hw_params_t *params = nullptr;
  float *buf;

  // Ladies and gentlemen, behold the COBOL
  if ((buf = static_cast<float *>(mmap(
         nullptr,
         SIGDIGGER_AUDIO_BUFFER_ALLOC,
         PROT_READ | PROT_WRITE,
         MAP_PRIVATE | MAP_ANONYMOUS,
         0,
         0))) == reinterpret_cast<float *>(-1)) {
    throw std::runtime_error(
          "Failed to allocate "
          + std::to_string(SIGDIGGER_AUDIO_BUFFER_ALLOC)
          + " of buffer bytes for soundcard.");
  }

  ATTEMPT(
        snd_pcm_open(&this->pcm, dev.c_str(), SND_PCM_STREAM_PLAYBACK, 0),
        "open audio device " + dev);

  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(pcm, params);

  ATTEMPT(
        snd_pcm_hw_params_set_access(
          this->pcm,
          params,
          SND_PCM_ACCESS_RW_INTERLEAVED),
        "set interleaved access for audio device");

  ATTEMPT(
        snd_pcm_hw_params_set_format(
          this->pcm,
          params,
          SND_PCM_FORMAT_FLOAT_LE),
        "set sample format");

  ATTEMPT(
        snd_pcm_hw_params_set_buffer_size(
          this->pcm,
          params,
          SIGDIGGER_AUDIO_BUFFER_SIZE),
        "set buffer size");

  ATTEMPT(
        snd_pcm_hw_params_set_channels(this->pcm, params, 1),
        "set output to mono");

  ATTEMPT(
        snd_pcm_hw_params_set_rate_near(this->pcm, params, &rate, nullptr),
        "set sample rate");

  ATTEMPT(snd_pcm_hw_params(this->pcm, params), "set device params");

  this->sampRate = rate;

  this->startWorker();
}

void
AudioPlayback::startWorker()
{
  this->worker = new PlaybackWorker(&this->bufferList, this->pcm);
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

  if (this->pcm != nullptr) {
    snd_pcm_drain(this->pcm);
    snd_pcm_close(this->pcm);
  }
}

unsigned int
AudioPlayback::getSampleRate(void) const
{
  return this->sampRate;
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
