//
//    DopplerDialog.h: Doppler effect dialog
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
#ifndef DOPPLERDIALOG_H
#define DOPPLERDIALOG_H

#include <QDialog>
#include <sigutils/types.h>
#include "ColorConfig.h"

namespace Ui {
  class DopplerDialog;
}

namespace SigDigger {
  class DopplerDialog : public QDialog
  {
    Q_OBJECT

    std::vector<SUCOMPLEX> data;
    qreal fs;
    qreal f0;
    qreal vDom;
    qreal sigmaV;
    qreal max = 1;

    bool exportToMatlab(QString const &path);

    void connectAll(void);
    void zoomReset(void);

  public:
    explicit DopplerDialog(QWidget *parent = nullptr);
    ~DopplerDialog();

    void setDominantVelocity(qreal vel);
    void setSigmaV(qreal vel);
    void setCenterFreq(qreal freq);
    void setMax(qreal ax);
    void setVelocityStep(qreal fs);
    void giveSpectrum(std::vector<SUCOMPLEX> &&);
    void setColorConfig(ColorConfig const &);

    void showEvent(QShowEvent *);

  public slots:
    void onSave(void);
    void onZoomReset(void);

  private:
    Ui::DopplerDialog *ui;
  };
}

#endif // DOPPLERDIALOG_H
