//
//    App/RemoteControlServer.cpp: description
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

#include "RemoteControlServer.h"
#include <QTcpSocket>
#include <QTcpServer>
#include <QCommandLineParser>
#include <GlobalProperty.h>
#include <Suscan/Logger.h>
#include <Version.h>
#include <SuWidgetsHelpers.h>

using namespace SigDigger;

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
#  define sliced(...) mid(__VA_ARGS__)
#endif

RemoteControlClient::RemoteControlClient(
    QTcpSocket *socket)
{
  this->socket   = socket;

  SU_INFO("Remote client created\n");
  write(
          "SIGDIGGER REMOTE CONTROL SERVER - VERSION "
        + QString(SIGDIGGER_VERSION_STRING)
        + "\n");
}

void
RemoteControlClient::write(QString const &data)
{
  socket->write(data.toUtf8());
}

bool
RemoteControlClient::tokenize(QString const &command, QStringList &out)
{
  QStringList result;
  int len = SCAST(int, command.size());
  bool qot = false, sqot = false;
  qsizetype argLen;

  for (int i = 0; i < len; i++) {
    int start = i;

    if (command[i] == '\"')
      qot = true;
    else if (command[i] == '\'')
      sqot = true;

    if (qot) {
      ++i;
      ++start;

      while (i < len && command[i] != '\"')
        ++i;

      if (i < len)
        qot = false;

      argLen = i - start;
      ++i;
    } else if (sqot) {
      ++i;
      ++start;

      while (i < len && command[i] != '\'')
        ++i;

      if (i < len)
        sqot = false;
      argLen = i - start;
      ++i;
    } else {
      while(i<len && command[i] != ' ')
        i++;
      argLen = i - start;
    }

    result.append(command.sliced(start, argLen));
  }

  if (qot || sqot)
    return false;

  out.clear();
  out.append(result);

  return true;
}

void
RemoteControlClient::process()
{
  while (socket->canReadLine()) {
    QString line = socket->readLine(1024);

    if (line.size() > 0 && line[line.size() - 1] == '\n') {
      QStringList args;

      if (!tokenize(line.trimmed(), args)) {
        write("Syntax error\n");
      } else if (args.size() > 0) {
        QString command = args[0].toLower();

        if (command == "set") {
          if (args.size() != 3) {
            write(command + ": invalid number of arguments\n");
          } else {
            GlobalProperty *prop = GlobalProperty::lookupProperty(args[1]);
            if (prop == nullptr) {
              write(command + " " + args[1] + ": unknown property\n");
            } else if (!prop->adjustable()) {
              write(command + " " + args[1] + ": property is read-only\n");
            } else {
              prop->setValue(args[2]);
              write(args[1] + " = " + prop->toString() + "\n");
            }
          }
        } else if (command == "get") {
          if (args.size() != 2) {
            write(command + ": invalid number of arguments\n");
          } else {
            GlobalProperty *prop = GlobalProperty::lookupProperty(args[1]);
            if (prop == nullptr) {
              write(command + " " + args[1] + ": unknown property\n");
            } else {
              write(args[1] + " = " + prop->toString() + "\n");
            }
          }
        } else {
          write(command + ": Unrecognized command\n");
        }
      }
    }
  }
}

RemoteControlClient::~RemoteControlClient()
{
  SU_INFO("Remote client left\n");
}

/////////////////////////// Remote Control Server //////////////////////////////
RemoteControlServer::RemoteControlServer(QObject *parent) : QObject(parent)
{
  m_server = new QTcpServer(this);

  connectAll();
}

void
RemoteControlServer::connectAll()
{
  connect(
        m_server,
        SIGNAL(newConnection()),
        this,
        SLOT(onNewConnection()));
}

void
RemoteControlServer::addConnection(QTcpSocket *socket)
{
  RemoteControlClient *client = new RemoteControlClient(socket);

  m_clientList.push_front(client);
  client->iterator = m_clientList.begin();
  m_socketToClient.insert(socket, client);

  connect(
        socket,
        SIGNAL(readyRead()),
        this,
        SLOT(onDataReady()));

  connect(
        socket,
        SIGNAL(disconnected()),
        this,
        SLOT(onDisconnect()));

  connect(
        socket,
        SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
        this,
        SLOT(onDisconnect()));
}

void
RemoteControlServer::removeConnection(QTcpSocket *socket)
{
  if (m_socketToClient.contains(socket)) {
    RemoteControlClient *client = m_socketToClient[socket];

    m_clientList.erase(client->iterator);
    m_socketToClient.remove(socket);

    delete client;
  }
}


bool
RemoteControlServer::isEnabled() const
{
  return m_enabled;
}

void
RemoteControlServer::setEnabled(bool enabled)
{
  if (m_enabled != enabled) {
    if (!enabled) {
      m_server->close();
    } else {
      enabled = m_server->listen(QHostAddress(m_hostAddress), m_listenPort);

      if (enabled) {
        SU_INFO(
              "Remote control enabled: listening on port %d...\n",
              m_listenPort);
      } else {
        QString reason = m_server->errorString();

        SU_ERROR(
              "Failed to start remote control server: %s\n",
              reason.toStdString().c_str());
      }
    }

    m_enabled = enabled;
  }
}

QString
RemoteControlServer::getLastError() const
{
  return m_server->errorString();
}

void
RemoteControlServer::setHost(QString const &host)
{
  m_hostAddress = host;
}

void
RemoteControlServer::setPort(uint16_t port)
{
  m_listenPort = port;
}

////////////////////////////////// Slots ///////////////////////////////////////

void
RemoteControlServer::onNewConnection()
{
  while (m_server->hasPendingConnections()) {
    QTcpSocket *socket = m_server->nextPendingConnection();

    addConnection(socket);
  }
}

void
RemoteControlServer::onDataReady()
{
  QTcpSocket *socket = static_cast<QTcpSocket *>(QObject::sender());

  if (m_socketToClient.contains(socket)) {
    RemoteControlClient *client = m_socketToClient[socket];

    client->process();
  }
}

void
RemoteControlServer::onDisconnect()
{
  QTcpSocket *socket = static_cast<QTcpSocket *>(QObject::sender());

  removeConnection(socket);
}
