//
//    InspectorPanel.h: Dockable inspector panel
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
#ifndef INSPECTORPANEL_H
#define INSPECTORPANEL_H

#include <PersistentWidget.h>
#include <TimeWindow.h>

namespace Ui {
  class InspectorPanel;
}

namespace SigDigger {
  class InspectorPanelConfig : public Suscan::Serializable {
  public:
    std::string inspectorClass = "psk";
    bool precise = false;

    // Overriden methods
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class InspectorPanel : public PersistentWidget
  {
    Q_OBJECT
    enum State {
      DETACHED,
      ATTACHED
    };

  private:
    // Convenience pointer
    InspectorPanelConfig *panelConfig = nullptr;

    // UI objects
    Ui::InspectorPanel *ui = nullptr;

    // TODO: Allow multiple TimeWindows
    TimeWindow *timeWindow = nullptr;
    qreal timeWindowFs = 1;

    // UI State
    State state = DETACHED;

    // Private methods
    void connectAll(void);
    void refreshUi(void);
    void setInspectorClass(std::string const &cls);

    std::vector<SUCOMPLEX> data;

  public:
    explicit InspectorPanel(QWidget *parent = nullptr);
    void postLoadInit(void);
    ~InspectorPanel() override;

    void setDemodFrequency(qint64);
    void setBandwidthLimits(unsigned int min, unsigned int max);
    void setBandwidth(unsigned int freq);
    void setPrecise(bool precise);
    void setState(enum State state);

    void resetRawInspector(qreal sampleRate);
    void feedRawInspector(const SUCOMPLEX *data, size_t size);

    unsigned int getBandwidth(void) const;
    std::string getInspectorClass(void) const;
    bool getPrecise(void) const;
    enum State getState(void) const;

    // Overriden methods
    Suscan::Serializable *allocConfig(void) override;
    void applyConfig(void) override;

  public slots:
    void onOpenInspector(void);
    void onBandwidthChanged(int);
    void onPreciseChanged(void);
    void onPressHold(void);
    void onReleaseHold(void);

  signals:
    void bandwidthChanged(int);
    void requestOpenInspector(QString);
    void startRawCapture(void);
    void stopRawCapture(void);
  };
}

#endif // INSPECTORPANEL_H
