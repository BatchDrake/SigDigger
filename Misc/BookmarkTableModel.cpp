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
#include <QDebug>

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
  // Frequency, bandwidth, modulation, name, color, remove
  return 7;
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
        return SuWidgetsHelpers::formatQuantity(bookmark.info.frequency, "Hz");

      case 1:
        return SuWidgetsHelpers::formatQuantity(bookmark.info.bandwidth(), "Hz");

      case 2:
        return bookmark.info.modulation;

      case 3:
        return "";

      case 4:
        return bookmark.info.name;

      case 5:
      case 6:
        return QVariant();
    }
  } else if (role == Qt::BackgroundColorRole) {
    if (index.column() == 3)
      return bookmark.info.color;
  }

  return QVariant();
}

QVariant
BookmarkTableModel::headerData(int s, Qt::Orientation hor, int role) const
{
  if (hor == Qt::Horizontal && role == Qt::DisplayRole) {
    const char *headers[] = {
      "Frequency",
      "Bandwidth",
      "Modulation",
      "Color",
      "Name",
      "",
    ""};

    if (s >= 0 && s < 7)
      return headers[s];
  }

  return QVariant();
}

void
BookmarkTableModel::notifyChanged(void)
{
  emit layoutChanged();
}

void
BookmarkTableModel::notifyRemovalStart(int row)
{
  beginRemoveRows(QModelIndex(), row, row);
}


void
BookmarkTableModel::notifyRemovalFinish(void)
{
  endRemoveRows();
}
