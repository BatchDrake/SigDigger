//
//    BackgroundTasksDialog.h: Description
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
#ifndef BACKGROUNDTASKSDIALOG_H
#define BACKGROUNDTASKSDIALOG_H

#include <QDialog>

namespace Ui {
  class BackgroundTasksDialog;
}

class QSortFilterProxyModel;

namespace SigDigger {
  class MultitaskController;
  class MultitaskControllerModel;

  class BackgroundTasksDialog : public QDialog
  {
      Q_OBJECT

      MultitaskController *controller = nullptr;
      MultitaskControllerModel *model = nullptr;
      QSortFilterProxyModel *proxy = nullptr;
      int prevRows = 0;
      void connectAll(void);

    public:
      explicit BackgroundTasksDialog(QWidget *parent = nullptr);
      ~BackgroundTasksDialog();

      void setController(MultitaskController *);

    private:
      Ui::BackgroundTasksDialog *ui;

    public slots:
      void onClose(void);
      void onCancelAll(void);
      void onLayoutChanged(void);
  };
}

#endif // BACKGROUNDTASKSDIALOG_H
