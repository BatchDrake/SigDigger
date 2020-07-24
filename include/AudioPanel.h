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

namespace Ui {
  class AudioPanel;
}

namespace SigDigger {
  class AudioPanelConfig : public Suscan::Serializable {
  public:
    bool enabled = false;
    std::string demod;
    std::string savePath;
    unsigned int rate = 44100;
    SUFLOAT cutOff = 15000;
    SUFLOAT volume = 50;

    // Overriden methods
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class AudioPanel : public GenericDataSaverUI
  {
    Q_OBJECT

    // Convenience pointer
    AudioPanelConfig *panelConfig = nullptr;

    // Data
    SUFLOAT bandwidth = 200000;

    // Private methods
    void connectAll(void);
    void populateRates(void);
    void refreshUi(void);

    static AudioDemod strToDemod(std::string const &str);
    static std::string demodToStr(AudioDemod);

  protected:
      void setDiskUsage(qreal) override;

  public:
    explicit AudioPanel(QWidget *parent = nullptr);
    ~AudioPanel() override;

    // Setters
    void setBandwidth(SUFLOAT);
    void setEnabled(bool);
    void setDemod(enum AudioDemod);
    void setSampleRate(unsigned int);
    void setCutOff(SUFLOAT);
    void setVolume(SUFLOAT);
    void setMuted(bool);

    // Overriden setters
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

    // Overriden getters
    bool getRecordState(void) const override;
    std::string getRecordSavePath(void) const override;

    // Overriden methods
    Suscan::Serializable *allocConfig(void) override;
    void applyConfig(void) override;

  public slots:
    void onDemodChanged(void);
    void onSampleRateChanged(void);
    void onFilterChanged(void);
    void onVolumeChanged(void);
    void onMuteToggled(bool);
    void onEnabledChanged(void);

    void onChangeSavePath(void);
    void onRecordStartStop(void);

  signals:
    void changed(void);
    void volumeChanged(float);

  private:
    Ui::AudioPanel *ui = nullptr;
  };
}

#endif // AUDIOPANEL_H
