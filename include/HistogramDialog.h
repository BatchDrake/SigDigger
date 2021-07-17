//
//    HistogramDialog.h: Histogram dialog
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
#ifndef HISTOGRAMDIALOG_H
#define HISTOGRAMDIALOG_H

#include <QDialog>
#include "SamplingProperties.h"
#include "Decider.h"
#include "ColorConfig.h"

namespace Ui {
  class HistogramDialog;
}

namespace SigDigger {
  class HistogramDialog : public QDialog
  {
    Q_OBJECT

    Decider dummyDecider;
    SamplingProperties properties;

    SUFLOAT min = INFINITY;
    SUFLOAT max = -INFINITY;

    bool limits = false;
    SUFLOAT selMin;
    SUFLOAT selMax;

    void refreshUi(void);
    void connectAll(void);

  public:
    explicit HistogramDialog(QWidget *parent = nullptr);
    ~HistogramDialog();

    void reset(void);
    void setProperties(SamplingProperties const &);
    void feed(const SUFLOAT *data, unsigned int len);

    void setColorConfig(ColorConfig const &);

    void closeEvent(QCloseEvent *);

  signals:
    void blanked();
    void stopTask();

  public slots:
    void onClose(void);
    void onNewLimits(float, float);
    void onResetLimits(void);

  private:
    Ui::HistogramDialog *ui;

  };
}

#endif // HISTOGRAMDIALOG_H
