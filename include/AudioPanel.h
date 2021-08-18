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
#ifndef AUDIOPANEL_H
#define AUDIOPANEL_H

#include <PersistentWidget.h>
#include <GenericDataSaverUI.h>
#include <AudioFileSaver.h>
#include <ColorConfig.h>
#include <Suscan/Analyzer.h>
#include <Suscan/Library.h>
#include <sgdp4/sgdp4-types.h>

namespace Ui {
  class AudioPanel;
}

namespace SigDigger {
  class FrequencyCorrectionDialog;

  class AudioPanelConfig : public Suscan::Serializable {
  public:
    bool enabled = false;
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

    // Overridden methods
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class AudioPanel : public GenericDataSaverUI
  {
    Q_OBJECT

    // Convenience pointer
    AudioPanelConfig *panelConfig = nullptr;

    // Data
    SUFLOAT        bandwidth  = 200000;
    SUFREQ         demodFreq  = 0;
    bool           isRealTime = false;
    struct timeval timeStamp = {0, 0};

    // UI methods
    ColorConfig colorConfig;
    FrequencyCorrectionDialog *fcDialog = nullptr;

    // Private methods
    void connectAll(void);
    void populateRates(void);
    void refreshUi(void);

  protected:
      void setDiskUsage(qreal) override;

  public:
    static AudioDemod strToDemod(std::string const &str);
    static std::string demodToStr(AudioDemod);

    explicit AudioPanel(QWidget *parent = nullptr);
    ~AudioPanel() override;

    // Setters
    void setBandwidth(SUFLOAT);
    void setDemodFreq(qint64);
    void setRealTime(bool);
    void setEnabled(bool);
    void setDemod(enum AudioDemod);
    void setSampleRate(unsigned int);
    void setTimeStamp(struct timeval const &);
    void setTimeLimits(
        struct timeval const &start,
        struct timeval const &end);
    void resetTimeStamp(struct timeval const &);
    void setCutOff(SUFLOAT);
    void setVolume(SUFLOAT);
    void setQth(xyz_t const &);
    void setMuted(bool);
    void setColorConfig(ColorConfig const &);
    void setSquelchEnabled(bool);
    void setSquelchLevel(SUFLOAT);
    void notifyOrbitReport(Suscan::OrbitReport const &);
    void notifyDisableCorrection(void);

    // Overridden setters
    void setRecordSavePath(std::string const &) override;
    void setSaveEnabled(bool enabled) override;
    void setCaptureSize(quint64) override;
    void setIORate(qreal) override;
    void setRecordState(bool state) override;

    // Getters
    SUFLOAT getBandwidth(void) const;
    bool getEnabled(void) const;
    enum AudioDemod getDemod(void) const;
    unsigned int getSampleRate(void) const;
    SUFLOAT getCutOff(void) const;
    SUFLOAT getVolume(void) const;
    bool    isMuted(void) const;
    SUFLOAT getMuteableVolume(void) const;

    bool    isCorrectionEnabled(void) const;
    bool    getSquelchEnabled(void) const;
    SUFLOAT getSquelchLevel(void) const;
    Suscan::Orbit getOrbit(void) const;

    // Overridden getters
    bool getRecordState(void) const override;
    std::string getRecordSavePath(void) const override;

    // Overridden methods
    Suscan::Serializable *allocConfig(void) override;
    void applyConfig(void) override;

  public slots:
    void onDemodChanged(void);
    void onSampleRateChanged(void);
    void onFilterChanged(void);
    void onVolumeChanged(void);
    void onMuteToggled(bool);
    void onEnabledChanged(void);
    void onAcceptCorrectionSetting(void);

    void onChangeSavePath(void);
    void onRecordStartStop(void);
    void onToggleSquelch(void);
    void onSquelchLevelChanged(void);
    void onOpenDopplerSettings(void);

  signals:
    void changed(void);
    void volumeChanged(float);
    void setCorrection(Suscan::Orbit);
    void disableCorrection(void);

  private:
    Ui::AudioPanel *ui = nullptr;
  };
}

#endif // AUDIOPANEL_H
