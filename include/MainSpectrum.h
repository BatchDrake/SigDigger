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
#include <GuiConfig.h>
#include <WFHelpers.h>
#include <Palette.h>
#include <QElapsedTimer>
#include <QToolBar>

#define SIGDIGGER_MAIN_SPECTRUM_GRACE_PERIOD_MS 1000

#define MS_VAR_FFT_SIZE  "%FFT_SIZE%"
#define MS_VAR_RBW       "%RBW%"
#define MS_VAR_SAMP_RATE "%SAMP_RATE%"
#define MS_VAR_DATE      "%DATE%"
#define MS_VAR_TIME      "%TIME%"
#define MS_VAR_DATETIME  "%DATETIME%"
#define MS_VAR_CITY      "%CITY%"
#define MS_VAR_LAT       "%LAT%"
#define MS_VAR_LON       "%LON%"
#define MS_VAR_LOCATOR   "%LOCATOR%"


class QTimeSlider;

namespace Ui {
  class MainSpectrum;
}

class Waterfall;
class GLWaterfall;

// Does it make sense to turn this into a PersistentWidget, anyways?
namespace SigDigger {
  class SuscanBookmarkSource;
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
    SuscanBookmarkSource *bookmarkSource = nullptr;
    Waterfall   *wf   = nullptr;
    GLWaterfall *glWf = nullptr;
    ColorConfig  lastColorConfig;
    QString      infoTextTemplate;
    QString      infoText;

    struct timeval lastTimeStamp;

    // UI State
    CaptureMode mode = UNAVAILABLE;
    Skewness filterSkewness = SYMMETRIC;
    bool throttling = false;
    bool resAdjusted = false;
    bool noLimits = false;
    qint64 minFreq = 0;
    qint64 maxFreq = 6000000000;
    QElapsedTimer lastFreqUpdate;
    qint64 freqGracePeriod =
        SIGDIGGER_MAIN_SPECTRUM_GRACE_PERIOD_MS;
    int maxToolWidth = 0;

    // Cached members (for UI update, etc)
    unsigned int cachedRate = 0;
    unsigned int bandwidth = 0;
    unsigned int cachedFftSize = 0;
    unsigned int zoom = 1;

    // Private methods
    void connectAll();
    void connectWf();
    void connectGLWf();
    void refreshUi();
    void updateLimits();
    void refreshFFTProperties();
    void refreshInfoText();

    // Static members
    static FrequencyBand deserializeFrequencyBand(Suscan::Object const &);

  public:
    explicit MainSpectrum(QWidget *parent = nullptr);
    ~MainSpectrum();

    // Actions
    void feed(
        float *data,
        int size,
        struct timeval const &tv,
        bool looped = false);

    void deserializeFATs();

    // Named channel API
    NamedChannelSetIterator addChannel(
        QString name,
        qint64 frequency,
        qint32 fMin,
        qint32 fMax,
        QColor boxColor,
        QColor markerColor,
        QColor cutOffColor);

    void removeChannel(NamedChannelSetIterator);
    void refreshChannel(NamedChannelSetIterator &);
    NamedChannelSetIterator findChannel(qint64 freq);

    NamedChannelSetIterator channelCBegin() const;
    NamedChannelSetIterator channelCEnd() const;


    // Setters
    void setTimeStamp(struct timeval const &);
    void setThrottling(bool);
    void setFrequencyLimits(qint64 min, qint64 max);
    void setCaptureMode(CaptureMode mode);
    void setCenterFreq(qint64 freq);
    void setLoFreq(qint64 loFreq);
    void setLnbFreq(qint64 lnbFreq);
    void setFreqs(qint64 freq, qint64 lnbFreq, bool silent = false);
    void setFilterBandwidth(unsigned int bw);

    void setPaletteGradient(const QColor *color);
    void setPandapterRange(float min, float max);
    void setWfRange(float min, float max);
    void setPanWfRatio(float ratio);
    void setColorConfig(ColorConfig const &cfg);
    void setGuiConfig(GuiConfig const &cfg);
    void setPeakHold(bool);
    void setPeakDetect(bool);
    void setExpectedRate(int);
    void setFilled(bool);
    void setTimeStamps(bool);
    void setBookmarks(bool);
    void setZoom(unsigned int zoom);
    void setSampleRate(unsigned int rate);
    void setTimeSpan(quint64 ms);
    void setGracePeriod(qint64 ms);

    void setShowFATs(bool);
    void pushFAT(FrequencyAllocationTable *);
    void removeFAT(QString const &name);
    void notifyHalt();
    void setFilterSkewness(enum Skewness); // TODO: Return *actual* bw
    void updateOverlay();

    void setGain(float);
    void setZeroPoint(float);
    void setUnits(QString const &, float, float);

    void addToolWidget(QWidget *widget, QString const &);
    void setSidePanelWidth(int);
    void setSidePanelRatio(qreal);
    void setLocked(bool);
    void setTimeStampsUTC(bool);

    // Getters
    bool getThrottling() const;
    CaptureMode getCaptureMode() const;
    qint64 getCenterFreq() const;
    qint64 getLoFreq() const;
    qint64 getLnbFreq() const;
    unsigned int getBandwidth() const;
    unsigned int getZoom() const;
    Skewness getFilterSkewness() const;
    FrequencyAllocationTable *getFAT(QString const &) const;
    void adjustSizes();
    int sidePanelWidth() const;
    qreal sidePanelRatio() const;

    bool canChangeFrequency(qint64) const;
    static int getFrequencyUnits(qint64 frew);

    qint32 computeLowCutFreq(int bw) const;
    qint32 computeHighCutFreq(int bw) const;

  signals:
    void bandwidthChanged();
    void frequencyChanged(qint64);
    void lnbFrequencyChanged(qint64);
    void loChanged(qint64);
    void rangeChanged(float, float);
    void zoomChanged(float);
    void newBandPlan(QString);
    void modulationChanged(QString);
    void seek(struct timeval);

  public slots:
    void onRangeChanged(float, float);
    void onWfBandwidthChanged(int, int);
    void onWfLoChanged();
    void onFrequencyChanged();
    void onNewCenterFreq(qint64);
    void onLoChanged();
    void onNewZoomLevel(float);
    void onNewModulation(QString);
    void onLnbFrequencyChanged();
    void onLockStateChanged();
  };
}

#endif // MAINSPECTRUM_H
