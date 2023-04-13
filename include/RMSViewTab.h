//
//    RMSViewTab.h: Description
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
#ifndef RMSVIEWTAB_H
#define RMSVIEWTAB_H

#include <QWidget>
#include <QTimer>
#include <QTcpSocket>
#include <sigutils/types.h>
#include <vector>
#include <ColorConfig.h>

namespace Ui {
  class RMSViewTab;
}

namespace SigDigger {
  class RMSViewTab : public QWidget
  {
      Q_OBJECT

      QTcpSocket *socket = nullptr;
      QTimer timer;
      std::string line;
      std::vector<SUCOMPLEX> data;

      qreal rate = 1;
      qreal first;
      qreal last;

      qreal m_time = 0;

      bool m_running = true;

      bool  m_haveMarker = false;
      qreal m_markerLinear = 0;
      qreal m_markerDb = 0;

      bool  m_haveDeltaMarker = false;
      qreal m_markerDeltaLinear = 0;
      qreal m_markerDeltaDb = 0;

      int     accum_ctr = 0;
      SUFLOAT energy_accum = 0;

      void refreshUi();
      void refreshSampleRate();
      void connectAll();
      void integrateMeasure(qreal timestamp, SUFLOAT mag);
      bool parseLine();
      void processSocketData();
      bool saveToMatlab(QString const &);
      void disconnectSocket();
      void fitVertical();
      void toggleModes(QObject *sender);

      bool userClear(QString const &);
      qreal getCurrentTimeDelta() const;
      bool  intTimeMode() const;

    public:
      void setVerticalLimitsLinear(qreal min, qreal max);
      void setVerticalLimitsDb(qreal min, qreal max);
      qreal getMin() const;
      qreal getMax() const;

      bool isLogScale() const;
      bool isAutoFit() const;
      bool isAutoScroll() const;

      int  getTimeScaleSelection() const;
      void setTimeScaleSelection(int);

      void setLogScale(bool);
      void setAutoFit(bool);
      void setAutoScroll(bool);

      void setIntegrationTimeMode(qreal, qreal);
      void setIntegrationTimeHint(qreal);
      qreal getIntegrationTimeHint() const;

      void setSampleRate(qreal);
      void feed(qreal, qreal);
      void setColorConfig(ColorConfig const &);

      bool running() const;

      explicit RMSViewTab(QWidget *parent, QTcpSocket *socket);
      ~RMSViewTab();

    private:
      Ui::RMSViewTab *ui;

    signals:
      void titleChanged(QString);
      void viewTypeChanged();
      void integrationTimeChanged(qreal);
      void toggleState();

    public slots:
      void onTimeChanged(qreal, qreal);
      void onTimeout();
      void onToggleStartStop();
      void onSave();
      void onToggleModes();
      void onResetZoom();
      void onSocketDisconnected();
      void onValueChanged(int);
      void onPointClicked(qreal, qreal, Qt::KeyboardModifiers);
      void onToolTip(int, int, qreal, qreal);
      void onTimeScaleChanged();
  };

}

#endif // RMSVIEWTAB_H
