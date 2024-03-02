//
//    PanoramicDialog.h: Description
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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
#ifndef PANORAMICDIALOG_H
#define PANORAMICDIALOG_H

#include <QDialog>
#include <map>
#include <Suscan/Source.h>
#include <PersistentWidget.h>
#include "ColorConfig.h"
#include "ui_PanoramicDialog.h"
#include "DeviceGain.h"
#include "Palette.h"

namespace Ui {
  class PanoramicDialog;
}

namespace SigDigger {
  struct SavedSpectrum {
    std::vector<float> data;
    qint64 start;
    qint64 end;

    void set(qint64 start, qint64 end, const float *data, size_t size);
    bool exportToFile(QString const &path);
  };

  class PanoramicDialogConfig : public Suscan::Serializable {
  public:
    bool fullRange = true;
    SUFREQ rangeMin;
    SUFREQ rangeMax;
    SUFLOAT panRangeMin = -90;
    SUFLOAT panRangeMax = 0;
    SUFREQ lnbFreq;
    int sampRate = 20000000;
    std::string device;
    std::string antenna;
    std::string strategy;
    std::string partitioning;
    std::string palette = "Turbo (Gqrx)";

    std::map<std::string, float> gains;
    bool hasGain(std::string const &dev, std::string const &name) const;
    SUFLOAT getGain(std::string const &dev, std::string const &name) const;
    void setGain(std::string const &dev, std::string const &name, SUFLOAT val);

    // Overriden methods
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class PanoramicDialog : public QDialog, public PersistentObject
  {
      Q_OBJECT

      PanoramicDialogConfig *dialogConfig = nullptr;
      bool running = false;
      QWidget *noGainLabel = nullptr;
      std::vector<DeviceGain *> gainControls;
      std::map<std::string, Suscan::Source::Device> deviceMap;
      std::vector<FrequencyAllocationTable *> FATs;

      QString bannedDevice;

      SavedSpectrum saved;

      qint64 freqStart = 0;
      qint64 freqEnd = 0;
      qint64 currBw = 0;
      qint64 demodFreq = 0;
      quint64 frames = 0;
      quint64 minBwForZoom = 0;
      QString paletteGradient = "Suscan";
      std::string currentFAT = "";

      bool fixedFreqMode = false;

      void connectAll(void);
      void refreshUi(void);
      void redrawMeasures(void);

      DeviceGain *lookupGain(std::string const &name);
      void clearGains(void);
      void refreshGains(Suscan::Source::Device &device);
      void deserializeFATs(void);
      void setRanges(Suscan::Source::Device const &);
      void adjustRanges(void);

      static FrequencyBand deserializeFrequencyBand(Suscan::Object const &);
      static int getFrequencyUnits(qint64);
      static unsigned int preferredRttMs(Suscan::Source::Device const &dev);

    public:
      explicit PanoramicDialog(QWidget *parent = nullptr);
      ~PanoramicDialog() override;

      void feed(
          qint64 freqStart,
          qint64 freqEnd,
          float *data,
          size_t size);

      SUFREQ getMinFreq(void) const;
      SUFREQ getMaxFreq(void) const;
      SUFREQ getLnbOffset(void) const;
      SUFLOAT getPreferredSampleRate(void) const;

      void setColors(ColorConfig const &config);
      void setPaletteGradient(QString const &gradient);
      void populateDeviceCombo(void);
      unsigned int getRttMs(void) const;
      float getRelBw(void) const;
      void setRunning(bool);
      void run(void);
      void setMinBwForZoom(quint64 bw);
      bool invalidRange(void) const;
      bool getSelectedDevice(Suscan::Source::Device &) const;
      QString getAntenna(void) const;
      QString getStrategy(void) const;
      QString getPartitioning(void) const;
      float getGain(QString const &) const;
      void setBannedDevice(QString const &);
      void saveConfig(void);

      // Overriden methods
      Suscan::Serializable *allocConfig(void) override;
      void applyConfig(void) override;

    signals:
      void detailChanged(qint64 freqMin, qint64 freqMax, bool noHop);
      void start(void);
      void stop(void);
      void reset(void);
      void gainChanged(QString, float);
      void strategyChanged(QString);
      void partitioningChanged(QString);
      void frameSkipChanged(void);
      void relBandwidthChanged(void);

    public slots:
      void onToggleScan(void);
      void onDeviceChanged(void);
      void onFullRangeChanged(void);
      void onFreqRangeChanged(void);
      void onRangeChanged(float, float);
      void onNewZoomLevel(float);
      void onNewOffset(void);
      void onNewBandwidth(int, int);
      void onBandPlanChanged(int);
      void onNewFftCenterFreq(qint64);
      void onPaletteChanged(int);
      void onStrategyChanged(int);
      void onLnbOffsetChanged(void);
      void onExport(void);
      void onGainChanged(QString name, float val);
      void onSampleRateSpinChanged(void);
      void onPartitioningChanged(int);

    private:
      Ui::PanoramicDialog *ui;
  };
}

#endif // PANORAMICDIALOG_H
