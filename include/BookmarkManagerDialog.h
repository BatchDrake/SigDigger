//
//    BookmarkManagerDialog.h: Description
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
#ifndef BOOKMARKMANAGERDIALOG_H
#define BOOKMARKMANAGERDIALOG_H

#include <QDialog>
#include <QModelIndex>

namespace Ui {
  class BookmarkManagerDialog;
}

class QSortFilterProxyModel;

namespace SigDigger {
  class BookmarkTableModel;
  class AddBookmarkDialog;
  class ButtonDelegate;

  class BookmarkManagerDialog : public QDialog
  {
      Q_OBJECT

      ButtonDelegate *removeDelegate = nullptr;
      ButtonDelegate *editDelegate = nullptr;

      AddBookmarkDialog *editDialog = nullptr;
      BookmarkTableModel *model = nullptr;
      QSortFilterProxyModel *proxy = nullptr;

      qint64 editingFrequency;

      void connectAll(void);

    public:
      explicit BookmarkManagerDialog(QWidget *parent = nullptr);
      virtual ~BookmarkManagerDialog() override;

      void notifyChanged(void);

      virtual void showEvent(QShowEvent *event) override;

    private:
      Ui::BookmarkManagerDialog *ui;

    public slots:
      void onRemoveBookmark(QModelIndex);
      void onEditBookmark(QModelIndex);
      void onCellActivated(QModelIndex const &);
      void onEditAccepted(void);

    signals:
      void frequencySelected(qint64);
      void bandwidthSelected(qint32);
      void modulationSelected(QString);
      void bookmarkChanged(void);
  };
}

#endif // BOOKMARKMANAGERDIALOG_H
