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
#ifndef ADDTLESOURCEDIALOG_H
#define ADDTLESOURCEDIALOG_H

#include <QDialog>
#include <Suscan/Library.h>

namespace Ui {
  class AddTLESourceDialog;
}

namespace SigDigger {
  class AddTLESourceDialog : public QDialog
  {
    Q_OBJECT

    static bool urlIsValid(QString const &);
    void connectAll(void);

  public:
    explicit AddTLESourceDialog(QWidget *parent = nullptr);
    ~AddTLESourceDialog();
    bool run(void);
    void reset(void);
    Suscan::TLESource getTLESource(void) const;

  private:
    Ui::AddTLESourceDialog *ui;

  public slots:
    void onFieldsChanged(void);
  };
}

#endif // ADDTLESOURCEDIALOG_H
