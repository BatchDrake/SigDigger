//
//    AlsaPlayer.cpp: Alsa sound player
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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

#include <AlsaPlayer.h>

using namespace SigDigger;

#define ATTEMPT(expr, what) \
  if ((err = expr) < 0)  \
    throw std::runtime_error("Failed to " + std::string(what) + " in ALSA player: " + std::string(snd_strerror(err)))


AlsaPlayer::AlsaPlayer(
    std::string const &dev,
    unsigned int rate,
    size_t bufSiz) :
  GenericAudioPlayer(rate)
{
  int err;
  snd_pcm_hw_params_t *params = nullptr;

  ATTEMPT(
        snd_pcm_open(&this->pcm, dev.c_str(), SND_PCM_STREAM_PLAYBACK, 0),
        "open audio device " + dev);

  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(pcm, params);

  ATTEMPT(
        snd_pcm_hw_params_set_access(
          this->pcm,
          params,
          SND_PCM_ACCESS_RW_INTERLEAVED),
        "set interleaved access for audio device");

  ATTEMPT(
        snd_pcm_hw_params_set_format(
          this->pcm,
          params,
          SND_PCM_FORMAT_FLOAT_LE),
        "set sample format");

  ATTEMPT(
        snd_pcm_hw_params_set_buffer_size(
          this->pcm,
          params,
          bufSiz),
        "set buffer size");

  ATTEMPT(
        snd_pcm_hw_params_set_channels(this->pcm, params, 1),
        "set output to mono");

  ATTEMPT(
        snd_pcm_hw_params_set_rate_near(this->pcm, params, &rate, nullptr),
        "set sample rate");

  ATTEMPT(snd_pcm_hw_params(this->pcm, params), "set device params");
}

bool
AlsaPlayer::write(const float *buffer, size_t len)
{
  long err;

  err = snd_pcm_writei(pcm, buffer, len);

  if (err == -EPIPE) {
    usleep(ALSAPLAYER_UNDERRUN_WAIT_PERIOD_MS * 1000);
    snd_pcm_prepare(pcm);
    err = snd_pcm_writei(pcm, buffer, len);
  }

  return err >= 0;
}

AlsaPlayer::~AlsaPlayer()
{
  if (this->pcm != nullptr) {
    snd_pcm_drain(this->pcm);
    snd_pcm_close(this->pcm);
  }
}
