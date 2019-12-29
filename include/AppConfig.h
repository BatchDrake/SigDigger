//
//    AppConfig.h: Application configuration object
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

#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <Suscan/Source.h>
#include <Suscan/Serializable.h>
#include <Suscan/AnalyzerParams.h>
#include <AppUI.h>

#include "ColorConfig.h"

#define SIGDIGGER_FFT_WINDOW_SIZE  4096u
#define SIGDIGGER_FFT_REFRESH_RATE 25u

#define SIGDIGGER_MAX_SAMPLE_RATE             3000000
#define SIGDIGGER_AUDIO_INSPECTOR_SAMPLE_RATE 44100
#define SIGDIGGER_AUDIO_INSPECTOR_MAGIC_ID    0xa01d10ff
#define SIGDIGGER_AUDIO_INSPECTOR_BANDWIDTH   200000
#define SIGDIGGER_AUDIO_INSPECTOR_REQID       0xaaaaaaaa

#define SIGDIGGER_PROFILE_FILE_MAX_SIZE       (1 << 20)

namespace SigDigger {  
  class AppConfig : public Suscan::Serializable {
    public:
      Suscan::Source::Config profile;
      Suscan::AnalyzerParams analyzerParams;
      ColorConfig colors;
      Suscan::Serializable *sourceConfig = nullptr;
      Suscan::Serializable *fftConfig = nullptr;
      Suscan::Serializable *inspectorConfig = nullptr;
      Suscan::Serializable *audioConfig = nullptr;

      int width = 1280;
      int height = 1024;
      int x = -1;
      int y = -1;

      int loFreq = 0;
      unsigned int bandwidth = 0;

      // Methods
      AppConfig(AppUI *ui = nullptr);
      [[ noreturn ]] AppConfig(Suscan::Object const &conf);

      void loadDefaults(void);

      // Overriden methods
      void deserialize(Suscan::Object const &conf) override;
      Suscan::Object &&serialize(void) override;
  };
}

#endif // APPCONFIG_H
