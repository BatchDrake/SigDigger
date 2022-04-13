//
//    AudioProcessor.h: Audio processor
//    Copyright (C) 2022 Gonzalo José Carracedo Carballal
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
#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <QObject>
#include <Suscan/Library.h>
#include <Suscan/Analyzer.h>
#include <AudioFileSaver.h>

namespace Suscan {
  class Analyzer;
};

namespace SigDigger {
  class UIMediator;
  class AudioPlayback;

  class AudioProcessor : public QObject
  {
    Q_OBJECT

    // Demodulator state
    Suscan::Orbit   m_orbit;
    bool            m_enabled = false;
    float           m_volume = 0;
    float           m_cutOff = 0;
    SUFREQ          m_freq = 0;
    unsigned int    m_sampleRate = 44100;
    AudioDemod      m_demod = AudioDemod::FM;
    bool            m_correctionEnabled = false;

    // Composed objects
    AudioFileSaver *m_audioFileSaver = nullptr;
    AudioPlayback  *m_playBack = nullptr;

    // Analyzer state objects
    Suscan::Analyzer *m_analyzer = nullptr;
    Suscan::Handle    m_audioInspHandle = 0;
    suscan_config_t  *m_audioCfgTemplate = nullptr;
    bool              m_audioInspectorOpened = false;
    float             m_maxAudioBw = 2e5;
    SUFREQ            m_lastLo = 0;

    // Private methods
    void connectAnalyzer();

  public:
    explicit AudioProcessor(UIMediator *, QObject *parent = nullptr);

    void setAnalyzer(Suscan::Analyzer *);
    void setVolume(float);
    void setAudioCorrection(Suscan::Orbit const &);
    void setCorrectionEnabled(bool);
    void setDemod(AudioDemod);
    void setSampleRate(unsigned);
    void setCutOff(float);
    void setDemodFreq(SUFREQ);
    void startRecording(QString);
    void stopRecording(void);

  public slots:
    // Here: process Analyzer messages
  };
}

#endif // AUDIOPROCESSOR_H
