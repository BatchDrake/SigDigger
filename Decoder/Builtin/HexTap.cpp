#include "HexTap.h"

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
HexTap::HexTap(Suscan::DecoderFactory *manufacturer) : Decoder(manufacturer)
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
  return true; // Accepts any bps
}

uint8_t
HexTap::getOutputBps(void) const
{
  return this->bps;
}

bool
HexTap::work(FrameId, const Symbol *buffer, size_t len)
{
  // TODO: Check for FrameId

  this->write(buffer, len);

  return true;
}
