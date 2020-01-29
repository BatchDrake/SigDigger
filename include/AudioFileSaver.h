//
//    AudioSaver.h: Save demodualted audio to file
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
#ifndef AUDIOFILESAVER_H
#define AUDIOFILESAVER_H

#include <GenericDataSaver.h>
#include <string>

namespace SigDigger {
  class AudioFileWriter;

  enum AudioDemod {
    AM,
    FM,
    USB,
    LSB
  };

  class AudioFileSaver : public GenericDataSaver {
    Q_OBJECT

    AudioFileWriter *writer = nullptr;

  public:
    struct AudioFileParams {
      std::string savePath;
      AudioDemod modulation;
      SUFREQ frequency;
      unsigned int sampRate;
    };

    AudioFileParams params;

    AudioFileSaver(AudioFileParams const &, QObject *);
    ~AudioFileSaver();
  };
}

#endif // AUDIOFILESAVER_H
