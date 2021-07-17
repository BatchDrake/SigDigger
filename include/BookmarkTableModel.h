//
//    MultitaskControllerModel.h: Description
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
#ifndef BOOKMARKTABLEMODEL_H
#define BOOKMARKTABLEMODEL_H

#include <QAbstractTableModel>
#include <Suscan/Library.h>
#include <QColor>

namespace SigDigger {
  class BookmarkTableModel : public QAbstractTableModel {
      Q_OBJECT

      const QMap<qint64,Suscan::Bookmark> *bookmarkPtr;

    public:
      BookmarkTableModel(
          QObject *parent,
          const QMap<qint64,Suscan::Bookmark> *);

      int rowCount(const QModelIndex &) const override;
      int columnCount(const QModelIndex &) const override;
      QVariant data(const QModelIndex &, int) const override;
      QVariant headerData(int, Qt::Orientation, int) const override;
      void notifyChanged(void);
      void notifyRemovalStart(int);
      void notifyRemovalFinish(void);

    signals:
      void bookmarkEdited(QString, qint64, QColor);
  };
}

#endif // BOOKMARKTABLEMODEL_H
