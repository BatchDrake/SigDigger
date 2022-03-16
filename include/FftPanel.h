//
//    FftPanel.h: Dockable FFT panel
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
#ifndef FFTPANEL_H
#define FFTPANEL_H

#include <PersistentWidget.h>
#include <Suscan/AnalyzerParams.h>
#include <QListWidgetItem>
#include <Suscan/Library.h>
#include <Suscan/Analyzer.h>

#include "Palette.h"

namespace Ui {
  class FftPanel;
}

namespace SigDigger {
  class FftPanelConfig : public Suscan::Serializable {
  public:
    bool collapsed = true;
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

    // Overriden methods
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class FftPanel : public PersistentWidget
  {
    Q_OBJECT

    // Convenience pointer
    FftPanelConfig *panelConfig = nullptr;

    // UI Objects
    Ui::FftPanel *ui = nullptr;

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
    void updateRefreshRates(void);
    void updateFftSizes(void);
    void updateTimeSpans(void);
    void connectAll(void);
    void populateUnits(void);
    void updateRbw(void);

    float zeroPointToDb(void) const
    {
      return this->getZeroPoint() * this->currentUnit.dBPerUnit;
    }

    float dbToZeroPoint(float dB) const
    {
      return dB / this->currentUnit.dBPerUnit;
    }

  public:
    explicit FftPanel(QWidget *parent = nullptr);
    ~FftPanel() override;

    void refreshPalettes(void);

    // Getters
    const QColor *getPaletteGradient(void) const;
    std::string getPalette(void) const;
    float getPandRangeMin(void) const;
    float getPandRangeMax(void) const;
    float getWfRangeMin(void) const;
    float getWfRangeMax(void) const;
    float getAveraging(void) const;
    float getPanWfRatio(void) const;
    unsigned int getFreqZoom(void) const;
    unsigned int getFftSize(void) const;
    unsigned int getTimeSpan(void) const;
    unsigned int getRefreshRate(void) const;
    bool getPeakHold(void) const;
    bool getPeakDetect(void) const;
    bool getRangeLock(void) const;
    bool getTimeStamps(void) const;
    bool getBookmarks(void) const;
    bool getFilled(void) const;

    QString getUnitName(void) const;
    float getZeroPoint(void) const;
    float getGain(void) const;
    float getCompleteZeroPoint(void) const;
    float getdBPerUnit(void) const;

    enum Suscan::AnalyzerParams::WindowFunction getWindowFunction(void) const;

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

    // Overriden methods
    Suscan::Serializable *allocConfig(void) override;
    void applyConfig(void) override;
    bool event(QEvent *) override;

  public slots:
    void onPandRangeChanged(int min, int max);
    void onWfRangeChanged(int min, int max);
    void onAveragingChanged(int val);
    void onAspectRatioChanged(int val);
    void onPaletteChanged(int);
    void onFreqZoomChanged(int);
    void onFftSizeChanged(void);
    void onTimeSpanChanged(void);
    void onRefreshRateChanged(void);
    void onRangeLockChanged(void);
    void onPeakChanged(void);
    void onFilledChanged(void);
    void onWindowFunctionChanged(void);
    void onTimeStampsChanged(void);
    void onBookmarksChanged(void);

    // Unit handling slots
    void onUnitChanged(void);
    void onZeroPointChanged(void);
    void onGainChanged(void);

  signals:
    void paletteChanged(void);
    void rangesChanged(void);
    void averagerChanged(void);
    void fftSizeChanged(void);
    void windowFunctionChanged(void);
    void refreshRateChanged(void);
    void timeSpanChanged(void);
    void timeStampsChanged(void);
    void bookmarksChanged(void);
    void unitChanged(QString, float, float);
    void zeroPointChanged(float);
    void gainChanged(float);
  };
}

#endif // FFTPANEL_H
