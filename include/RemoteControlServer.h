//
//    include/RemoteControlServer.h: description
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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
#ifndef REMOTECONTROLSERVER_H
#define REMOTECONTROLSERVER_H

#include <QObject>
#include <QMap>
#include <list>

class QTcpSocket;
class QTcpServer;

namespace SigDigger{
  struct RemoteControlClient {
    QTcpSocket *socket = nullptr;
    std::list<RemoteControlClient *>::iterator iterator;

    // Context (at the time being: null)
    RemoteControlClient(QTcpSocket *);
    ~RemoteControlClient();
    void process();
    bool tokenize(QString const &command, QStringList &out);
    void write(QString const &);
  };

  class RemoteControlServer : public QObject
  {
    Q_OBJECT

    // Configuration properties
    QString  m_hostAddress = "localhost";
    uint16_t m_listenPort = 1234;
    bool     m_enabled = false;

    // Server objects
    QTcpServer *m_server = nullptr;
    std::list<RemoteControlClient *> m_clientList;
    QMap<QTcpSocket *, RemoteControlClient *> m_socketToClient;

    void connectAll();

    void addConnection(QTcpSocket *);
    void removeConnection(QTcpSocket *);

  public:
    explicit RemoteControlServer(QObject *parent = nullptr);

    bool isEnabled() const;
    void setEnabled(bool);

    void setHost(QString const &);
    void setPort(uint16_t);
    QString getLastError() const;

  public slots:
    void onNewConnection();
    void onDataReady();
    void onDisconnect();
  };
}

#endif // REMOTECONTROLSERVER_H
