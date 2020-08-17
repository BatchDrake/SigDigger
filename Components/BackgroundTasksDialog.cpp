//
//    BackgroundTasksDialog.cpp: Description
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
#include <BackgroundTasksDialog.h>
#include "ui_BackgroundTasksDialog.h"

#include <MultitaskControllerModel.h>
#include <QSortFilterProxyModel>
#include <QMouseEvent>
#include <QMessageBox>

using namespace SigDigger;

ProgressBarDelegate::ProgressBarDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void
ProgressBarDelegate::paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    const MultitaskControllerModel *model =
        static_cast<const MultitaskControllerModel *>(index.model());

    if (model != nullptr) {
      QStyleOptionProgressBar progressBarOption;
      qreal progress = model->data(index, Qt::DisplayRole).value<qreal>();
      int intProgress = static_cast<int>(progress * 100);
      progressBarOption.rect = option.rect;
      progressBarOption.minimum = 0;
      progressBarOption.maximum = 100;
      progressBarOption.progress = intProgress;
      progressBarOption.text = QString::number(intProgress) + "%";
      progressBarOption.textVisible = true;

      QApplication::style()->drawControl(
            QStyle::CE_ProgressBar,
            &progressBarOption,
            painter);
    }
}

ButtonDelegate::ButtonDelegate(QObject *parent, QString text)
    : QItemDelegate(parent)
{
  this->text = text;
}


void
ButtonDelegate::paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &) const
{
  QStyleOptionButton button;
  QRect r = option.rect; //getting the rect of the cell
  int x, y, w, h;

  x = r.left(); //the X coordinate
  y = r.top(); //the Y coordinate
  w = 70; //button width
  h = r.height(); //button height

  button.rect = QRect(x, y, w, h);
  button.text = text;
  button.state = this->pressed
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
  } else if (event->type() == QEvent::FocusOut) {
    this->pressed = false;
  } else if (event->type() == QEvent::MouseButtonRelease) {
    QMouseEvent * e = static_cast<QMouseEvent *>(event);
    int clickX = e->x();
    int clickY = e->y();

    QRect r = option.rect; //getting the rect of the cell
    int x, y, w, h;

    this->pressed = false;

    x = r.left();
    y = r.top();
    w = 70;
    h = r.height();

    if (clickX > x && clickX < x + w && clickY > y && clickY < y + h)
      emit clicked(index);
  }

  return true;
}


BackgroundTasksDialog::BackgroundTasksDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::BackgroundTasksDialog)
{
  ui->setupUi(this);

  this->connectAll();
}

BackgroundTasksDialog::~BackgroundTasksDialog()
{
  delete ui;
}


void
BackgroundTasksDialog::connectAll(void)
{
  connect(
        this->ui->cancelAllButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onCancelAll(void)));

  connect(
        this->ui->buttonBox,
        SIGNAL(rejected()),
        this,
        SLOT(hide()));
}

void
BackgroundTasksDialog::setController(MultitaskController *controller)
{
  ButtonDelegate *delegate = new ButtonDelegate(this, "&Cancel");

  this->controller = controller;
  this->model = new MultitaskControllerModel(this, controller);
  this->proxy = new QSortFilterProxyModel(this);
  this->proxy->setSourceModel(this->model);

  // Until we figure out what the heck is wrong with the proxy model
  // we stay with the unsortable model
  this->ui->tableView->setModel(this->model);
  this->ui->tableView->setSortingEnabled(false);

  this->ui->tableView->setItemDelegateForColumn(
        4,
        new ProgressBarDelegate(this));

  this->ui->tableView->setItemDelegateForColumn(
        5,
        delegate);

  connect(
        this->model,
        SIGNAL(taskError(QString, QString)),
        this,
        SLOT(onError(QString, QString)));


  connect(
        this->model,
        SIGNAL(layoutChanged(void)),
        this,
        SLOT(onLayoutChanged(void)));

  connect(
        this->model,
        SIGNAL(
          dataChanged(
            const QModelIndex &,
            const QModelIndex &,
            const QVector<int> &)),
        this,
        SLOT(onLayoutChanged(void)));

  connect(
        delegate,
        SIGNAL(clicked(QModelIndex)),
        this,
        SLOT(onCancelClicked(QModelIndex)));
}

/////////////////////////////////// Slots //////////////////////////////////////
void
BackgroundTasksDialog::onClose(void)
{
  this->hide();
}

void
BackgroundTasksDialog::onCancelAll(void)
{
  if (this->controller != nullptr)
    this->controller->cancelAll();
}

void
BackgroundTasksDialog::onLayoutChanged(void)
{
  int rows = this->model->rowCount(QModelIndex());

  if (rows > this->prevRows)
    this->ui->tableView->resizeColumnsToContents();
  else if (rows == this->prevRows)
    this->ui->tableView->resizeColumnToContents(2);

  this->prevRows = rows;

  this->ui->tableView->setColumnWidth(3, 120);
  this->ui->cancelAllButton->setEnabled(rows > 0);
}

void
BackgroundTasksDialog::onCancelClicked(QModelIndex index)
{
  this->controller->cancelByIndex(index.row());
}

void
BackgroundTasksDialog::onError(QString title, QString err)
{
  QMessageBox::critical(
        nullptr,
        "Background task error",
        "Background task <b>" + title
        + "</b> failed during execution with the following error:<br /><code>"
        + err + "</code>");
}

