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
#include <RMSViewerSettingsDialog.h>
#include <QMessageBox>

#include "ui_RMSViewer.h"

using namespace SigDigger;

RMSViewer::RMSViewer(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::RMSViewer)
{
  ui->setupUi(this);

  this->settingsDialog = new RMSViewerSettingsDialog(this);
  this->connectAll();
}

void
RMSViewer::openNewView(QTcpSocket *socket)
{
  RMSViewTab *tab = new RMSViewTab(this, socket);

  int ndx = this->ui->serverTabWidget->addTab(
        tab,
        "Power graph [" + socket->peerAddress().toString() + "]");

  connect(
        tab,
        SIGNAL(titleChanged(QString)),
        this,
        SLOT(onTitleChanged(QString)));

  this->ui->stackedWidget->setCurrentIndex(0);
  this->ui->serverTabWidget->setCurrentIndex(ndx);
}

bool
RMSViewer::haveAddrData(void) const
{
  return this->listenPort != 0 && this->listenAddr.size() > 0;
}


bool
RMSViewer::openSettings(void)
{
  int result = this->settingsDialog->exec();

  if (result == QDialog::Accepted) {
    this->listenAddr = this->settingsDialog->getHost();
    this->listenPort = this->settingsDialog->getPort();
    return true;
  }

  return false;
}

bool
RMSViewer::getListeningState(void) const
{
  return this->listening;
}

void
RMSViewer::setListeningState(bool state)
{
  if (this->listening != state) {
    if (state) {
      if (!this->haveAddrData()) {
        QMessageBox::critical(
              this,
              "TCP server",
              "TCP server has not been properly configured. Please open the "
              "settings dialog to define the listening address and port and "
              "try again.");
      } else {
        this->listening = this->server.listen(
              QHostAddress(this->listenAddr),
              this->listenPort);

        if (!this->listening) {
          QMessageBox::critical(
                this,
                "TCP server",
                "Failed to start TCP server: " + this->server.errorString());
        }
      }
    } else {
      this->server.close();
      this->listening = false;
    }

    if (this->listening) {
      this->setWindowTitle(
            "RMSViewer [Listening from "
            + this->listenAddr
            + ":"
            + QString::number(this->listenPort)
            + "]");
      this->ui->welcomeLabel->setText(
            "<i>Accepting connections from "
            + this->listenAddr
            + ":"
            + QString::number(this->listenPort)
            + "...</i>");
    } else {
      this->setWindowTitle(
            "RMSViewer [Stopped]");
      this->ui->welcomeLabel->setText(
            "<i>Click start button to start RMS Viewer server.</i>");
    }
  }
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

  connect(
        this->ui->actionStartStop,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleListening(void)));

  connect(
        this->ui->actionSettings,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onOpenSettings(void)));

  connect(
        this->ui->serverTabWidget,
        SIGNAL(tabCloseRequested(int)),
        this,
        SLOT(onTabCloseRequested(int)));
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

  this->setListeningState(false);
}

void
RMSViewer::onNewConnection(void)
{
  QTcpSocket *socket;

  while ((socket = this->server.nextPendingConnection()) != nullptr)
    this->openNewView(socket);
}

void
RMSViewer::onToggleListening(void)
{
  if (this->ui->actionStartStop->isChecked()) {
    if (!this->haveAddrData())
      if (!this->openSettings()) {
        this->setListeningState(false);
        return;
      }

    if (this->haveAddrData())
      this->setListeningState(true);
  } else {
    this->setListeningState(false);
  }
}

void
RMSViewer::onOpenSettings(void)
{
  if (this->openSettings() && this->haveAddrData()) {
    if (this->getListeningState()) {
      int ret = QMessageBox::question(
          this,
          "Restart TCP server",
          "TCP server settings have just changed, but it must be restarted "
          "in order for the changes to take effect. Restarting will not close "
          "existing remote connections.\n\nDo you want to restart "
          "it now?");
      if (ret == QMessageBox::Yes) {
        this->setListeningState(false);
        this->setListeningState(true);
      }
    }
  }
}

void
RMSViewer::onTitleChanged(QString title)
{
  QWidget *tab = static_cast<QWidget *>(this->sender());
  int ndx = this->ui->serverTabWidget->indexOf(tab);

  if (ndx != -1)
    this->ui->serverTabWidget->setTabText(ndx, title);

}

void
RMSViewer::onTabCloseRequested(int ndx)
{
  QString name = this->ui->serverTabWidget->tabText(ndx);

  if (QMessageBox::question(
        this,
        "Close tab",
        "You are about to close tab " + name + ". This will "
        "close the connection and clear unsaved data. Are you sure?") ==
      QMessageBox::Yes) {
    QWidget *widget = this->ui->serverTabWidget->widget(ndx);
    this->ui->serverTabWidget->removeTab(ndx);
    delete widget;

    if (this->ui->serverTabWidget->count() == 0)
      this->ui->stackedWidget->setCurrentIndex(1);
  }
}
