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
#ifndef MAINSPECTRUM_H
#define MAINSPECTRUM_H

#include <PersistentWidget.h>
#include <Suscan/Messages/PSDMessage.h>
#include <ColorConfig.h>
#include <Waterfall.h>
#include <Palette.h>

namespace Ui {
  class MainSpectrum;
}

// Does it make sense to turn this into a PersistentWidget, anyways?
namespace SigDigger {
  class MainSpectrum : public QWidget
  {
    Q_OBJECT

  public:
    enum CaptureMode {
      UNAVAILABLE,
      CAPTURE,
      REPLAY
    };

    enum Skewness {
      SYMMETRIC,
      UPPER,
      LOWER
    };

  private:
    // UI Objects
    Ui::MainSpectrum *ui = nullptr;
    std::vector<FrequencyAllocationTable *> FATs;

    // UI State
    CaptureMode mode = UNAVAILABLE;
    Skewness filterSkewness = SYMMETRIC;
    bool throttling = false;
    qint64 minFreq = 0;
    qint64 maxFreq = 6000000000;

    // Cached members (for UI update, etc)
    unsigned int cachedRate = 0;
    unsigned int bandwidth = 0;
    unsigned int zoom = 1;

    // Private methods
    void connectAll(void);
    void refreshUi(void);
    void updateLimits(void);

    // Static members
    static FrequencyBand deserializeFrequencyBand(Suscan::Object const &);

  public:
    explicit MainSpectrum(QWidget *parent = nullptr);
    ~MainSpectrum();

    // Actions
    void feed(float *data, int size);
    void deserializeFATs(void);

    // Setters
    void setThrottling(bool);
    void setFrequencyLimits(qint64 min, qint64 max);
    void setCaptureMode(CaptureMode mode);
    void setCenterFreq(qint64 freq);
    void setLoFreq(qint64 loFreq);
    void setLnbFreq(qint64 lnbFreq);
    void setFreqs(qint64 freq, qint64 lnbFreq);
    void setFilterBandwidth(unsigned int bw);

    void setPaletteGradient(const QColor *color);
    void setPandapterRange(float min, float max);
    void setWfRange(float min, float max);
    void setPanWfRatio(float ratio);
    void setColorConfig(ColorConfig const &cfg);
    void setPeakHold(bool);
    void setPeakDetect(bool);
    void setExpectedRate(int);
    void setTimeStamps(bool);

    void setZoom(unsigned int zoom);
    void setSampleRate(unsigned int rate);
    void setTimeSpan(quint64 ms);

    void setShowFATs(bool);
    void pushFAT(FrequencyAllocationTable *);
    void removeFAT(QString const &name);
    void notifyHalt(void);
    void setFilterSkewness(enum Skewness); // TODO: Return *actual* bw

    // Getters
    bool getThrottling(void) const;
    CaptureMode getCaptureMode(void) const;
    qint64 getCenterFreq(void) const;
    qint64 getLoFreq(void) const;
    qint64 getLnbFreq(void) const;
    unsigned int getBandwidth(void) const;
    unsigned int getZoom(void) const;
    FrequencyAllocationTable *getFAT(QString const &) const;

    static int getFrequencyUnits(qint64 frew);


  signals:
    void bandwidthChanged(void);
    void frequencyChanged(qint64);
    void lnbFrequencyChanged(qint64);
    void loChanged(qint64);
    void rangeChanged(float, float);
    void zoomChanged(float);
    void newBandPlan(QString);

  public slots:
    void onRangeChanged(float, float);
    void onWfBandwidthChanged(int, int);
    void onWfLoChanged(void);
    void onFrequencyChanged(void);
    void onNewCenterFreq(qint64);
    void onLoChanged(void);
    void onNewZoomLevel(float);
    void onLnbFrequencyChanged(void);
  };
}

#endif // MAINSPECTRUM_H
