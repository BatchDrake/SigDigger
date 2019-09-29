//
//    FACFrameSync.h: UI for the FAC frame synchronizer
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

#ifndef FACFRAMESYNC_H
#define FACFRAMESYNC_H

#include <Decoder.h>
#include <Decider.h>
#include <FACSyncCracker.h>

namespace SigDigger {
  class FACFrameSyncConfig : public Suscan::Serializable {
  public:
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class FACFrameSyncUI;

  class FACFrameSync : public QObject, public Decoder
  {
      Q_OBJECT
      FACFrameSyncConfig config;
      FACSyncCracker *cracker = nullptr;
      FACFrameSyncUI *ui = nullptr;

      int frameLen = 0;
      int p = 0;
      int pShifted = 0;
      bool enabled = false;
      bool guessing = false;

      uint8_t bps = 1;
      uint8_t mask = 1;

    public:
      FACFrameSync(Suscan::DecoderFactory *manufacturer);
      void delayedConstructor(void);
      virtual ~FACFrameSync() override;

      Suscan::Serializable const &getConfig(void) const override;
      bool setConfig(Suscan::Serializable &config) override;

      std::string getStateString(void) const override;
      bool setInputBps(uint8_t bps) override;
      uint8_t getOutputBps(void) const override;
      bool work(FrameId frame, const Symbol *buffer, size_t len) override;

    public slots:
      void onToggleEnable(bool);
      void onToggleGuess(bool);
      void onApplyConfig(void);
      void onUiChanged(void);

      void onCrackerStateChanged(int state);
      void onCrackerProgress(qreal);
  };
}

#endif // FACFRAMESYNC_H
