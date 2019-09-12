//
//    Decoder.h: Abstract decoder
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
#ifndef DECODER_H
#define DECODER_H

#include <Suscan/Compat.h>
#include <Suscan/Serializable.h>
#include <Suscan/DecoderFactory.h>

#include <Decider.h>

#define SIGDIGGER_DECODER_DEFAULT_BUFFER_SIZE 8192

namespace SigDigger {
  typedef uint32_t FrameId;

  class DummyDecoderConfig : public Suscan::Serializable {
  public:
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class Decoder : public Suscan::Decoder
  {
    std::vector<Symbol> buffer;
    Suscan::DecoderFactory *manufacturer = nullptr;
    FrameId frame = 0;
    size_t bufferSize = SIGDIGGER_DECODER_DEFAULT_BUFFER_SIZE;
    Decoder *next = nullptr;

  protected:
    bool write(const Symbol *output, size_t len);
    bool nextFrame(void);

  public:
    Decoder(Suscan::DecoderFactory *manufacturer);
    Suscan::DecoderFactory *getManufacturer(void) const;
    virtual ~Decoder();
    void setBufferSize(size_t size);
    bool flush(void);
    bool plug(Decoder *next);

    // Virtual methods
    virtual Suscan::Serializable const &getConfig(void) const = 0;
    virtual bool setConfig(Suscan::Serializable &config) = 0;
    virtual std::string getStateString(void) const = 0;
    virtual bool setInputBps(uint8_t bps) = 0;
    virtual uint8_t getOutputBps(void) const = 0;
    virtual bool work(FrameId frame, const Symbol *buffer, size_t len) = 0;
  };
}

#endif // DECODER_H
