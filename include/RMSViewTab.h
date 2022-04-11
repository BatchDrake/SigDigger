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

      int     accum_ctr = 0;
      SUFLOAT energy_accum = 0;

      void refreshSampleRate();
      void connectAll();
      void integrateMeasure(qreal timestamp, SUFLOAT mag);
      bool parseLine(void);
      void processSocketData(void);
      bool saveToMatlab(QString const &);
      void disconnectSocket(void);
      void fitVertical(void);

    public:
      explicit RMSViewTab(QWidget *parent, QTcpSocket *socket);
      ~RMSViewTab();

    private:
      Ui::RMSViewTab *ui;

    signals:
      void titleChanged(QString);

    public slots:
      void onTimeout();
      void onStop(void);
      void onSave(void);
      void onToggleModes(void);
      void onResetZoom(void);
      void onSocketDisconnected(void);
      void onValueChanged(int);
  };

}

#endif // RMSVIEWTAB_H
