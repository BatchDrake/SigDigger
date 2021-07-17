//
//    AudioSaver.cpp: Save demodualted audio to file
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

#include <AudioFileSaver.h>
#include <sndfile.h>
#include <unistd.h>

using namespace SigDigger;

namespace SigDigger {
  class AudioFileWriter : public GenericDataWriter {
    AudioFileSaver::AudioFileParams params;
    std::string fullPath;
    std::string lastError;
    SNDFILE *sfp = nullptr;

  public:
    AudioFileWriter(AudioFileSaver::AudioFileParams const &params);
    ~AudioFileWriter();

    bool prepare(void);
    bool canWrite(void) const;
    std::string getError(void) const;
    ssize_t write(const SUCOMPLEX *data, size_t len);
    bool close(void);
  };
}

std::string
AudioFileWriter::getError(void) const
{
  return this->lastError;
}

bool
AudioFileWriter::prepare(void)
{
  if (this->sfp == nullptr) {
    char fileName[128];
    unsigned int index = 1;
    SF_INFO sfinfo;
    std::string modulation;

    switch (this->params.modulation) {
      case AM:
        modulation = "AM";
        break;

      case FM:
        modulation = "FM";
        break;

      case USB:
        modulation = "USB";
        break;

      case LSB:
        modulation = "LSB";
        break;
    }

    do {
      snprintf(
            fileName,
            sizeof(fileName),
            "audio-%s-%.0lf-%d-%04d.wav",
            modulation.c_str(),
            this->params.frequency,
            this->params.sampRate,
            index++);
      this->fullPath = this->params.savePath + "/" + fileName;
    } while (access(this->fullPath.c_str(), F_OK) != -1);

    sfinfo.channels = 1;
    sfinfo.samplerate = static_cast<int>(this->params.sampRate);
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    if ((this->sfp = sf_open(this->fullPath.c_str(), SFM_WRITE, &sfinfo))
        == nullptr) {
      this->lastError =
          std::string("Save file ")
          + this->fullPath
          + std::string(" failed: ")
          + sf_strerror(nullptr);
      return false;
    }
  }

  return true;
}

AudioFileWriter::AudioFileWriter(AudioFileSaver::AudioFileParams const &p)
{
  this->params = p;
}

AudioFileWriter::~AudioFileWriter(void)
{
  this->close();
}

bool
AudioFileWriter::canWrite(void) const
{
  return this->sfp != nullptr;
}

ssize_t
AudioFileWriter::write(const SUCOMPLEX *data, size_t len)
{
  ssize_t result;
  std::vector<SUFLOAT> realData;
  unsigned int i;

  if (this->sfp == nullptr)
    return 0;

  realData.resize(len);

  for (i = 0; i < len; ++i)
    realData[i] = SU_C_REAL(data[i]);

  result = sf_write_float(this->sfp, realData.data(), len);

  return result;
}

bool
AudioFileWriter::close(void)
{
  if (this->sfp != nullptr) {
    sf_close(this->sfp);
    this->sfp = nullptr;
  }

  return true;
}
//////////////////////////////// AudioFileSaver ///////////////////////////////


AudioFileSaver::~AudioFileSaver()
{
  if (this->writer != nullptr)
    delete this->writer;
}

AudioFileSaver::AudioFileSaver(
    AudioFileParams const &params,
    QObject *parent) :
  GenericDataSaver(this->writer = new AudioFileWriter(params), parent)
{
  this->params = params;
  this->setSampleRate(params.sampRate);
}
