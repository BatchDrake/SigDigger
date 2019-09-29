//
//    FACFrameSync.cpp: FAC frame synchronizer
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


#include "FACFrameSync.h"
#include "FACFrameSyncUI.h"

using namespace SigDigger;

////////////////////////////////// Config //////////////////////////////////////
void
FACFrameSyncConfig::deserialize(Suscan::Object const &)
{
}

Suscan::Object &&
FACFrameSyncConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  return this->persist(obj);
}

////////////////////////// FACFrameSync implementation /////////////////////////////
FACFrameSync::FACFrameSync(Suscan::DecoderFactory *manufacturer) :
  QObject(nullptr),
  Decoder(manufacturer)
{
  this->cracker = new FACSyncCracker(this);
}

void
FACFrameSync::delayedConstructor(void)
{
  this->ui = static_cast<FACFrameSyncUI *>(this->getDecoderObjects()->ui);

  this->onUiChanged();

  connect(
        this->ui,
        SIGNAL(controlsChanged(void)),
        this,
        SLOT(onUiChanged(void)));

  connect(
        this->ui,
        SIGNAL(toggleEnable(bool)),
        this,
        SLOT(onToggleEnable(bool)));

  connect(
        this->ui,
        SIGNAL(toggleGuess(bool)),
        this,
        SLOT(onToggleGuess(bool)));

  connect(
        this->ui,
        SIGNAL(applyConfig(void)),
        this,
        SLOT(onApplyConfig(void)));

  connect(
        this->cracker,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onCrackerStateChanged(int)));

  connect(
        this->cracker,
        SIGNAL(progress(qreal)),
        this,
        SLOT(onCrackerProgress(qreal)));
}

FACFrameSync::~FACFrameSync()
{
  delete cracker;
}

Suscan::Serializable const &
FACFrameSync::getConfig(void) const
{
  return this->config;
}

bool
FACFrameSync::setConfig(Suscan::Serializable &config)
{
  this->config.deserialize(config.serialize());

  return true;
}

std::string
FACFrameSync::getStateString(void) const
{
  return "Dumping current frame";
}

bool
FACFrameSync::setInputBps(uint8_t bps)
{
  this->bps = bps;
  this->mask = static_cast<Symbol>((1 << bps) - 1);

  return true; // Accepts any bps
}

uint8_t
FACFrameSync::getOutputBps(void) const
{
  return this->bps;
}

bool
FACFrameSync::work(FrameId, const Symbol *buffer, size_t len)
{
  if (this->guessing)
    this->cracker->feed(buffer, len);

  if (this->enabled) {
    while (len > 0) {
      size_t avail = static_cast<size_t>(this->frameLen - this->pShifted);
      size_t chunk = len;

      if (chunk > avail)
        chunk = avail;

      if (!this->write(buffer, chunk))
        return false;

      this->pShifted += chunk;
      this->p += chunk;

      if (this->pShifted >= this->frameLen) {
        this->pShifted = 0;
        this->nextFrame();
      }

      if (this->p >= this->frameLen)
        this->p -= this->frameLen;

      len -= chunk;
      buffer += chunk;
    }
  } else {
    return this->write(buffer, len);
  }

  return true;
}


void
FACFrameSync::onUiChanged(void)
{
  if (this->frameLen != this->ui->getLength()) {
    this->frameLen = this->ui->getLength();
    this->p %= this->frameLen;
  }

  this->pShifted = (this->p + this->ui->getShift()) % this->frameLen;

  this->cracker->setBufferSize(static_cast<size_t>(this->ui->getBufferLength()));
}

void
FACFrameSync::onToggleEnable(bool enabled)
{
  this->enabled = enabled;
}

void
FACFrameSync::onToggleGuess(bool guessing)
{
  this->cracker->restart();
  this->ui->setProgress(0);
  this->guessing = guessing;

  if (guessing)
    this->ui->setStateString("Acquiring");
  else
    this->ui->setStateString("Stand by");

  this->ui->setCandidateString("N/A");
  this->ui->setApplyEnabled(false);
  this->ui->setSequence(std::vector<Symbol>());
}

void
FACFrameSync::onApplyConfig(void)
{
  if (this->cracker->get() != -1)
    this->ui->setLength(this->cracker->get());
}

void
FACFrameSync::onCrackerStateChanged(int state)
{
  FACSyncCracker::State asEnum = static_cast<FACSyncCracker::State>(state);

  switch (asEnum) {
    case FACSyncCracker::ACQUIRING:
      this->ui->setStateString("Acquiring");
      break;

    case FACSyncCracker::RUNNING:
      this->ui->setStateString("Computing fast auto-correlation");
      break;

    case FACSyncCracker::FOUND:
      this->ui->setGuessingEnabled(false);
      this->onToggleGuess(false);
      this->ui->setStateString("Candidate found!");
      this->ui->setCandidateString(
            QString::number(this->cracker->get()) + " symbols");
      this->ui->setSequence(this->cracker->syncSymbols());
      break;
  }

  this->ui->setApplyEnabled(asEnum == FACSyncCracker::FOUND);
}

void
FACFrameSync::onCrackerProgress(qreal progress)
{
  this->ui->setProgress(progress);
}
