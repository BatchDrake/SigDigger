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
#include <QStyledItemDelegate>
#include <QItemDelegate>

namespace Ui {
  class BackgroundTasksDialog;
}

namespace Suscan {
  class MultitaskController;
}

class QSortFilterProxyModel;

namespace SigDigger {
  class MultitaskControllerModel;

  class ProgressBarDelegate : public QStyledItemDelegate
  {
      Q_OBJECT

  public:
      ProgressBarDelegate(QObject *parent = nullptr);
      void paint(QPainter *painter, const QStyleOptionViewItem &option,
                 const QModelIndex &index) const;
  };

  class ButtonDelegate : public QItemDelegate
  {
      Q_OBJECT

      QString text;
      bool pressed = false;

  public:
      ButtonDelegate(QObject *parent, QString);
      void paint(
          QPainter *painter,
          const QStyleOptionViewItem &option,
          const QModelIndex &index) const;

      bool editorEvent(
          QEvent *event,
          QAbstractItemModel *model,
          const QStyleOptionViewItem &option,
          const QModelIndex &index);

    signals:
      void clicked(QModelIndex);
  };

  class BackgroundTasksDialog : public QDialog
  {
      Q_OBJECT

      Suscan::MultitaskController *controller = nullptr;
      MultitaskControllerModel *model = nullptr;
      QSortFilterProxyModel *proxy = nullptr;
      int prevRows = 0;
      void connectAll(void);

    public:
      explicit BackgroundTasksDialog(QWidget *parent = nullptr);
      ~BackgroundTasksDialog();

      void setController(Suscan::MultitaskController *);

    private:
      Ui::BackgroundTasksDialog *ui;

    public slots:
      void onClose(void);
      void onCancelAll(void);
      void onLayoutChanged(void);
      void onCancelClicked(QModelIndex);
      void onError(QString title, QString err);
  };
}

#endif // BACKGROUNDTASKSDIALOG_H
