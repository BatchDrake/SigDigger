#include "HexTap.h"
#include "HexTapUI.h"

using namespace SigDigger;

////////////////////////////////// Config //////////////////////////////////////
void
HexTapConfig::deserialize(Suscan::Object const &)
{
}

Suscan::Object &&
HexTapConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  return this->persist(obj);
}

////////////////////////// HexTap implementation /////////////////////////////
HexTap::HexTap(Suscan::DecoderFactory *manufacturer) :
  QObject(nullptr),
  Decoder(manufacturer)
{
}

void
HexTap::delayedConstructor(void)
{
  this->ui = static_cast<HexTapUI *>(this->getDecoderObjects()->ui);

  connect(
        this->ui,
        SIGNAL(controlsChanged(void)),
        this,
        SLOT(onUiChanged(void)));

  connect(
        this->ui,
        SIGNAL(clear(void)),
        this,
        SLOT(onClear(void)));


  this->shift = this->ui->shift();
  this->lsb = this->ui->lsb();
  this->reverse = this->ui->reverse();
  this->pack = this->ui->pack();
}

HexTap::~HexTap()
{

}

Suscan::Serializable const &
HexTap::getConfig(void) const
{
  return this->config;
}

bool
HexTap::setConfig(Suscan::Serializable &config)
{
  this->config.deserialize(config.serialize());

  return true;
}

std::string
HexTap::getStateString(void) const
{
  return "Dumping current frame";
}

bool
HexTap::setInputBps(uint8_t bps)
{
  this->bps = bps;
  this->mask = static_cast<Symbol>((1 << bps) - 1);

  this->lsb2msb.resize(1 << bps);

  // Recalculate lsb2msb dictionary
  for (unsigned i = 0; i < this->lsb2msb.size(); ++i) {
    Symbol s = 0;
    for (auto j = 0; j < bps; ++j)
      if (i & (1 << j))
        s |= 1 << (bps - j - 1);
    this->lsb2msb[i] = s;
  }

  return true; // Accepts any bps
}

uint8_t
HexTap::getOutputBps(void) const
{
  return this->bps;
}

void
HexTap::feedSelected(const Symbol *syms, size_t size)
{
  if (this->pack) {
    Symbol s;
    for (size_t i = 0; i < size; ++i) {
      s = this->reverse ? syms[i] : lsb2msb[syms[i]];

      for (unsigned j = 0; j < this->bps; ++j) {
        if (this->ptr >= 0 && s & (1 << j))
          this->currByte |= this->lsb
              ? 1 << this->ptr
              : 1 << (7 - this->ptr);

        if (++this->ptr == 8) {
          this->selectedFrameBytes.push_back(this->currByte);
          this->currByte = 0;
          this->ptr = 0;
        }
      }
    }
  } else {
    this->selectedFrameBytes.insert(
          this->selectedFrameBytes.end(),
          syms,
          syms + size);
  }
}

void
HexTap::recalcSelectedFrameBytes(void)
{
  this->currByte = 0;
  this->ptr = this->shift;
  this->selectedFrameBytes.clear();


  if (this->selectedFrameId < this->frames.size())
    this->feedSelected(
        this->frames[this->selectedFrameId].data(),
        this->frames[this->selectedFrameId].size());
}

void
HexTap::selectFrame(FrameId id)
{
  if (this->selectedFrameId != id) {
    this->selectedFrameId = id;
    this->recalcSelectedFrameBytes();
    this->ui->setFrameBytes(&this->selectedFrameBytes);
  }
}

FrameBytes *
HexTap::getFrame(FrameId id)
{
  this->selectFrame(id);
  return &this->selectedFrameBytes;
}

bool
HexTap::work(FrameId frameId, const Symbol *buffer, size_t len)
{
  size_t bytesLen;

  if (this->empty) {
    // Baseline state
    this->currFrameId = frameId;
    this->selectedFrameId = 0;
    this->relFrameId = 0;
    this->empty = false;

    // Set frame size to 1
    this->frames.resize(1);
    this->current = &this->frames[0];
    this->ui->setFrameCount(1);
    this->ui->setFrameBytes(this->getFrame(0));
  }

  if (this->currFrameId != frameId) {
    this->currFrameId = frameId;
    ++this->relFrameId;
    this->nextFrame();
    this->frames.resize(this->frames.size() + 1);
    this->current = &this->frames[this->frames.size() - 1];

    this->ui->setFrameCount(static_cast<int>(this->frames.size()));
  }

  if (this->ui->autoScroll())
    this->selectFrame(this->relFrameId);

  this->current->insert(this->current->end(), buffer, buffer + len);

  if (this->selectedFrameId == this->relFrameId) {
    bytesLen = this->selectedFrameBytes.size();
    this->feedSelected(buffer, len);
    this->ui->refreshFrom(bytesLen);
  }

  this->write(buffer, len);

  return true;
}

void
HexTap::onClear(void)
{
  this->frames.resize(0);
  this->frames.resize(1);

  this->ui->setFrameCount(1);
  this->selectedFrameBytes.clear();
  this->ui->setFrameBytes(this->getFrame(0));
  this->empty = true;
}

void
HexTap::onUiChanged(void)
{
  this->shift = this->ui->shift();
  this->lsb = this->ui->lsb();
  this->reverse = this->ui->reverse();
  this->pack = this->ui->pack();

  this->selectFrame(this->ui->frameId());
  this->recalcSelectedFrameBytes();
  this->ui->setFrameBytes(this->getFrame(this->selectedFrameId));
}
