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

  class PanoramicDialog : public QDialog
  {
      Q_OBJECT

      bool running = false;
      std::vector<DeviceGain *> gainControls;
      std::vector<Palette> palettes;
      std::map<std::string, Suscan::Source::Device> deviceMap;

      SavedSpectrum saved;

      quint64 freqStart = 0;
      quint64 freqEnd = 0;
      qint64 currBw = 0;
      qint64 demodFreq = 0;
      quint64 frames = 0;
      quint64 minBwForZoom = 0;
      QString paletteGradient = "Suscan";

      bool adjustingRange = false;
      bool fixedFreqMode = false;

      void connectAll(void);
      void refreshUi(void);
      void redrawMeasures(void);

      DeviceGain *lookupGain(std::string const &name);
      void clearGains(void);
      void refreshGains(Suscan::Source::Device &device);
      void deserializePalettes(void);

      void setRanges(Suscan::Source::Device const &);
      void setWfRange(qint64 min, qint64 max);
      void adjustRanges(void);

    public:
      explicit PanoramicDialog(QWidget *parent = nullptr);
      ~PanoramicDialog();

      void feed(
          quint64 freqStart,
          quint64 freqEnd,
          float *data,
          size_t size);

      SUFREQ getMinFreq(void) const;
      SUFREQ getMaxFreq(void) const;
      SUFREQ getLnbOffset(void) const;

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
      QString getStrategy(void) const;
      QString getPartitioning(void) const;
      float getGain(QString const &) const;

    signals:
      void detailChanged(quint64 freqMin, quint64 freqMax, bool noHop);
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
      void onNewZoomLevel(void);
      void onNewOffset(void);
      void onNewBandwidth(int, int);
      void onNewCenterFreq(qint64);
      void onPaletteChanged(int);
      void onStrategyChanged(QString);
      void onLnbOffsetChanged(void);
      void onExport(void);

    private:
      Ui::PanoramicDialog *ui;
  };
}

#endif // PANORAMICDIALOG_H
