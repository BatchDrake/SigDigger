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
#include <sys/mman.h>

using namespace SigDigger;

#define ATTEMPT(expr, what) \
  if ((err = expr) < 0)  \
    throw std::runtime_error("Failed to " + std::string(what) + ": " + std::string(snd_strerror(err)))


///////////////////////////// Playback worker /////////////////////////////////
PlaybackWorker::PlaybackWorker(AudioBufferList *instance)
{
  PaStream *stream;
  PaError pErr = Pa_Initialize();
  
   if( pErr != paNoError )
  {
      std::cerr << "Error number: " << pErr << std::endl ;
      std::cerr << "Error message: " << Pa_GetErrorText( pErr ) << std::endl ;
  }
  PaStreamParameters outputParameters;

  outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
  outputParameters.channelCount = 1;
  outputParameters.sampleFormat = paFloat32;
  outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
  outputParameters.hostApiSpecificStreamInfo = NULL;

  pErr = Pa_OpenStream(
     &stream,
     NULL,
     &outputParameters,
     SIGDIGGER_AUDIO_SAMPLE_RATE,
     SIGDIGGER_AUDIO_BUFFER_SIZE,
     paClipOff,
     NULL,
     NULL );

  if( pErr != paNoError )
  {
      std::cerr << "Pa_OpenStream Error number: " << pErr << std::endl ;
      std::cerr << "Error message: " << Pa_GetErrorText( pErr ) << std::endl ;
  }

  pErr = Pa_StartStream( stream );

  if( pErr != paNoError )
  {
      std::cerr << "Pa_StartStream Error number: " << pErr << std::endl ;
      std::cerr << "Error message: " << Pa_GetErrorText( pErr ) << std::endl ;
  }

  this->pcm      = stream;
  this->instance = instance;
}

void
PlaybackWorker::play(void)
{
  float *buffer;
  PaError err;  

  while (!this->halting && (buffer = this->instance->next()) != nullptr) {
    
    err = Pa_WriteStream(this->pcm, buffer, SIGDIGGER_AUDIO_BUFFER_SIZE );

    if(err == paOutputUnderflowed)
        printf("warning: paOutputUnderflowed\n");
    else if(err != paNoError) {
        printf("Pa_WriteStream error %i (%s)\n", err, Pa_GetErrorText(err));
        for(int i = 0; i < 10; ++i) {
            usleep(10 * 1000);
        }
    }

    // Done with this buffer, mark as free.
    this->instance->release();

   // if (err < 0)
     // emit error();
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
  if (this->pcm != nullptr) {
    std::cerr << "Inside halt !!"  << std::endl;
    Pa_StopStream( pcm );
    Pa_CloseStream( pcm );
    Pa_Terminate();
  }
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

  
  this->sampRate = rate;
  this->startWorker();
}

void
AudioPlayback::startWorker()
{
  this->worker = new PlaybackWorker(&this->bufferList);
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
    std::cerr << "AudioPlayback: reached watermark, buffering again..." << std::endl;
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

