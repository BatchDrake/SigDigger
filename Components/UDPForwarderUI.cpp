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

#include "UDPForwarderUI.h"
#include "ui_UDPForwarderUI.h"

using namespace SigDigger;

void
UDPForwarderUI::connectAll(void)
{
  connect(
        this->ui->udpStartStopButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onForwardStartStop(void)));
}

UDPForwarderUI::UDPForwarderUI(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::UDPForwarderUI)
{
  ui->setupUi(this);

  this->connectAll();
}

UDPForwarderUI::~UDPForwarderUI()
{
  delete ui;
}

void
UDPForwarderUI::setHost(std::string const &host)
{
  this->ui->udpHostLabel->setText(QString::fromStdString(host));
}

void
UDPForwarderUI::setPort(uint16_t port)
{
  this->ui->udpPortLabel->setValue(port);
}

void
UDPForwarderUI::setFrameLen(unsigned int len)
{
  this->ui->udpFrameLen->setValue(static_cast<int>(len));
}

void
UDPForwarderUI::setIORate(qreal value)
{
  this->ui->udpIOBwProgress->setValue(static_cast<int>(value * 100));
}

void
UDPForwarderUI::setForwardState(bool state)
{
  this->ui->udpStartStopButton->setChecked(state);

  this->ui->udpHostLabel->setEnabled(!state);
  this->ui->udpPortLabel->setEnabled(!state);
  this->ui->udpFrameLen->setEnabled(!state);

  if (!state)
    this->ui->udpIOBwProgress->setValue(0);
}

void
UDPForwarderUI::setForwardEnabled(bool enabled)
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
UDPForwarderUI::setCaptureSize(quint64 size)
{
  this->ui->lengthSizeLabel->setText(
        formatCaptureSize(size * sizeof(float _Complex)));
}

std::string
UDPForwarderUI::getHost(void) const
{
  return this->ui->udpHostLabel->text().toStdString();
}

uint16_t
UDPForwarderUI::getPort(void) const
{
  return static_cast<uint16_t>(this->ui->udpPortLabel->value());
}

unsigned int
UDPForwarderUI::getFrameLen(void) const
{
  return static_cast<unsigned int>(this->ui->udpFrameLen->value());
}

bool
UDPForwarderUI::getForwardState(void) const
{
  return this->ui->udpStartStopButton->isChecked();
}

///////////////////////////////// Slots ///////////////////////////////////////
void
UDPForwarderUI::onForwardStartStop(void)
{
  emit forwardStateChanged(this->ui->udpStartStopButton->isChecked());
}
