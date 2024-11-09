//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
#ifndef DEVICEGAIN_H
#define DEVICEGAIN_H

#include <QWidget>
#include <Suscan/Device.h>

class Ui_DeviceGain;

namespace SigDigger {
  class DeviceGain : public QWidget
  {
    Q_OBJECT

    Ui_DeviceGain *m_ui = nullptr;

    std::string    m_name;
    float          m_min;
    float          m_max;
    float          m_step;
    float          m_defl;

    int            m_current; // Current value, to prevent spurious event triggers

  public:
    explicit DeviceGain(QWidget *parent, Suscan::DeviceGainDesc const &desc);

    inline std::string const &
    getName(void) const
    {
      return m_name;
    }

    void setGain(float);
    float getGain(void) const;

    virtual ~DeviceGain() override;

  signals:
    void gainChanged(QString, float);

  public slots:
    void onValueChanged(int);
    void onResetClicked(void);
  };
}

#endif // DEVICEGAIN_H
