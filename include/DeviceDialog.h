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
#include <vector>

namespace Ui {
  class DeviceDialog;
}

namespace SigDigger {
  class DeviceDialog : public QDialog
  {
      Q_OBJECT

      static QPixmap getDeviceIcon(Suscan::Source::Device const &);
      void setRefreshing(bool refreshing);
      void connectAll(void);

    public:
      explicit DeviceDialog(QWidget *parent = nullptr);
      ~DeviceDialog();
      void refreshDevices(void);
      void refreshDone(void);
      void run(void);

    signals:
      void refreshRequest(void);

    public slots:
      void onRefresh(void);
      void onOk(void);

    private:
      Ui::DeviceDialog *ui;
  };
}

#endif // DEVICEDIALOG_H
