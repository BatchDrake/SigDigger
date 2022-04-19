//
//    Default/Audio/AudioWidget.h: description
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
#ifndef AUDIOWIDGET_H
#define AUDIOWIDGET_H

#include <ToolWidgetFactory.h>
#include <ColorConfig.h>
#include <AudioFileSaver.h>

namespace Ui {
  class AudioPanel;
}

namespace SigDigger {
  class AudioProcessor;
  class AudioWidgetFactory;
  class FrequencyCorrectionDialog;
  class MainSpectrum;

  class AudioWidgetConfig : public Suscan::Serializable {
  public:
    bool enabled = false;
    bool collapsed = false;
    std::string demod;
    std::string savePath;
    unsigned int rate   = 44100;
    SUFLOAT cutOff      = 15000;
    SUFLOAT volume      = -6;

    bool squelch        = false;
    SUFLOAT amSquelch   = .1f;
    SUFLOAT ssbSquelch  = 1e-3f;

    bool tleCorrection  = false;
    bool isSatellite    = false;
    std::string satName = "ISS (ZARYA)";
    std::string tleData = "";

    // Overriden methods
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize() override;
  };

  class AudioWidget : public ToolWidget
  {
    Q_OBJECT

    AudioWidgetConfig *panelConfig = nullptr;

    // Data
    SUFLOAT        bandwidth  = 200000;
    SUFREQ         demodFreq  = 0;
    bool           isRealTime = false;
    struct timeval timeStamp  = {0, 0};

    // Processing members
    AudioProcessor *m_processor  = nullptr;
    Suscan::Analyzer *m_analyzer = nullptr; // Borrowed
    bool m_haveSourceInfo = false;
    bool m_audioAllowed = true;

    // UI members
    int m_state;
    MainSpectrum *m_spectrum;
    Ui::AudioPanel *ui = nullptr;
    ColorConfig colorConfig;
    FrequencyCorrectionDialog *fcDialog = nullptr;


    // Private methods
    void connectAll();
    void populateRates();
    void refreshUi();

    // Private setters
    void setBandwidth(SUFLOAT);
    void setDemodFreq(qint64);
    void setEnabled(bool);
    void setDemod(enum AudioDemod);
    void setSampleRate(unsigned int);
    void setCutOff(SUFLOAT);
    void setVolume(SUFLOAT);
    void setMuted(bool);
    void setSquelchEnabled(bool);
    void setSquelchLevel(SUFLOAT);

    // Recorder state setters
    void setDiskUsage(qreal);
    void refreshDiskUsage();
    void setRecordSavePath(std::string const &);
    void setSaveEnabled(bool enabled);
    void setCaptureSize(quint64);
    void setIORate(qreal);
    void setRecordState(bool state);

    // Private getters
    SUFLOAT getBandwidth() const;
    bool getEnabled() const;
    bool shouldOpenAudio() const;
    enum AudioDemod getDemod() const;
    unsigned int getSampleRate() const;
    SUFLOAT getCutOff() const;
    SUFLOAT getVolume() const;
    bool    isMuted() const;
    SUFLOAT getMuteableVolume() const;
    bool    isCorrectionEnabled() const;
    bool    getSquelchEnabled() const;
    SUFLOAT getSquelchLevel() const;
    Suscan::Orbit getOrbit() const;
    bool getRecordState(void) const;
    std::string getRecordSavePath(void) const;

  public:
    AudioWidget(AudioWidgetFactory *, UIMediator *, QWidget *parent = nullptr);
    ~AudioWidget() override;

    // Configuration methods
    Suscan::Serializable *allocConfig() override;
    void applyConfig() override;
    bool event(QEvent *) override;

    // Overriden methods
    void setState(int, Suscan::Analyzer *) override;
    void setQth(Suscan::Location const &) override;
    void setColorConfig(ColorConfig const &) override;
    void setTimeStamp(struct timeval const &) override;
    void setProfile(Suscan::Source::Config &) override;

  public slots:
    void onSpectrumBandwidthChanged(void);
    void onSpectrumLoChanged(qint64);
    void onSpectrumFrequencyChanged(qint64 freq);

    // Generic UI
    void onDemodChanged();
    void onSampleRateChanged();
    void onFilterChanged();
    void onVolumeChanged();
    void onMuteToggled(bool);
    void onEnabledChanged();
    void onAcceptCorrectionSetting();
    void onChangeSavePath();
    void onRecordStartStop();
    void onToggleSquelch();
    void onSquelchLevelChanged();
    void onOpenDopplerSettings();

    // Notifications
    void onSetTLE(Suscan::InspectorMessage const &);
    void onOrbitReport(Suscan::InspectorMessage const &);

    // Saver UI
    void onAudioSaveError(void);
    void onAudioSaveSwamped(void);
    void onAudioSaveRate(qreal rate);
    void onAudioCommit(void);

    // Analyzer slots
    void onSourceInfoMessage(Suscan::SourceInfoMessage const &);
  };
}

#endif // AUDIOWIDGET_H
