//
//    FrameSync.cpp: Sequence-based frame synchronizer
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

#include "FrameSync.h"
#include "FrameSyncUI.h"

using namespace SigDigger;

////////////////////////////////// Config //////////////////////////////////////
void
FrameSyncConfig::deserialize(Suscan::Object const &)
{
}

Suscan::Object &&
FrameSyncConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  return this->persist(obj);
}

////////////////////////// FrameSync implementation /////////////////////////////
FrameSync::FrameSync(Suscan::DecoderFactory *manufacturer) :
  QObject(nullptr),
  Decoder(manufacturer)
{
}

void
FrameSync::delayedConstructor(void)
{
  this->ui = static_cast<FrameSyncUI *>(this->getDecoderObjects()->ui);

  this->onUiChanged();

  connect(
        this->ui,
        SIGNAL(controlsChanged(void)),
        this,
        SLOT(onUiChanged(void)));
}

FrameSync::~FrameSync()
{

}

Suscan::Serializable const &
FrameSync::getConfig(void) const
{
  return this->config;
}

bool
FrameSync::setConfig(Suscan::Serializable &config)
{
  this->config.deserialize(config.serialize());

  return true;
}

std::string
FrameSync::getStateString(void) const
{
  return "Dumping current frame";
}

bool
FrameSync::setInputBps(uint8_t bps)
{
  this->bps = bps;
  this->mask = static_cast<Symbol>((1 << bps) - 1);

  this->ui->setBps(bps);

  return true; // Accepts any bps
}

uint8_t
FrameSync::getOutputBps(void) const
{
  return this->bps;
}

bool
FrameSync::work(FrameId, const Symbol *buffer, size_t len)
{
  if (this->mode == REPETITION) {
    size_t prevNdx = 0;
    if (this->repetitions > 0) {
      for (size_t i = 0; i < len; ++i) {
        if (buffer[i] == this->last) {
          if (++this->count >= this->repetitions) {
            this->write(&buffer[prevNdx], i + 1 - prevNdx);
            this->nextFrame();
            prevNdx = i + 1;
            this->count = 0;
          }
        } else {
          this->last = buffer[i];
          this->count = 1;
        }
      }

      if (prevNdx < len)
        this->write(&buffer[prevNdx], len - prevNdx);
    } else {
      this->write(buffer, len);
    }
  } else if (this->mode == SEQUENCE) {
    if (this->chopper != nullptr) {
      Symbol s;
      while (len-- > 0) {
        if (chopper_feed(chopper, *buffer++)) {
          this->nextFrame();
        }

        if (chopper_is_ready(chopper)) {
          s = chopper_read(chopper);
          this->write(&s, 1);
        }
      }
    } else {
      this->write(buffer, len);
    }
  }

  return true;
}

void
FrameSync::updateChopper(void)
{
  struct chopper_params params;

  if (this->chopper != nullptr) {
    chopper_destroy(this->chopper);
    this->chopper = nullptr;
  }

  if (this->sequence.size() > 0) {
    params.seq_data = this->sequence.data();
    params.seq_len  = static_cast<unsigned>(this->sequence.size());

    this->chopper = chopper_new(&params);

    if (this->chopper == nullptr)
      throw std::bad_alloc();
  }
}

void
FrameSync::onUiChanged(void)
{
  this->sequence = this->ui->getSequence();
  this->repetitions = static_cast<unsigned>(this->ui->getTimes());

  this->mode = this->ui->isRepetition() ? REPETITION : SEQUENCE;

  if (this->mode == SEQUENCE)
    this->updateChopper();
}
