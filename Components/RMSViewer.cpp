//
//    RMSViewer.cpp: Description
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

#include <RMSViewer.h>
#include <RMSViewTab.h>
#include <QMessageBox>

#include "ui_RMSViewer.h"

using namespace SigDigger;

RMSViewer::RMSViewer(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::RMSViewer)
{
  ui->setupUi(this);

  this->connectAll();

  this->server.listen(QHostAddress("0.0.0.0"), 9999);
}

void
RMSViewer::openNewView(QTcpSocket *socket)
{
  RMSViewTab *tab = new RMSViewTab(this, socket);

  this->ui->serverTabWidget->addTab(
        tab,
        "Power graph [" + socket->peerAddress().toString() + "]");
  this->ui->stackedWidget->setCurrentIndex(0);
}

void
RMSViewer::connectAll(void)
{
  connect(
        &this->server,
        SIGNAL(acceptError(QAbstractSocket::SocketError)),
        this,
        SLOT(onAcceptError(QAbstractSocket::SocketError)));

  connect(
        &this->server,
        SIGNAL(newConnection()),
        this,
        SLOT(onNewConnection()));
}

RMSViewer::~RMSViewer()
{
  delete ui;
}

/////////////////////////////////// Slots //////////////////////////////////////

void
RMSViewer::onAcceptError(QAbstractSocket::SocketError)
{
  QMessageBox::critical(
        this,
        "Accept error",
        "Failed to accept new connection: " + this->server.errorString());
}

void
RMSViewer::onNewConnection(void)
{
  QTcpSocket *socket;

  while ((socket = this->server.nextPendingConnection()) != nullptr)
    this->openNewView(socket);
}
