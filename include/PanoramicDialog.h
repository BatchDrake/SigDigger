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

namespace Ui {
  class PanoramicDialog;
}

namespace SigDigger {
  class PanoramicDialog : public QDialog
  {
      Q_OBJECT

      bool running = false;
      std::vector<DeviceGain *> gainControls;
      std::map<std::string, Suscan::Source::Device> deviceMap;
      void setRanges(Suscan::Source::Device const &);
      void setWfRange(quint64 min, quint64 max);
      void adjustRanges(void);
      void connectAll(void);

      quint64 freqStart = 0;
      quint64 freqEnd = 0;
      qint64 demodFreq = 0;
      quint64 frames = 0;

      void refreshUi(void);
      void redrawMeasures(void);

      bool adjustingRange = false;

      DeviceGain *lookupGain(std::string const &name);
      void clearGains(void);
      void refreshGains(Suscan::Source::Device &device);

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

      void setColors(ColorConfig const &config);
      void setPaletteGradient(const QColor *table);
      void populateDeviceCombo(void);
      unsigned int getRttMs(void) const;
      float getRelBw(void) const;
      void setRunning(bool);
      void run(void);
      bool invalidRange(void) const;
      bool getSelectedDevice(Suscan::Source::Device &) const;

    signals:
      void detailChanged(quint64 freqMin, quint64 freqMax);
      void start(void);
      void stop(void);
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

    private:
      Ui::PanoramicDialog *ui;
  };
}

#endif // PANORAMICDIALOG_H
