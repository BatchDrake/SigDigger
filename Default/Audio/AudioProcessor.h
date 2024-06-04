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
  class AnalyzerRequestTracker;
  struct AnalyzerRequest;
};

namespace SigDigger {
  class UIMediator;
  class AudioPlayback;
  class MainSpectrum;

  class AudioProcessor : public QObject
  {
    Q_OBJECT

    // Demodulator state
    Suscan::Orbit   m_orbit;
    bool            m_enabled = false;
    bool            m_agc = true;
    float           m_agcTimeScale = 1.;
    float           m_volume = 0;
    float           m_cutOff = 0;
    SUFREQ          m_lo = 0;
    SUFREQ          m_tuner = 0;
    unsigned int    m_sampleRate = 44100;
    unsigned int    m_requestedRate = 44100;
    AudioDemod      m_demod = AudioDemod::FM;
    bool            m_correctionEnabled = false;
    bool            m_squelch = false;
    SUFLOAT         m_squelchLevel;
    SUFREQ          m_bw = 2e5; // Hz

    // Composed objects
    AudioFileSaver *m_audioFileSaver = nullptr;
    QString         m_savedPath;
    AudioPlayback  *m_playBack = nullptr;
    Suscan::AnalyzerRequestTracker *m_tracker = nullptr;
    QString         m_audioError;
    std::string     m_audioDevice;

    // Audio inspector state
    bool              m_opened = false;
    bool              m_opening = false;
    bool              m_settingRate = false;
    Suscan::Analyzer *m_analyzer = nullptr;
    Suscan::Handle    m_audioInspHandle = -1;
    uint32_t          m_audioInspId = 0xffffffff;
    suscan_config_t  *m_audioCfgTemplate = nullptr;
    bool              m_audioInspectorOpened = false;
    SUFREQ            m_maxAudioBw = 2e5; // Hz

    // Other references
    MainSpectrum     *m_spectrum = nullptr;
    UIMediator       *m_mediator = nullptr;

    // Private methods
    void connectAll();
    void connectAudioFileSaver();
    void connectAnalyzer();
    void disconnectAnalyzer();
    bool openAudio();
    bool closeAudio();
    void setParams();
    void setTrueLoFreq();
    void setTrueBandwidth();
    void assertAudioDevice();

  public:
    explicit AudioProcessor(UIMediator *, QObject *parent = nullptr);
    virtual ~AudioProcessor() override;

    SUFREQ calcTrueLoFreq() const;
    SUFREQ calcTrueBandwidth() const;

    void setAnalyzer(Suscan::Analyzer *);
    void setEnabled(bool);
    void setVolume(float);
    void setSquelchEnabled(bool);
    void setAGCEnabled(bool);
    void setAGCTimeScale(float);
    void setSquelchLevel(float);
    void setAudioCorrection(Suscan::Orbit const &);
    void setCorrectionEnabled(bool);
    void setDemod(AudioDemod);
    void setSampleRate(unsigned);
    void setCutOff(float);
    void setTunerFreq(SUFREQ);
    void setLoFreq(SUFREQ);
    void setBandwidth(SUFREQ);

    SUFREQ getTrueChannelFreq() const;
    SUFREQ getChannelFreq() const;
    SUFREQ getChannelBandwidth() const;

    bool isAudioAvailable() const;
    QString getAudioError() const;
    bool isRecording() const;
    bool isOpened() const;
    size_t getSaveSize() const;

  signals:
    void audioClosed();
    void audioOpened();
    void audioError(QString);

    void recStopped();
    void recSwamped();
    void recSaveRate(qreal);
    void recCommit();

    void orbitReport(Suscan::InspectorMessage const &);
    void setTLE(Suscan::InspectorMessage const &);

  public slots:
    // These two are slots to trigger the recording stop on signal
    bool startRecording(QString);
    void stopRecording(void);

    void onInspectorMessage(Suscan::InspectorMessage const &);
    void onInspectorSamples(Suscan::SamplesMessage const &);
    void onOpened(Suscan::AnalyzerRequest const &);
    void onCancelled(Suscan::AnalyzerRequest const &);
    void onError(Suscan::AnalyzerRequest const &, std::string const &);
  };
}

#endif // AUDIOPROCESSOR_H
