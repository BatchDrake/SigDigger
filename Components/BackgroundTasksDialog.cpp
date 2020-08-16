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

using namespace SigDigger;

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
}

void
BackgroundTasksDialog::setController(MultitaskController *controller)
{
  this->controller = controller;
  this->model = new MultitaskControllerModel(this, controller);
  this->proxy = new QSortFilterProxyModel(this);
  this->proxy->setSourceModel(this->model);
  this->ui->tableView->setModel(this->proxy);

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

  this->prevRows = rows;

  this->ui->cancelAllButton->setEnabled(rows > 0);
}
