//
//    SourcePanel.h: source control UI
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

#ifndef SOURCEPANEL_H
#define SOURCEPANEL_H

#include <PersistentWidget.h>
#include <Suscan/Analyzer.h>

#include "DataSaverUI.h"
#include "DeviceGain.h"
#include "AutoGain.h"

namespace Ui {
  class SourcePanel;
}

namespace SigDigger {
  class SourcePanelConfig : public Suscan::Serializable {
    public:
      bool throttle = false;
      bool dcRemove = false;
      bool iqRev = false;
      bool agcEnabled = false;

      unsigned int throttleRate = 196000;
      std::string captureFolder;

      // Overriden methods
      void deserialize(Suscan::Object const &conf) override;
      Suscan::Object &&serialize(void) override;
  };

  class SourcePanel : public PersistentWidget
  {
      Q_OBJECT

    public:
      enum State {
        DETACHED,
        ATTACHED
      };

    private:
      // Convenience pointer
      SourcePanelConfig *panelConfig = nullptr;

      // Managed objects
      Suscan::Source::Config *profile = nullptr;

      // UI objects
      Ui::SourcePanel *ui = nullptr;
      std::vector<DeviceGain *> gainControls;
      DataSaverUI *saverUI = nullptr;

      // UI State
      unsigned int rate = 0;
      unsigned int processRate = 0;
      State state = DETACHED;
      std::map<std::string, std::vector<AutoGain>> autoGains;
      bool throttleable = false;
      std::vector<AutoGain> *currAutoGainSet;
      AutoGain *currentAutoGain = nullptr;

      // Private methods
      DeviceGain *lookupGain(std::string const &name);
      void clearGains(void);
      void refreshGains(Suscan::Source::Config &config);
      void selectAutoGain(unsigned int);
      void refreshAutoGains(Suscan::Source::Config &config);
      void applyCurrentAutogain(void);
      void selectAntenna(std::string const &name);
      void setBandwidth(float bw);
      void connectAll(void);
      void refreshUi(void);

    public:
      // Inlined methods
      enum State
      getState(void) const
      {
        return this->state;
      }

      std::string
      getRecordSavePath(void) const
      {
        return this->panelConfig->captureFolder;
      }

      bool
      isThrottleEnabled(void) const
      {
        return this->panelConfig->throttle;
      }

      bool
      getDCremove(void) const
      {
        return this->panelConfig->dcRemove;
      }

      bool
      getIQReverse(void) const
      {
        return this->panelConfig->iqRev;
      }

      bool
      getAGCEnabled(void) const
      {
        return this->panelConfig->agcEnabled;
      }

      unsigned int
      getThrottleRate(void) const
      {
        return this->panelConfig->throttleRate;
      }

      unsigned int
      getEffectiveRate(void) const
      {
        return this->throttleable && this->isThrottleEnabled()
            ? this->getThrottleRate()
            : this->rate;
      }

      explicit SourcePanel(QWidget *parent = nullptr);
      ~SourcePanel() override;

      // Public API:
      void deserializeAutoGains(void);

      void setThrottleable(bool val);
      void setProfile(Suscan::Source::Config *);
      void setSampleRate(unsigned int rate);
      void setProcessRate(unsigned int rate);

      void setGain(std::string const &name, SUFLOAT val);

      void setCaptureSize(quint64);
      void setDiskUsage(qreal);
      void setIORate(qreal);
      void setRecordState(bool state);
      void setSavePath(std::string const &path);
      void setState(enum State state);
      void setDCRemove(bool remove);
      void setIQReverse(bool rev);
      void setAGCEnabled(bool enabled);

      // Getters
      bool getRecordState(void) const;
      std::string getAntenna(void) const;
      float getBandwidth(void) const;

      // Overriden methods
      Suscan::Serializable *allocConfig(void) override;
      void applyConfig(void) override;

    signals:
      void toggleRecord(void);
      void throttleConfigChanged(void);
      void gainChanged(QString name, float val);
      void antennaChanged(QString name);
      void toggleDCRemove(void);
      void toggleIQReverse(void);
      void toggleAGCEnabled(void);
      void bandwidthChanged(void);

    public slots:
      void onGainChanged(QString name, float val);
      void onAntennaChanged(int);
      void onChangeSavePath(void);
      void onRecordStartStop(void);
      void onSelectAutoGain(void);
      void onToggleAutoGain(void);
      void onChangeAutoGain(void);
      void onThrottleChanged(void);
      void onToggleDCRemove(void);
      void onToggleIQReverse(void);
      void onToggleAGCEnabled(void);
      void onBandwidthChanged(void);
  };
};

#endif // SOURCEPANEL_H
