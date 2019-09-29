//
//    Misc/FACSyncCracker.cpp: Fast Autocorrelation cracker
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

#include <cmath>
#include <FACSyncCracker.h>

#include <QThread>

using namespace SigDigger;

////////////////////////// FAC Sync worker thread /////////////////////////////
FACSyncWorker::FACSyncWorker(const std::vector<Symbol> &symbols)
{
  this->fac.resize(symbols.size());

  for (size_t i = 0; i < symbols.size(); ++i)
    this->fac[i] = symbols[i];
}

void
FACSyncWorker::run(void)
{
  // It is okay if you find this difficult to read. C++ made the interfacing
  // with C libraries extremely cumbersome and ugly.

  Q_ASSERT(this->buffer == nullptr);
  Q_ASSERT(this->direct == nullptr);
  Q_ASSERT(this->reverse == nullptr);

  SU_ATTEMPT(
        this->buffer = static_cast<SU_FFTW(_complex) *>(
        SU_FFTW(_malloc)(
          this->fac.size() * sizeof(SU_FFTW(_complex)))));

  SU_ATTEMPT(
        this->direct = SU_FFTW(_plan_dft_1d)(
          static_cast<int>(this->fac.size()),
          this->buffer,
          this->buffer,
          FFTW_FORWARD,
          FFTW_ESTIMATE));

  SU_ATTEMPT(
        this->reverse = SU_FFTW(_plan_dft_1d)(
          static_cast<int>(this->fac.size()),
          this->buffer,
          this->buffer,
          FFTW_BACKWARD,
          FFTW_ESTIMATE));

  // Copy everything to the FFTW buffer
  for (size_t i = 0; i < this->fac.size(); ++i) {
    this->buffer[i][0] = this->fac[i];
    this->buffer[i][1] = 1;
  }

  // Notify cracker
  emit started();

  // Execute plan
  SU_FFTW(_execute)(this->direct);

  // Do the trick. Turn the current spectrum into the spectrum of the
  // autocorrelation of the buffer. Note the spectrum here will be
  // symmetric
  for (size_t i = 0; i < this->fac.size(); ++i) {
    this->buffer[i][0] = this->buffer[i][0] * this->buffer[i][0] +
        this->buffer[i][1] * this->buffer[i][1];
    this->buffer[i][1] = 0;
  }

  // Reverse. We are undoing the FFT of something that is symmetric and
  // real. The resulting signal must also be symmetric and real.
  SU_FFTW(_execute)(this->reverse);

  // Copy magnitude. It is okay with simply using abs() of floats.
  for (size_t i = 0; i < this->fac.size(); ++i)
    this->fac[i] = SU_ABS(this->buffer[i][0]);

  // Done. Notify cracker
  emit done();
}

std::vector<SUFLOAT> const &
FACSyncWorker::get(void) const
{
  return this->fac;
}

FACSyncWorker::~FACSyncWorker(void)
{
  if (this->direct != nullptr)
    SU_FFTW(_destroy_plan)(this->direct);

  if (this->reverse != nullptr)
    SU_FFTW(_destroy_plan)(this->reverse);

  if (this->buffer != nullptr)
    SU_FFTW(_free)(this->buffer);
}

///////////////////////////// FAC Sync cracker ////////////////////////////////
FACSyncCracker::FACSyncCracker(QObject *parent, size_t len) : QObject(parent)
{
  this->buffer.resize(len);
}

void
FACSyncCracker::setBufferSize(size_t len)
{
  if (this->p > len)
    this->p = static_cast<unsigned>(len);

  this->buffer.resize(len);
}

void
FACSyncCracker::feed(const Symbol *buffer, size_t len)
{
  if (this->state == ACQUIRING) {
    size_t avail = this->buffer.size() - this->p;
    if (len > avail)
      len = avail;

    for (size_t i = 0; i < len; ++i)
      this->buffer[this->p++] = buffer[i];

    emit progress(static_cast<qreal>(this->p) / this->buffer.size());

    if (this->p == this->buffer.size()) {
      // Buffer is full. Reset pointer and create FAC Sync worker

      this->p = 0;
      this->worker = new FACSyncWorker(this->buffer);
      this->workerThread = new QThread();
      this->worker->moveToThread(this->workerThread);

      // Now signals will be queued. Connect them.
      connect(
            this->worker,
            SIGNAL(started(void)),
            this,
            SLOT(onStarted(void)));

      connect(
            this->worker,
            SIGNAL(done(void)),
            this,
            SLOT(onDone(void)));

      connect(
            this,
            SIGNAL(full(void)),
            this->worker,
            SLOT(run(void)));

      // On done, automatically quit thread
      connect(
            this->worker,
            SIGNAL(done(void)),
            this->workerThread,
            SLOT(quit(void)));

      // On worker thread finished, delete later
      connect(
            this->workerThread,
            SIGNAL(finished(void)),
            this->workerThread,
            SLOT(deleteLater(void)));

      this->state = RUNNING;
      emit stateChanged(this->state);

      this->workerThread->start();

      emit full();
    }
  }
}

void
FACSyncCracker::setBps(uint8_t bps)
{
  this->bps = bps;
  this->maxSym = static_cast<Symbol>((1 << bps) - 1);
}

void
FACSyncCracker::restart(void)
{
  this->p = 0;
  this->state = ACQUIRING;
}

int
FACSyncCracker::get(void) const
{
  return this->delay;
}

std::vector<Symbol>
FACSyncCracker::syncSymbols(void) const
{
  return this->syncSeq;
}

float
FACSyncCracker::significance(void) const
{
  return this->relAmp;
}

