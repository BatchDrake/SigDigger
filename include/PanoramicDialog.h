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
#include <AbstractWaterfall.h>
#include <GuiConfig.h>

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
    Suscan::Object &&serialize() override;
  };

  class PanoramicDialog : public QDialog, public PersistentObject
  {
      Q_OBJECT

      PanoramicDialogConfig *m_dialogConfig = nullptr;
      bool m_running = false;
      QWidget *m_noGainLabel = nullptr;
      std::vector<DeviceGain *> m_gainControls;
      std::map<std::string, Suscan::Source::Device> m_deviceMap;
      std::vector<FrequencyAllocationTable *> m_FATs;

      QString m_bannedDevice;

      SavedSpectrum m_saved;

      qint64 m_freqStart = 0;
      qint64 m_freqEnd = 0;
      qint64 m_currBw = 0;
      qint64 m_demodFreq = 0;
      quint64 m_frames = 0;
      quint64 m_minBwForZoom = 0;
      QString m_paletteGradient = "Suscan";
      std::string m_currentFAT = "";

      bool m_fixedFreqMode = false;
      Ui::PanoramicDialog *m_ui = nullptr;
      AbstractWaterfall *m_waterfall = nullptr;
      ColorConfig m_colorConfig;

      void connectAll();
      void connectWaterfall();
      void refreshUi();
      void redrawMeasures();

      DeviceGain *lookupGain(std::string const &name);
      void clearGains();
      void refreshGains(Suscan::Source::Device &device);
      void deserializeFATs();
      void setRanges(Suscan::Source::Device const &);
      void adjustRanges();

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

      void getZoomRange(qint64 &min, qint64 &max, bool &noHop) const;
      SUFREQ getMinFreq() const;
      SUFREQ getMaxFreq() const;
      SUFREQ getLnbOffset() const;
      SUFLOAT getPreferredSampleRate() const;

      void setColors(ColorConfig const &config);
      void setPaletteGradient(QString const &gradient);
      void populateDeviceCombo();
      unsigned int getRttMs() const;
      float getRelBw() const;
      void setRunning(bool);
      void run();
      void setMinBwForZoom(quint64 bw);
      bool invalidRange() const;
      bool getSelectedDevice(Suscan::Source::Device &) const;
      QString getAntenna() const;
      QString getStrategy() const;
      QString getPartitioning() const;
      float getGain(QString const &) const;
      void setBannedDevice(QString const &);
      void saveConfig();
      void setGuiConfig(GuiConfig const &cfg);

      // Overriden methods
      Suscan::Serializable *allocConfig() override;
      void applyConfig() override;

    signals:
      void detailChanged(qint64 freqMin, qint64 freqMax, bool noHop);
      void start();
      void stop();
      void reset();
      void gainChanged(QString, float);
      void strategyChanged(QString);
      void partitioningChanged(QString);
      void frameSkipChanged();
      void relBandwidthChanged();

    public slots:
      void onToggleScan();
      void onDeviceChanged();
      void onFullRangeChanged();
      void onFreqRangeChanged();
      void onRangeChanged(float, float);
      void onNewZoomLevel(float);
      void onNewOffset();
      void onNewBandwidth(int, int);
      void onBandPlanChanged(int);
      void onNewFftCenterFreq(qint64);
      void onPaletteChanged(int);
      void onStrategyChanged(int);
      void onLnbOffsetChanged();
      void onExport();
      void onGainChanged(QString name, float val);
      void onSampleRateSpinChanged();
      void onPartitioningChanged(int);
  };
}

#endif // PANORAMICDIALOG_H
