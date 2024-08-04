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
#ifndef ALSAPLAYER_H
#define ALSAPLAYER_H

#include <GenericAudioPlayer.h>
#include <alsa/asoundlib.h>

#define ALSAPLAYER_UNDERRUN_WAIT_PERIOD_MS 150

namespace SigDigger {
  class AlsaPlayer : public GenericAudioPlayer {
    snd_pcm_t *pcm = nullptr;

  public:
    AlsaPlayer(std::string const &dev, unsigned int rate, size_t bufSiz);
    static bool enumerateDevices(std::vector<GenericAudioDevice> &);
    static GenericAudioDevice getDefaultDevice();
    bool write(const float *, size_t) override;
    ~AlsaPlayer() override;
  };
}

#endif // ALSAPLAYER_H
