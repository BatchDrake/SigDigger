//
//    DecoderDialog.h: Decoder chooser dialog
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
#ifndef DECODERDIALOG_H
#define DECODERDIALOG_H

#include <QDialog>

namespace Ui {
  class DecoderDialog;
}

namespace Suscan {
  class DecoderFactory;
};

namespace SigDigger {
  class DecoderDialog : public QDialog
  {
    Q_OBJECT

    std::vector<Suscan::DecoderFactory *> cache;

    void populate(void);

  public:
    explicit DecoderDialog(QWidget *parent = nullptr);
    bool run(void);
    Suscan::DecoderFactory *getSelected(void) const;
    ~DecoderDialog();

  private:
    Ui::DecoderDialog *ui;
  };
}

#endif // DECODERDIALOG_H
