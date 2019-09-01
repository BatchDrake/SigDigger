//
//    UDPForwarderUI.h: UDP data forwarder UI
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

#ifndef UDPFORWARDERUI_H
#define UDPFORWARDERUI_H

#include <QWidget>

namespace Ui {
  class UDPForwarderUI;
}

namespace SigDigger {
  class UDPForwarderUI : public QWidget
  {
    Q_OBJECT

    void connectAll(void);

  public:
    explicit UDPForwarderUI(QWidget *parent = nullptr);
    ~UDPForwarderUI();

    // Setters
    void setHost(std::string const &name);
    void setPort(uint16_t port);
    void setFrameLen(unsigned int len);
    void setIORate(qreal rate);
    void setForwardState(bool state);
    void setForwardEnabled(bool enabled);
    void setCaptureSize(quint64 size);

    // Getters
    std::string getHost(void) const;
    uint16_t getPort(void) const;
    unsigned int getFrameLen(void) const;
    bool getForwardState(void) const;

  public slots:
    void onForwardStartStop(void);

  signals:
    void forwardStateChanged(bool state);

  private:
    Ui::UDPForwarderUI *ui;
  };
}

#endif // UDPFORWARDERUI_H
