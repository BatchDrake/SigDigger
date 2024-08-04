//
//    TableDelegates.cpp: Generic TableView delegates
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
#include <QApplication>
#include <QMouseEvent>
#include "TableDelegates.h"
#include <QFontMetrics>

using namespace SigDigger;


#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  define GET_X(e) static_cast<int>((e)->position().x())
#  define GET_Y(e) static_cast<int>((e)->position().y())
#else
#  define GET_X(e) (e)->x()
#  define GET_Y(e) (e)->y()
#endif // QT_VERSION

ButtonDelegate::ButtonDelegate(QObject *parent, QString text)
    : QItemDelegate(parent)
{
  QFont font;

  font.setFamily(font.defaultFamily());

  QFontMetrics fm(font);

  this->text = text;
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  this->buttonWidth = fm.horizontalAdvance(" " + text + " ");
#else
  this->buttonWidth = fm.width(" " + text + " ");
#endif // QT_VERSION_CHECK
}


void
ButtonDelegate::paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
  QStyleOptionButton button;
  QRect r = option.rect; //getting the rect of the cell
  int x, y, w, h;
  x = r.left(); //the X coordinate
  y = r.top(); //the Y coordinate
  w = this->buttonWidth; //button width
  h = r.height(); //button height

  button.rect = QRect(x, y, w, h);
  button.text = text;
  button.state = this->pressed && index.row() == this->rowPressed
      ? QStyle::State_Sunken
      : QStyle::State_Enabled;

  QApplication::style()->drawControl(
        QStyle::CE_PushButton,
        &button,
        painter);
}

bool
ButtonDelegate::editorEvent(
    QEvent *event,
    QAbstractItemModel *,
    const QStyleOptionViewItem &option,
    const QModelIndex &index)
{
  if (event->type() == QEvent::MouseButtonPress) {
    this->pressed = true;
    this->rowPressed = index.row();
  } else if (event->type() == QEvent::FocusOut) {
    this->pressed = false;
  } else if (event->type() == QEvent::MouseButtonRelease) {
    QMouseEvent * e = static_cast<QMouseEvent *>(event);
    int clickX = GET_X(e);
    int clickY = GET_Y(e);

    QRect r = option.rect; //getting the rect of the cell
    int x, y, w, h;

    this->pressed = false;

    x = r.left();
    y = r.top();
    w = this->buttonWidth;
    h = r.height();

    if (clickX > x && clickX < x + w && clickY > y && clickY < y + h)
      emit clicked(index);
  }

  return true;
}

QSize
ButtonDelegate::sizeHint(
    const QStyleOptionViewItem &option,
    const QModelIndex &) const
{
  QSize size;

  size.setWidth(this->buttonWidth);
  size.setHeight(option.rect.width());

  return size;
}
