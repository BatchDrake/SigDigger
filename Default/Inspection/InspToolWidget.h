//
//    InspToolWidget.h: Dockable inspector panel
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
#ifndef InspToolWidget_H
#define InspToolWidget_H

#include <ToolWidgetFactory.h>
#include <TimeWindow.h>
#include <ColorConfig.h>
#include <Suscan/Analyzer.h>
#include <Suscan/AnalyzerRequestTracker.h>

#define SIGDIGGER_DEFAULT_SQUELCH_TRIGGER  10
#define SIGDIGGER_DEFAULT_UPDATEUI_PERIOD_MS 250.

namespace Ui {
  class InspectorPanel;
}

namespace Suscan {
  class AnalyzerRequestTracker;
  struct AnalyzerRequest;
}

namespace SigDigger {
  class MultitaskController;
  class InspToolWidgetFactory;

  class InspToolWidgetConfig : public Suscan::Serializable {
  public:
    bool collapsed = true;
    std::string inspectorClass    = "psk";
    std::string palette           = "Suscan";
    std::string inspFactory       = "GenericInspector";
    SUFLOAT autoSquelchTriggerSNR = SIGDIGGER_DEFAULT_SQUELCH_TRIGGER;
    unsigned int paletteOffset;
    int paletteContrast;
    bool precise = false;

    // Overriden methods
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize() override;
  };

  class InspToolWidget : public ToolWidget
  {
    Q_OBJECT

  public:
    enum State {
      DETACHED,
      ATTACHED
    };

  private:
    // Convenience pointer
    InspToolWidgetConfig *m_panelConfig = nullptr;

    // UI objects
    Ui::InspectorPanel *m_ui = nullptr;
    bool m_stateSet = false;

    // TODO: Allow multiple TimeWindows
    TimeWindow *m_timeWindow = nullptr;
    qreal       m_timeWindowFs = 1;
    qint64      m_demodFreq = 0;
    SUFLOAT     m_squelch = 0;
    SUFLOAT     m_hangLevel = 0;
    bool        m_autoSquelch = false;
    bool        m_autoSquelchTriggered = false;

    Suscan::AnalyzerRequestTracker *m_tracker  = nullptr;
    Suscan::Analyzer               *m_analyzer = nullptr;
    bool                            m_opened   = false;
    Suscan::AnalyzerRequest         m_request;
    // UI State
    State m_state = DETACHED;
    Suscan::AnalyzerSourceInfo m_sourceInfo =
        Suscan::AnalyzerSourceInfo();

    std::vector<SUCOMPLEX> m_data;
    std::vector<SUCOMPLEX> m_history;
    unsigned int m_historyPtr = 0;
    SUFLOAT  m_currEnergy = 0;
    SUFLOAT  m_powerAccum = 0;
    SUFLOAT  m_powerError = 0;
    SUSCOUNT m_hangCounter = 0;
    SUSCOUNT m_maxSamples = 0;
    SUSCOUNT m_hangLength = 0;
    SUSCOUNT m_powerSamples = 0;
    SUSCOUNT m_totalSamples = 0;
    SUSCOUNT m_uiRefreshSamples = 0;

    // Private methods
    void connectAll();
    void refreshUi();
    void enableAutoSquelch();
    void cancelAutoSquelch();
    void setInspectorClass(std::string const &cls);
    void refreshCaptureInfo();
    void openTimeWindow();
    void transferHistory();
    void loadDemodulators();

    void applySourceInfo(Suscan::AnalyzerSourceInfo const &info);
    void setDemodFrequency(qint64);
    void setBandwidthLimits(unsigned int min, unsigned int max);
    void setBandwidth(unsigned int freq);
    void setPrecise(bool precise);
    void setState(enum State state);

    void startRawCapture();
    void stopRawCapture();

    void resetRawInspector(qreal sampleRate);
    void feedRawInspector(const SUCOMPLEX *data, size_t size);

    unsigned int getBandwidth() const;
    std::string getInspectorClass() const;
    bool getPrecise() const;
    enum State getState() const;
    void refreshInspectorCombo();

    void openInspector(
        const char *inspFactory,
        const char *inspClass,
        bool);

  public:
    explicit InspToolWidget(
        InspToolWidgetFactory *factory,
        UIMediator *mediator,
        QWidget *parent = nullptr);

    ~InspToolWidget() override;

    // Overriden methods
    void setState(int, Suscan::Analyzer *) override;
    void setProfile(Suscan::Source::Config &) override;
    void setColorConfig(ColorConfig const &) override;
    Suscan::Serializable *allocConfig() override;
    void applyConfig() override;
    bool event(QEvent *) override;

  public slots:
    // UI slots
    void onInspClassChanged();
    void onOpenInspector();
    void onBandwidthChanged(double);
    void onPreciseChanged();
    void onPressHold();
    void onReleaseHold();


    void onPressAutoSquelch();
    void onReleaseAutoSquelch();
    void onToggleAutoSquelch();

    void onTimeWindowConfigChanged();
    void onTriggerSNRChanged(double val);

    // Main UI slots
    void onSpectrumBandwidthChanged();
    void onSpectrumFrequencyChanged();

    // Request tracker slots
    void onOpened(Suscan::AnalyzerRequest const &);
    void onCancelled(Suscan::AnalyzerRequest const &);
    void onError(Suscan::AnalyzerRequest const &, std::string const &);

    // Analyzer slots
    void onSourceInfoMessage(Suscan::SourceInfoMessage const &);
    void onInspectorMessage(Suscan::InspectorMessage const &);
    void onInspectorSamples(Suscan::SamplesMessage const &);

    void onOpenASK();
    void onOpenFSK();
    void onOpenPSK();
    void onOpenRMS();
  };
}

#endif // InspToolWidget_H
