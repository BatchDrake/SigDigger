//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
#ifndef PORTAUDIOPLAYER_H
#define PORTAUDIOPLAYER_H

#include <GenericAudioPlayer.h>
#include <portaudio.h>

namespace SigDigger {
  class PortAudioPlayer : public GenericAudioPlayer
  {
    PaStream *stream = nullptr;
    static bool initialized;

    static bool assertPaInitialization(void);
    static void paFinalizer(void);

  public:
    PortAudioPlayer(std::string const &dev, unsigned int rate, size_t bufSiz);
    static GenericAudioDevice deviceIndexToDevice(PaDeviceIndex index);
    static PaDeviceIndex strToDeviceIndex(std::string const &);
    static GenericAudioDevice getDefaultDevice();
    static bool enumerateDevices(std::vector<GenericAudioDevice> &);
    bool write(const float *samples, size_t size) override;
    ~PortAudioPlayer() override;
  };
}

#endif // PORTAUDIOPLAYER_H
