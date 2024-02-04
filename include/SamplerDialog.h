//
//    SamplerDialog.h: Sampler dialog
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
#ifndef SAMPLERDIALOG_H
#define SAMPLERDIALOG_H

#include <QDialog>
#include "WaveSampler.h"
#include "ColorConfig.h"

namespace Ui {
  class SamplerDialog;
}

namespace SigDigger {
  class SamplerDialog : public QDialog
  {
    Q_OBJECT

    Decider            m_decider;
    SamplingProperties m_properties;

    SUFLOAT m_minVal = +INFINITY;
    SUFLOAT maxVal = -INFINITY;

    SUFLOAT m_minAmp = 0;
    SUFLOAT m_maxAmp = 1;

    bool m_scrolling = false;
    bool m_autoScroll = true;

    void connectAll(void);
    void refreshUi(void);

    unsigned int getVScrollPageSize(void) const;
    unsigned int getHScrollOffset(void) const;
    void refreshHScrollBar(void) const;
    void refreshVScrollBar(void) const;

  public:
    explicit SamplerDialog(QWidget *parent = nullptr);
    ~SamplerDialog();

    void setProperties(SamplingProperties const &prop);
    void setAmplitudeLimits(SUFLOAT min, SUFLOAT max);
    void reset(void);
    void feedSet(WaveSampleSet const &set);
    void setColorConfig(ColorConfig const &cfg);
    void closeEvent(QCloseEvent *);
    void fitToSamples(void);

    WaveSampler *makeSampler(void);

  signals:
    void resample(void);
    void stopTask(void);

  public slots:
    void onClose(void);
    void onBpsChanged(void);
    void onZoomChanged(void);
    void onRowSizeChanged(void);
    void onHScroll(int);
    void onVScroll(int);
    void onOffsetChanged(unsigned int);
    void onHOffsetChanged(int);
    void onStrideChanged(unsigned int);
    void onSymViewZoomChanged(unsigned int);
    void onHoverSymbol(unsigned int);
    void onSaveSymView(void);

  private:
    Ui::SamplerDialog *ui;


  };
}

#endif // SAMPLERDIALOG_H
