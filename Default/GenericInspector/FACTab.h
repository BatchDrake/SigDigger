//
//    FACTab.h: Fast Autocorrelation Tab
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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
#ifndef FACTAB_H
#define FACTAB_H

#include <QWidget>
#include <util/compat-time.h>

class ThrottleControl;
#include <sigutils/types.h>
#include "ColorConfig.h"

namespace Ui {
  class FACTab;
}

namespace SigDigger {
  class FACTab : public QWidget
  {
    Q_OBJECT

    qreal fs = 1;

    struct timeval lastRefresh = {0, 0};
    SU_FFTW(_plan) direct = nullptr;
    SU_FFTW(_plan) reverse = nullptr;

    std::vector<SUCOMPLEX> buffer;
    std::vector<SUCOMPLEX> fac;
    SUFLOAT alpha;
    SUFLOAT min = INFINITY;
    SUFLOAT max = -INFINITY;

    bool recording = false;
    bool adjustZoom = false;

    unsigned int p = 0;

    void refreshUi(void);
    void connectAll(void);
    void resizeFAC(int);

  public:
    explicit FACTab(QWidget *parent = nullptr);
    ~FACTab();

    void setThrottleControl(ThrottleControl *);
    void setSampleRate(qreal);
    void setColorConfig(ColorConfig const &cfg);

    inline bool
    isRecording(void) const
    {
      return this->recording;
    }

    void feed(const SUCOMPLEX *, unsigned int);

  public slots:
    void onRecord(void);
    void onAdjustAveraging(void);
    void onChangeFACSize(void);
    void onUnitsChanged(void);
    void onChangePeakDetect(void);

  private:
    Ui::FACTab *ui;
  };
}

#endif // FACTAB_H
