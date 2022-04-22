//
//    FFTWidget.h: description
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
#ifndef FFTWIDGET_H
#define FFTWIDGET_H

#include "ToolWidgetFactory.h"
#include <Suscan/Library.h>
#include <Suscan/Analyzer.h>

namespace Ui {
  class FftPanel;
}

namespace Ui {
  class SourceWidget;
}

namespace SigDigger {
  class FFTWidgetFactory;
  class Palette;
  class UIMediator;
  class MainSpectrum;

  struct FFTWidgetConfig : public Suscan::Serializable {
    bool collapsed = false;
    float averaging = 1;
    float panWfRatio = 0.3f;
    bool peakDetect = false;
    bool peakHold = false;
    bool filled = false;

    float panRangeMin = -60;
    float panRangeMax = -10;

    float wfRangeMin = -60;
    float wfRangeMax = -10;

    unsigned int timeSpan = 0;

    bool rangeLock = true;
    bool timeStamps = false;
    bool bookmarks = true;

    std::string palette = "Magma (Feely)";

    std::string unitName;
    float zeroPoint;
    float gain;

    int zoom = 1;

    // Overriden methods·
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize() override;
  };


  class FFTWidget : public ToolWidget
  {
    Q_OBJECT

    // Convenience pointer
    FFTWidgetConfig *panelConfig = nullptr;

    // UI Objects
    Ui::FftPanel *ui = nullptr;
    MainSpectrum *m_spectrum = nullptr;
    UIMediator   *m_mediator = nullptr;
    Suscan::Analyzer *m_analyzer = nullptr;

    // UI Data
    unsigned int rate = 0;
    unsigned int defaultFftSize = 0;
    unsigned int fftSize = 0;
    unsigned int refreshRate = 0;
    unsigned int defaultRefreshRate = 0;

    std::vector<unsigned int> sizes;
    std::vector<unsigned int> refreshRates;
    std::vector<unsigned int> timeSpans;

    Suscan::SpectrumUnit currentUnit;

    const Palette *selected = nullptr;

    // Private methods
    void addFftSize(unsigned int sz);
    void addTimeSpan(unsigned int timeSpan);
    void addRefreshRate(unsigned int rate);
    void updateRefreshRates();
    void updateFftSizes();
    void updateTimeSpans();
    void connectAll();
    void populateUnits();
    void updateRbw();

    void refreshPalettes();

    // Getters
    const QColor *getPaletteGradient() const;
    std::string getPalette() const;
    float getPandRangeMin() const;
    float getPandRangeMax() const;
    float getWfRangeMin() const;
    float getWfRangeMax() const;
    float getAveraging() const;
    float getPanWfRatio() const;
    unsigned int getFreqZoom() const;
    unsigned int getFftSize() const;
    unsigned int getTimeSpan() const;
    unsigned int getRefreshRate() const;
    bool getPeakHold() const;
    bool getPeakDetect() const;
    bool getRangeLock() const;
    bool getTimeStamps() const;
    bool getBookmarks() const;
    bool getFilled() const;

    QString getUnitName() const;
    float getZeroPoint() const;
    float getGain() const;
    float getCompleteZeroPoint() const;
    float getdBPerUnit() const;

    enum Suscan::AnalyzerParams::WindowFunction getWindowFunction() const;

    void applySourceInfo(Suscan::AnalyzerSourceInfo const &info);

    // Setters
    void setPeakHold(bool);
    void setPeakDetect(bool);
    void setRangeLock(bool);
    bool setPalette(std::string const &);
    void setPandRangeMin(float);
    void setPandRangeMax(float);
    void setWfRangeMin(float);
    void setWfRangeMax(float);
    void setAveraging(float);
    void setPanWfRatio(float);
    void setFreqZoom(int);
    void setDefaultFftSize(unsigned int);
    void setFftSize(unsigned int);
    void setDefaultRefreshRate(unsigned int);
    void setRefreshRate(unsigned int);
    void setTimeSpan(unsigned int);
    void setTimeStamps(bool);
    void setBookmarks(bool);

    bool setUnitName(QString);
    void setZeroPoint(float);
    void setGain(float);

    void setFilled(bool);
    void setSampleRate(unsigned int);
    void setWindowFunction(enum Suscan::AnalyzerParams::WindowFunction func);

    // Refresh logic
    void refreshSpectrumSettings();
    void refreshSpectrumScaleSettings();
    void refreshSpectrumAxesSettings();
    void refreshSpectrumRepresentationSettings();
    void refreshSpectrumWaterfallSettings();
    void refreshSpectrumUnits();
    void refreshParamControls(Suscan::AnalyzerParams const &params);
    void updateAnalyzerParams();

    float
    zeroPointToDb() const
    {
      return this->getZeroPoint() * this->currentUnit.dBPerUnit;
    }

    float
    dbToZeroPoint(float dB) const
    {
      return dB / this->currentUnit.dBPerUnit;
    }

  public:
    FFTWidget(FFTWidgetFactory *, UIMediator *, QWidget *parent = nullptr);
    ~FFTWidget() override;

    // Configuration methods
    Suscan::Serializable *allocConfig() override;
    void applyConfig() override;
    bool event(QEvent *) override;

    // Overriden methods
    void setState(int, Suscan::Analyzer *) override;
    void setProfile(Suscan::Source::Config &) override;

  public slots:
    void onPandRangeChanged(int min, int max);
    void onWfRangeChanged(int min, int max);
    void onAveragingChanged(int val);
    void onAspectRatioChanged(int val);
    void onPaletteChanged(int);
    void onFreqZoomChanged(int);
    void onFftSizeChanged();
    void onTimeSpanChanged();
    void onRefreshRateChanged();
    void onRangeLockChanged();
    void onPeakChanged();
    void onFilledChanged();
    void onWindowFunctionChanged();
    void onTimeStampsChanged();
    void onBookmarksChanged();

    // Unit handling slots
    void onUnitChanged();
    void onZeroPointChanged();
    void onGainChanged();

    // Analyzer slots
    void onAnalyzerParams(const Suscan::AnalyzerParams &params);
    void onSourceInfoMessage(Suscan::SourceInfoMessage const &msg);

    // Spectrum slots
    void onRangeChanged(float min, float max);
    void onZoomChanged(float level);
  };
}

#endif // FFTWIDGET_H
