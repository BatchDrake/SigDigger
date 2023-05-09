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

#ifndef AUDIOPLAYBACK_H
#define AUDIOPLAYBACK_H

#include <QObject>
#include <QMutex>
#include <QThread>
#include <string>
#include <Suscan/Library.h>
#include <GenericAudioPlayer.h>
#include <sigutils/util/compat-unistd.h>

#define SIGDIGGER_AUDIO_BUFFER_ALLOC static_cast<size_t>(1 << 14)
#define SIGDIGGER_AUDIO_BUFFER_SIZE (SIGDIGGER_AUDIO_BUFFER_ALLOC / sizeof (float))
#define SIGDIGGER_AUDIO_SAMPLE_RATE         44100
#define SIGDIGGER_AUDIO_BUFFER_NUM          20
#define SIGDIGGER_AUDIO_BUFFER_MIN          10
#define SIGDIGGER_AUDIO_BUFFERING_WATERMARK 2

#define SIGDIGGER_AUDIO_BUFFER_SIZE_MIN     256
#define SIGDIGGER_AUDIO_BUFFER_DELAY_MS     20

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#  define QRecursiveMutex QMutex
#endif

namespace SigDigger {
  class AudioBufferList;

  class PlaybackWorker : public QObject {
      Q_OBJECT

      GenericAudioPlayer *player = nullptr;  // Owned
      AudioBufferList *instance; // Weak
      float gain = 1;
      unsigned int bufferSize;
      std::string device;
      unsigned int sampRate;

    public:
      static unsigned int calcBufferSizeForRate(unsigned int rate);

      PlaybackWorker(
          AudioBufferList *instance = nullptr,
          std::string const &dev = "default",
          unsigned int sampRate = SIGDIGGER_AUDIO_SAMPLE_RATE);
      unsigned int getBufferSize(void) const;

    public slots:
      void startPlayback();
      void stopPlayback();
      void setSampleRate(unsigned int rate);
      void setGain(float);
      void play(void);
      void halt(void);

    signals:
      void ready(void);
      void error(QString);
      void starving(void);
      void finished(void);
  };

  // TODO: turn into std::list
  struct AudioBuffer {
    AudioBuffer *next = nullptr;
    AudioBuffer *prev = nullptr;
    float *data = nullptr;

    AudioBuffer();
    ~AudioBuffer();
  };

  class AudioBufferList {
    std::vector<AudioBuffer> allocation;

    // Free list head. All buffers are initially here
    AudioBuffer *freeList = nullptr;
    unsigned int freeLen = 0;
    unsigned int totalLen = 0;

    // Current buffer. The one being filled up
    AudioBuffer *current = nullptr;

    // Playlist head. These buffers are the ones being played by
    // the playback worker
    AudioBuffer *playListHead = nullptr;
    AudioBuffer *playListTail = nullptr;
    unsigned int playListLen = 0;

    // Buffer being played.
    AudioBuffer *playBuffer = nullptr;

    // Mutex to ensure ordered access
    QRecursiveMutex listMutex;

  public:
    AudioBufferList(unsigned int num);

    unsigned int
    getFreeLen(void) const
    {
      return this->freeLen;
    }

    unsigned int
    getPlayListLen(void) const
    {
      return this->playListLen;
    }

    void clear(void);

    void reset(void);

    // Takes one from the freelist, replaces current, returns pointer
    float *reserve(void);

    // Takes current and puts it in the playList
    void commit(void);

    // Takes one from the playList
    float *next(void);

    // Moves current playing buffer to the freeList
    void release(void);
  };

  class AudioPlayback : public QObject {
    Q_OBJECT

    // Audio buffer list
    AudioBufferList bufferList;
    QThread *workerThread  = nullptr;
    PlaybackWorker *worker = nullptr;

    bool buffering = true;
    bool running = false;
    bool ready = false;
    float *current_buffer = nullptr;
    float volume = 1;

    std::string  device;
    unsigned int completed = 0;
    unsigned int ptr = 0;
    unsigned int sampRate;
    unsigned int bufferSize;

    void startWorker(void);

    public:
      static bool enumerateDevices(std::vector<GenericAudioDevice> &);
      static std::string getDefaultDevice();
      static const char *audioLibrary();

      AudioPlayback(
          std::string const &,
          unsigned int rate = SIGDIGGER_AUDIO_SAMPLE_RATE);
      virtual ~AudioPlayback();
      unsigned int getSampleRate(void) const;
      void setSampleRate(unsigned int);
      void write(const SUCOMPLEX *samples, SUSCOUNT size);
      void start(void);
      void stop(void);
      float getVolume(void) const;
      void setVolume(float);
      void cancelPlayBack(void);

      inline bool
      isRunning(void) const
      {
        return this->running;
      }

    public slots:
      void onError(QString);
      void onStarving(void);
      void onReady(void);

    signals:
      void restart(void);
      void halt(void);
      void error(QString);
      void sampleRate(unsigned int);
      void gain(float);
      void startPlayback(void);
      void stopPlayback(void);
  };
}

#endif // AUDIOPLAYBACK_H
