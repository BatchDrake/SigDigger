//
//    SourceWidget.h: description
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
#ifndef SOURCEWIDGET_H
#define SOURCEWIDGET_H

#include "ToolWidgetFactory.h"
#include "DataSaverUI.h"
#include "DeviceGain.h"
#include "AutoGain.h"

namespace Ui {
  class SourcePanel;
}

namespace SigDigger{
  class SourceWidgetFactory;

  struct GainPresetSetting : public Suscan::Serializable {
    std::string driver;
    std::string name;
    int         value;

    // Overriden methods
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class SourceWidgetConfig : public Suscan::Serializable {
    public:
      Suscan::Serializable *dataSaverConfig = nullptr;
      bool collapsed = false;
      bool throttle = false;
      bool dcRemove = false;
      bool iqRev = false;
      bool agcEnabled = false;
      bool gainPresetEnabled = false;

      std::map<std::string, GainPresetSetting> agcSettings;
      unsigned int throttleRate = 196000;

      // Overriden methods
      void deserialize(Suscan::Object const &conf) override;
      Suscan::Object &&serialize(void) override;
  };

  class SourceWidget : public ToolWidget
  {
    Q_OBJECT

    // Convenience pointer
    SourceWidgetConfig *panelConfig = nullptr;

    // Managed objects
    Suscan::Source::Config *profile = nullptr;
    Suscan::AnalyzerSourceInfo sourceInfo =
        Suscan::AnalyzerSourceInfo();

    // UI objects
    int m_state;
    Suscan::Analyzer *m_analyzer = nullptr; // Borrowed
    bool m_haveSourceInfo = false;
    Ui::SourcePanel *ui = nullptr;
    std::vector<DeviceGain *> gainControls;
    DataSaverUI *saverUI = nullptr;

    // UI State
    unsigned int rate = 0;
    unsigned int processRate = 0;
    bool haveSourceInfo = false;
    std::map<std::string, std::vector<AutoGain>> autoGains;
    bool throttleable = false;
    std::vector<AutoGain> *currAutoGainSet = nullptr;
    AutoGain *currentAutoGain = nullptr;

    // Private methods
    DeviceGain *lookupGain(std::string const &name);
    void clearGains(void);
    void refreshGains(Suscan::Source::Config &config);
    bool tryApplyGains(Suscan::AnalyzerSourceInfo const &info);
    void selectAutoGain(unsigned int);
    bool selectAutoGain(std::string const &);
    void refreshAutoGains(Suscan::Source::Config &config);
    void refreshCurrentAutoGain(std::string const &);
    void applyCurrentAutogain(void);
    void selectAntenna(std::string const &name);
    void setBandwidth(float bw);
    void setPPM(float ppm);
    void populateAntennaCombo(Suscan::AnalyzerSourceInfo const &info);
    void connectAll(void);
    void refreshUi(void);

    void setThrottleable(bool val);
    void setSampleRate(unsigned int rate);
    unsigned int getEffectiveRate(void) const;
    void setProcessRate(unsigned int rate);
    void applySourceInfo(Suscan::AnalyzerSourceInfo const &info);
    void setGain(std::string const &name, SUFLOAT val);

    void setCaptureSize(quint64);
    void setIORate(qreal);
    void setRecordState(bool state);
    void setSavePath(std::string const &path);
    void setDCRemove(bool remove);
    void setIQReverse(bool rev);
    void setAGCEnabled(bool enabled);

    bool setBlockingSignals(bool);

  public:
    SourceWidget(SourceWidgetFactory *, UIMediator *, QWidget *parent = nullptr);
    ~SourceWidget() override;

    // Configuration methods
    Suscan::Serializable *allocConfig() override;
    void applyConfig() override;
    bool event(QEvent *) override;

    // Overriden methods
    void setState(int, Suscan::Analyzer *) override;
    void setProfile(Suscan::Source::Config &) override;

  public slots:
    void onSourceInfoMessage(Suscan::SourceInfoMessage const &msg);
    void onPSDMessage(Suscan::PSDMessage const &msg);
    void onGainChanged(QString name, float val);
    void onAntennaChanged(int);
    void onRecordStartStop(void);
    void onSelectAutoGain(void);
    void onToggleAutoGain(void);
    void onChangeAutoGain(void);
    void onThrottleChanged(void);
    void onToggleDCRemove(void);
    void onToggleIQReverse(void);
    void onToggleAGCEnabled(void);
    void onBandwidthChanged(void);
    void onPPMChanged(void);
  };
}

#endif // SOURCEWIDGET_H
