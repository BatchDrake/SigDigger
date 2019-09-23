#ifndef HEXTAP_H
#define HEXTAP_H

#include <Decoder.h>
#include <Decider.h>

namespace SigDigger {
  class HexTapConfig : public Suscan::Serializable {
  public:
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  typedef std::vector<Symbol> Frame;
  typedef std::vector<uint8_t> FrameBytes;

  class HexTapUI;

  class HexTap : public QObject, public Decoder
  {
      Q_OBJECT

      HexTapConfig config;
      HexTapUI *ui = nullptr;
      uint8_t bps = 1;
      uint8_t mask = 1;
      uint8_t currByte = 0;
      int ptr = 0;

      int shift = 0;
      std::vector<Symbol> lsb2msb;
      bool lsb = true;
      bool reverse = false;
      bool empty = true;
      bool pack = false;

      FrameId currFrameId = 0;
      FrameId relFrameId = 0;
      FrameId selectedFrameId = 0;

      std::vector<Frame> frames;
      Frame *current = nullptr;

      FrameBytes selectedFrameBytes;

      void recalcSelectedFrameBytes(void);
      void feedSelected(const Symbol *, size_t);
      void selectFrame(FrameId id);

    public:
      int frameCount(void) const;
      FrameBytes *getFrame(FrameId id);

      HexTap(Suscan::DecoderFactory *manufacturer);
      void delayedConstructor(void);

      virtual ~HexTap() override;

      Suscan::Serializable const &getConfig(void) const override;
      bool setConfig(Suscan::Serializable &config) override;

      std::string getStateString(void) const override;
      bool setInputBps(uint8_t bps) override;
      uint8_t getOutputBps(void) const override;
      bool work(FrameId frame, const Symbol *buffer, size_t len) override;

    public slots:
      void onClear(void);
      void onUiChanged(void);
  };
}

#endif // HEXTAP_H
