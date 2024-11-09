//
//    DeviceDialog.h: Description
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
#ifndef DEVICEDIALOG_H
#define DEVICEDIALOG_H

#include <QDialog>
#include <Suscan/Library.h>
#include <Suscan/Source.h>
#include <Suscan/Device.h>
#include <vector>

namespace Ui {
  class DeviceDialog;
}

namespace SigDigger {
  class DeviceDialog : public QDialog
  {
      Q_OBJECT

      static QPixmap getDeviceIcon(Suscan::DeviceProperties const &);
      void setRefreshing(bool refreshing);
      void connectAll();

    public:
      explicit DeviceDialog(QWidget *parent = nullptr);
      virtual ~DeviceDialog() override;
      void refreshDevices();
      void refreshDone();
      void run();

    signals:
      void refreshRequest();

    public slots:
      void onRefresh();
      void onOk();

    private:
      Ui::DeviceDialog *ui;
  };
}

#endif // DEVICEDIALOG_H
