//
//    BookmarkTableModel.cpp: Table model for Multitask controller
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
#include <BookmarkTableModel.h>
#include <SuWidgetsHelpers.h>
#include <cmath>
#include <QColor>

using namespace SigDigger;

BookmarkTableModel::BookmarkTableModel(
    QObject *parent,
    const QMap<qint64,Suscan::Bookmark> *map) : QAbstractTableModel(parent)
{
  this->bookmarkPtr = map;
}


int
BookmarkTableModel::rowCount(const QModelIndex &) const
{
  return this->bookmarkPtr->count();
}

int
BookmarkTableModel::columnCount(const QModelIndex &) const
{
  // Frequency, name, color, remove
  return 5;
}

QVariant
BookmarkTableModel::data(const QModelIndex &index, int role) const
{
  if (index.row() < 0 || index.row() >= this->bookmarkPtr->count())
    return QVariant();

  Suscan::Bookmark bookmark =
      (*this->bookmarkPtr)[this->bookmarkPtr->keys().at(index.row())];

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
      case 0:
        return SuWidgetsHelpers::formatQuantity(bookmark.frequency, "Hz");

      case 1:
        return "";

      case 2:
        return QString::fromStdString(bookmark.name);

      case 3:
      case 4:
        return QVariant();
    }
  } else if (role == Qt::BackgroundColorRole) {
    if (index.column() == 1)
      return QColor(QString::fromStdString(bookmark.color));
  }

  return QVariant();
}

QVariant
BookmarkTableModel::headerData(int s, Qt::Orientation hor, int role) const
{
  if (hor == Qt::Horizontal && role == Qt::DisplayRole) {
    const char *headers[] = {
      "Frequency",
      "Color",
      "Name",
      "",
    ""};

    if (s >= 0 && s < 5)
      return headers[s];
  }

  return QVariant();
}

void
BookmarkTableModel::notifyChanged(void)
{
  emit layoutChanged();
}
