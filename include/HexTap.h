#ifndef HEXTAP_H
#define HEXTAP_H

#include <Decoder.h>

namespace SigDigger {
  class HexTapConfig : public Suscan::Serializable {
  public:
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class HexTap : public Decoder
  {
      HexTapConfig config;
      uint8_t bps = 1;
      uint8_t mask = 1;

    public:
      HexTap(Suscan::DecoderFactory *manufacturer);

      Suscan::Serializable const &getConfig(void) const override;
      bool setConfig(Suscan::Serializable &config) override;

      std::string getStateString(void) const override;
      bool setInputBps(uint8_t bps) override;
      uint8_t getOutputBps(void) const override;
      bool work(FrameId frame, const Symbol *buffer, size_t len) override;
  };
}

#endif // HEXTAP_H
