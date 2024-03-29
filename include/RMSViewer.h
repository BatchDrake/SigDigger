//
//    RMSViewer.h: TCP server to visualize remote RMS data
//    originating from suscli rms
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
#ifndef RMSVIEWER_H
#define RMSVIEWER_H

#include <QMainWindow>
#include <QTcpServer>
#include <vector>
#include <QAbstractSocket>

namespace Ui {
  class RMSViewer;
}

namespace SigDigger {
  class RMSViewTab;
  class RMSViewerSettingsDialog;

  class RMSViewer : public QMainWindow
  {
      Q_OBJECT

      QTcpServer server;
      RMSViewerSettingsDialog *settingsDialog = nullptr;

      bool     listening  = false;
      QString  listenAddr = "";
      uint16_t listenPort = 0;

      void openNewView(QTcpSocket *);
      void connectAll(void);

      bool haveAddrData(void) const;
      bool openSettings(void);

      bool getListeningState(void) const;
      void setListeningState(bool);

    public:
      explicit RMSViewer(QWidget *parent = nullptr);
      ~RMSViewer();

    private:
      Ui::RMSViewer *ui;

    public slots:
      void onAcceptError(QAbstractSocket::SocketError socketError);
      void onNewConnection(void);

      void onToggleListening(void);
      void onOpenSettings(void);

      void onTitleChanged(QString);
      void onTabCloseRequested(int);
  };
};

#endif // RMSVIEWER_H
