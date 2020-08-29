//
//    include/TableDelegates.h: Description
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
#ifndef TABLEDELEGATES_H
#define TABLEDELEGATES_H

#include <QStyledItemDelegate>
#include <QItemDelegate>

namespace SigDigger {
  class ButtonDelegate : public QItemDelegate
  {
      Q_OBJECT

      QString text;
      bool pressed = false;
      int buttonWidth = 0;

  public:
      int
      width(void) const
      {
        return this->buttonWidth;
      }

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
}

#endif // TABLEDELEGATES_H
