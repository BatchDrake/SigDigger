//
//    PortAudioPlayer.cpp: PortAudio player
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

#include <PortAudioPlayer.h>

#define ATTEMPT(expr, what) \
  if ((err = expr) < 0)  \
    throw std::runtime_error("Failed to " + std::string(what) + ": " + std::string(snd_strerror(err)))


using namespace SigDigger;

bool PortAudioPlayer::initialized = false;

bool
PortAudioPlayer::assertPaInitialization()
{
  if (!initialized) {
    PaError err = Pa_Initialize();
    initialized = err == paNoError;

    if (initialized)
      atexit(PortAudioPlayer::paFinalizer);
  }

  return initialized;
}

void
PortAudioPlayer::paFinalizer()
{
  if (initialized)
    Pa_Terminate();
}

PortAudioPlayer::PortAudioPlayer(
    std::string const &devStr,
    unsigned int rate,
    size_t bufSiz)
  : GenericAudioPlayer(rate)
{
  PaStreamParameters outputParameters;
  PaError pErr;
  PaDeviceIndex index;

  if (!assertPaInitialization())
    throw std::runtime_error("Failed to initialize PortAudio library");

  index = strToDeviceIndex(devStr);
  if (index == paNoDevice)
    throw std::runtime_error("Failed to initialize PortAudio library: playback device not found");

  outputParameters.device = index;
  outputParameters.channelCount = 1;
  outputParameters.sampleFormat = paFloat32;
  outputParameters.suggestedLatency =
      Pa_GetDeviceInfo(outputParameters.device)->defaultHighOutputLatency;
  outputParameters.hostApiSpecificStreamInfo = nullptr;

  pErr = Pa_OpenStream(
     &this->stream,
     nullptr,
     &outputParameters,
     rate,
     bufSiz,
     paClipOff,
     nullptr,
     nullptr);

  if (pErr != paNoError)
      throw std::runtime_error(
          std::string("Failed to open PortAudio stream: ")
          + Pa_GetErrorText(pErr));

  pErr = Pa_StartStream( stream );

  if (pErr != paNoError)
      throw std::runtime_error(
          std::string("Failed to start PortAudio stream: ")
          + Pa_GetErrorText(pErr));
}

GenericAudioDevice
PortAudioPlayer::deviceIndexToDevice(PaDeviceIndex index)
{
  GenericAudioDevice dev;
  const PaDeviceInfo  *deviceInfo;
  const PaHostApiInfo *apiInfo;

  if (index == paNoDevice)
    goto done;

  if (!assertPaInitialization())
    goto done;

  deviceInfo = Pa_GetDeviceInfo(index);
  if (deviceInfo == nullptr)
    goto done;

  apiInfo    = Pa_GetHostApiInfo(deviceInfo->hostApi);
  if (apiInfo == nullptr)
    goto done;

  dev.devStr      = std::string(apiInfo->name) + ":" + std::string(deviceInfo->name);
  dev.description = std::string(deviceInfo->name) + " (" + std::string(apiInfo->name) + ")";

done:
  return dev;
}

GenericAudioDevice
PortAudioPlayer::getDefaultDevice()
{
  return deviceIndexToDevice(Pa_GetDefaultOutputDevice());
}

PaDeviceIndex
PortAudioPlayer::strToDeviceIndex(std::string const &string)
{
  const PaDeviceInfo  *deviceInfo;
  const PaHostApiInfo *apiInfo;
  std::string devStr;
  int numDevices;

  if (string.size() == 0)
    return Pa_GetDefaultOutputDevice();

  numDevices = Pa_GetDeviceCount();
  for (int i = 0; i < numDevices; ++i) {
    deviceInfo = Pa_GetDeviceInfo(i);
    apiInfo    = Pa_GetHostApiInfo(deviceInfo->hostApi);

    devStr     = std::string(apiInfo->name) + ":" + std::string(deviceInfo->name);
    if (string == devStr)
      return i;
  }

  return paNoDevice;
}

bool
PortAudioPlayer::enumerateDevices(std::vector<GenericAudioDevice> &list)
{
  GenericAudioDevice   dev;
  const PaDeviceInfo  *deviceInfo;
  const PaHostApiInfo *apiInfo;

  int numDevices;

  if (!assertPaInitialization())
    return false;

  numDevices = Pa_GetDeviceCount();

  list.clear();

  for (int i = 0; i < numDevices; ++i) {
    deviceInfo = Pa_GetDeviceInfo(i);
    apiInfo    = Pa_GetHostApiInfo(deviceInfo->hostApi);

    dev.devStr      = std::string(apiInfo->name) + ":" + std::string(deviceInfo->name);
    dev.description = std::string(deviceInfo->name) + " (" + std::string(apiInfo->name) + ")";

    list.push_back(dev);
  }

  return true;
}

bool
PortAudioPlayer::write(const float *buffer, size_t len)
{
  PaError err;

  // TODO: How about writing silence?

  err = Pa_WriteStream(this->stream, buffer, len);
  if (err == paOutputUnderflowed)
    err = Pa_WriteStream(this->stream, buffer, len);

  return err == paNoError;
}

PortAudioPlayer::~PortAudioPlayer()
{
  if (this->stream != nullptr) {
    Pa_StopStream(this->stream);
    Pa_CloseStream(this->stream);
  }
}
