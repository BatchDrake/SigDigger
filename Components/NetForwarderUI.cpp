//
//    UDPForwarderUI.cpp: UDP data forwarder UI
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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

#include "NetForwarderUI.h"
#include "ui_NetForwarderUI.h"

using namespace SigDigger;

void
NetForwarderUI::connectAll(void)
{
  connect(
        this->ui->udpStartStopButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onForwardStartStop(void)));
}

NetForwarderUI::NetForwarderUI(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::UDPForwarderUI)
{
  ui->setupUi(this);

  this->spinner = new WaitingSpinnerWidget(parent);
  this->spinner->setLineLength(5);
  this->spinner->setInnerRadius(5);
  this->ui->spinGrid->addWidget(this->spinner);

  this->connectAll();
}

NetForwarderUI::~NetForwarderUI()
{
  delete spinner;
  delete ui;
}

void
NetForwarderUI::setHost(std::string const &host)
{
  this->ui->hostEdit->setText(QString::fromStdString(host));
}

void
NetForwarderUI::setPort(uint16_t port)
{
  this->ui->portSpin->setValue(port);
}

void
NetForwarderUI::setFrameLen(unsigned int len)
{
  this->ui->frameLen->setValue(static_cast<int>(len));
}

void
NetForwarderUI::setIORate(qreal value)
{
  this->ui->ioBwProgress->setValue(static_cast<int>(value * 100));
}

void
NetForwarderUI::setPreparing(bool preparing)
{
  if (preparing)
    this->spinner->start();
  else
    this->spinner->stop();
}

void
NetForwarderUI::setForwardState(bool state)
{
  this->ui->udpStartStopButton->setChecked(state);

  this->ui->hostEdit->setEnabled(!state);
  this->ui->portSpin->setEnabled(!state);
  this->ui->frameLen->setEnabled(!state);

  this->ui->udpStartStopButton->setText(state ? "Stop" : "Forward");

  if (!state) {
    this->ui->ioBwProgress->setValue(0);
    this->setPreparing(false);
  }
}

void
NetForwarderUI::setForwardEnabled(bool enabled)
{
  if (!enabled)
    this->setForwardState(false);

  this->ui->udpStartStopButton->setEnabled(enabled);
}

static QString
formatCaptureSize(quint64 size)
{
  if (size < (1ull << 10))
    return QString::number(size) + " bytes";
  else if (size < (1ull << 20))
    return QString::number(size >> 10) + " KiB";
  else if (size < (1ull << 30))
    return QString::number(size >> 20) + " MiB";

  return QString::number(size >> 30) + " GiB";
}

void
NetForwarderUI::setCaptureSize(quint64 size)
{
  this->ui->txLenLabel->setText(
        formatCaptureSize(size * sizeof(float _Complex)));
}

void
NetForwarderUI::setTcp(bool tcp)
{
  this->ui->socketTypeCombo->setCurrentIndex(tcp ? 1 : 0);
}

std::string
NetForwarderUI::getHost(void) const
{
  return this->ui->hostEdit->text().toStdString();
}

uint16_t
NetForwarderUI::getPort(void) const
{
  return static_cast<uint16_t>(this->ui->portSpin->value());
}

unsigned int
NetForwarderUI::getFrameLen(void) const
{
  return static_cast<unsigned int>(this->ui->frameLen->value());
}

bool
NetForwarderUI::getForwardState(void) const
{
  return this->ui->udpStartStopButton->isChecked();
}

bool
NetForwarderUI::getTcp(void) const
{
  return this->ui->socketTypeCombo->currentIndex() == 1;
}

///////////////////////////////// Slots ///////////////////////////////////////
void
NetForwarderUI::onForwardStartStop(void)
{
  this->ui->udpStartStopButton->setText(
        this->ui->udpStartStopButton->isChecked()
        ? "Stop"
        : "Forward");

  emit forwardStateChanged(this->ui->udpStartStopButton->isChecked());
}
