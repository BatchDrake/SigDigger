//
//    SaveProfileDialog.h: Save profile dialog
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

#ifndef SAVEPROFILEDIALOG_H
#define SAVEPROFILEDIALOG_H

#include <QDialog>

namespace Ui {
  class SaveProfileDialog;
}

namespace SigDigger {
  class SaveProfileDialog : public QDialog
  {
      Q_OBJECT

    public:
      explicit SaveProfileDialog(QWidget *parent = nullptr);
      ~SaveProfileDialog();
      bool run(void);

    public slots:
      void onTextChanged(void);

      QString getProfileName(void) const;
      void setProfileName(QString const &name);

    private:
      Ui::SaveProfileDialog *ui;
  };
}

#endif // SAVEPROFILEDIALOG_H
