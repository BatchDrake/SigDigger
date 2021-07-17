//
//    RMSViewerSettingsDialog.h: Description
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
#ifndef RMSVIEWERSETTINGSDIALOG_H
#define RMSVIEWERSETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
  class RMSViewerSettingsDialog;
}

namespace SigDigger {
  class RMSViewerSettingsDialog : public QDialog
  {
      Q_OBJECT

    public:
      explicit RMSViewerSettingsDialog(QWidget *parent = nullptr);
      ~RMSViewerSettingsDialog();
      QString getHost(void) const;
      uint16_t getPort(void) const;

    private:
      Ui::RMSViewerSettingsDialog *ui;
  };
};

#endif // RMSVIEWERSETTINGSDIALOG_H