void
FACSyncCracker::findSync(void)
{
  std::vector<SUFLOAT> mode;
  std::vector<SUFLOAT> sigma2;
  std::vector<unsigned int> histogram;
  SUFLOAT sqSigmas = this->sigmas * this->sigmas;
  SUFLOAT sqSigmaMax =  0;
  SUFLOAT sqSigmaThreshold;
  SUFLOAT K;
  int packets;
  unsigned ui, uf;
  int first = 0;
  int candidateStart = 0;
  int candidateLength = 0;

  Q_ASSERT(this->delay > 0);

  mode.resize(static_cast<size_t>(this->delay));
  sigma2.resize(static_cast<size_t>(this->delay));
  histogram.resize(this->maxSym + 1);

  std::fill(sigma2.begin(), sigma2.end(), 0);

  packets = static_cast<int>(this->buffer.size()) / this->delay;

  K = 1.f / packets;

  // Christ I swear I hate this language.
  for (int i = 0; i < this->delay; ++i) {
    unsigned int maxCount = 0;
    Symbol sym = 0;

    ui = static_cast<unsigned>(i);
    std::fill(histogram.begin(), histogram.end(), 0);

    for (int j = 0; j < packets; ++j) {
      uf = static_cast<unsigned>(i + j * this->delay);
      ++histogram[this->buffer[uf] & this->maxSym];
    }

    // Traverse histogram and find max
    for (int j = 0; j <= this->maxSym; ++j) {
      uf = static_cast<unsigned int>(j);
      if (histogram[uf] > maxCount) {
        maxCount = histogram[uf];
        sym = static_cast<Symbol>(j);
      }
    }

    // Update mode
    mode[ui] = sym;
  }

  // This is a special variance. It is not calculated based on a deviation
  // from the mean, but from the mode.
  for (int j = 0; j < packets; ++j)
    for (int i = 0; i < this->delay; ++i) {
      ui = static_cast<unsigned>(i);
      uf = static_cast<unsigned>(i + j * this->delay);

      sigma2[ui] += K * (this->buffer[uf] - mode[ui]) * (this->buffer[uf] - mode[ui]);
    }

  // Alright, now we have the average symbol plus its standard deviation.
  // Now we can make guesses on the most likely sequence.

  // For purely random data, we expect mu to be maxSym / 2. We can easily
  // estimate a maximum value for the variance
  for (int i = 0; i <= this->maxSym; ++i)
    sqSigmaMax += (i - .5f * this->maxSym) * (i - .5f * this->maxSym);
  sqSigmaMax /= this->maxSym + 1;

  // Now we can use this value as a threshold. If the variance is
  // below certain value (let's say 25 times below the maximum),
  // we assume this is a frame sync.
  sqSigmaThreshold = sqSigmaMax / sqSigmas;

  // We start looking for sequences now
  for (int i = 0; i < this->delay; ++i) {
    ui = static_cast<unsigned>(i);

    if (sigma2[ui] > sqSigmaThreshold) {
      // Too high. Break sequence
      if (i - first > candidateLength) {
        // However, this is a good candidate
        candidateLength = i - first;
        candidateStart = first;
      }

      // Next candidate may start right after this one
      first = i + 1;
    }
  }

  // Maybe this sequence is at the end
  if (this->delay - first > candidateLength) {
    // However, this is a good candidate
    candidateLength = this->delay - first;
    candidateStart = first;
  }

  // Copy sequence
  this->syncSeqStart = candidateStart;
  this->syncSeq.resize(static_cast<unsigned>(candidateLength));
  for (int i = 0; i < candidateLength; ++i)
    this->syncSeq[static_cast<unsigned>(i)] =
        static_cast<Symbol>(
          round(mode[static_cast<unsigned>(i + candidateStart)]));
}

void
FACSyncCracker::guess(void)
{
  // First step: compute standard deviation
  SUFLOAT avg = 0;
  SUFLOAT var = 0;
  SUFLOAT sqSigmas = this->sigmas * this->sigmas;
  size_t maxNdx = 0;
  SUFLOAT max = 0;

  for (size_t i = 0; i < this->fac.size(); ++i)
    avg += this->fac[i];
  avg /= this->fac.size();

  for (size_t i = 0; i < this->fac.size(); ++i)
    var += (this->fac[i] - avg) * (this->fac[i] - avg);
  var /= this->fac.size();

  // Second step: look for the biggest coefficient (ignoring the first)
  for (size_t i = 1; i < this->fac.size() / 2; ++i) {
    if (this->fac[i] > max) {
      max = this->fac[i];
      maxNdx = i;
    }
  }

  // Third step: is this maximum significant?
  if (max * max >= var * sqSigmas) {
    this->delay = static_cast<int>(maxNdx);
    this->relAmp = max / SU_SQRT(var);
    this->findSync();
  } else {
    this->delay = -1;
  }
}

void
FACSyncCracker::onDone(void)
{
  // Worker done and signal automatically called deleteLater()
  this->workerThread = nullptr;

  // Regarding the worker itself, we retrieve the FAC buffer and compute the
  // most likely delay
  this->fac = this->worker->get();

  // Delete worker later.
  this->worker->deleteLater();
  this->worker = nullptr;

  // Guess!
  this->guess();

  // Emit new state
  this->p = 0;
  if (this->get() != -1) {
    this->state = FOUND;
  } else {
    this->state = ACQUIRING;
  }

  emit stateChanged(this->state);
}

void
FACSyncCracker::onStarted(void)
{

}

FACSyncCracker::~FACSyncCracker(void)
{
  if (this->workerThread != nullptr) {
    this->workerThread->quit();
    this->workerThread->wait();
    this->workerThread->deleteLater();
  }

  if (this->worker != nullptr)
    this->worker->deleteLater();
}
