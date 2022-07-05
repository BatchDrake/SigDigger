//
//    QuickConnectDialog.h: Quick connect dialog
//    Copyright (C) 2022 Gonzalo José Carracedo Carballal
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
#ifndef QUICKCONNECTDIALOG_H
#define QUICKCONNECTDIALOG_H

#include <QDialog>
#include <Suscan/Source.h>

namespace Ui {
  class QuickConnectDialog;
}

namespace SigDigger {
  class QuickConnectDialog : public QDialog
  {
    Q_OBJECT

  public:
    explicit QuickConnectDialog(QWidget *parent = nullptr);
    ~QuickConnectDialog();
    void setProfile(Suscan::Source::Config const &);
    QString getHost(void) const;
    QString getUser(void) const;
    QString getPassword(void) const;
    int getPort(void) const;

  private:
    Ui::QuickConnectDialog *ui;
  };
}

#endif // QUICKCONNECTDIALOG_H
