#ifndef FrameSync_H
#define FrameSync_H

#include <Decoder.h>
#include <Decider.h>

#include <chopper.h>

namespace SigDigger {
  class FrameSyncConfig : public Suscan::Serializable {
  public:
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class FrameSyncUI;

  class FrameSync : public QObject, public Decoder
  {
      Q_OBJECT

    public:
      enum FrameSyncMode {
        REPETITION,
        SEQUENCE
      };

    private:
      FrameSyncConfig config;
      FrameSyncUI *ui = nullptr;
      Symbol last;
      unsigned int count = 0;
      unsigned int repetitions = 0;
      chopper_t *chopper = nullptr;

      FrameSyncMode mode = REPETITION;
      std::vector<Symbol> sequence;
      int times;

      uint8_t bps = 1;
      uint8_t mask = 1;

      void updateChopper(void);

    public:
      int frameCount(void) const;

      FrameSync(Suscan::DecoderFactory *manufacturer);
      void delayedConstructor(void);

      virtual ~FrameSync() override;

      Suscan::Serializable const &getConfig(void) const override;
      bool setConfig(Suscan::Serializable &config) override;

      std::string getStateString(void) const override;
      bool setInputBps(uint8_t bps) override;
      uint8_t getOutputBps(void) const override;
      bool work(FrameId frame, const Symbol *buffer, size_t len) override;

    public slots:
      void onUiChanged(void);
  };
}

#endif // FrameSync_H
