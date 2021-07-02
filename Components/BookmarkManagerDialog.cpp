//
//    BookmarkManagerDialog.cpp: Description
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
#include <BookmarkManagerDialog.h>
#include "ui_BookmarkManagerDialog.h"
#include <BookmarkTableModel.h>
#include <QSortFilterProxyModel>
#include <Suscan/Library.h>
#include <AddBookmarkDialog.h>
#include <TableDelegates.h>

using namespace SigDigger;

BookmarkManagerDialog::BookmarkManagerDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::BookmarkManagerDialog)
{
  this->removeDelegate = new ButtonDelegate(this, "&Remove");
  this->editDelegate = new ButtonDelegate(this, "&Edit");

  ui->setupUi(this);

  this->model = new BookmarkTableModel(
        this,
        &Suscan::Singleton::get_instance()->getBookmarkMap());
  this->proxy = new QSortFilterProxyModel(this);
  this->proxy->setSourceModel(this->model);

  this->ui->bookmarkView->setModel(this->model);
  this->ui->bookmarkView->setSortingEnabled(false);

  this->editDialog = new AddBookmarkDialog(this);
  this->editDialog->setWindowTitle("Edit bookmark");

  this->ui->bookmarkView->setItemDelegateForColumn(
        5,
        this->editDelegate);

  this->ui->bookmarkView->setItemDelegateForColumn(
        6,
        this->removeDelegate);

  QHeaderView *headerView = this->ui->bookmarkView->horizontalHeader();
  headerView->setStretchLastSection(false);
  headerView->setSectionResizeMode(4, QHeaderView::Stretch);

  this->connectAll();
}

void
BookmarkManagerDialog::connectAll(void)
{
  connect(
        this->editDelegate,
        SIGNAL(clicked(QModelIndex)),
        this,
        SLOT(onEditBookmark(QModelIndex)));

  connect(
        this->removeDelegate,
        SIGNAL(clicked(QModelIndex)),
        this,
        SLOT(onRemoveBookmark(QModelIndex)));

  connect(
        this->editDialog,
        SIGNAL(accepted(void)),
        this,
        SLOT(onEditAccepted(void)));

  connect(
        this->ui->bookmarkView,
        SIGNAL(activated(QModelIndex const &)),
        this,
        SLOT(onCellActivated(QModelIndex const &)));
}

BookmarkManagerDialog::~BookmarkManagerDialog()
{
  delete ui;
}

void
BookmarkManagerDialog::showEvent(QShowEvent *)
{
  this->notifyChanged();

  this->ui->bookmarkView->resizeColumnsToContents();
  this->ui->bookmarkView->setColumnWidth(3, this->editDelegate->width());
  this->ui->bookmarkView->setColumnWidth(4, this->removeDelegate->width());
}

void
BookmarkManagerDialog::notifyChanged(void)
{
  this->model->notifyChanged();

  emit bookmarkChanged();
}

void
BookmarkManagerDialog::onRemoveBookmark(QModelIndex index)
{
  auto map = &Suscan::Singleton::get_instance()->getBookmarkMap();

  if (this->ui->bookmarkView->model() == this->proxy)
    index = this->proxy->mapToSource(index);

  int row = index.row();

  if (row >= 0 && row < map->count()) {
    Suscan::Bookmark bm = (*map)[map->keys().at(row)];
    this->model->notifyRemovalStart(row);
    Suscan::Singleton::get_instance()->removeBookmark(bm.info.frequency);
    this->model->notifyRemovalFinish();
  }
}

void
BookmarkManagerDialog::onEditAccepted(void)
{
  BookmarkInfo newInfo;
  newInfo.frequency = this->editDialog->frequency();
  newInfo.name = this->editDialog->name();
  newInfo.color = this->editDialog->color();
  newInfo.lowFreqCut = -this->editDialog->bandwidth() / 2;
  newInfo.highFreqCut = this->editDialog->bandwidth() / 2;
  newInfo.modulation = editDialog->modulation();

  if (this->editingFrequency == newInfo.frequency) {
    Suscan::Singleton::get_instance()->replaceBookmark(newInfo);

  } else {
    Suscan::Singleton::get_instance()->removeBookmark(this->editingFrequency);
    Suscan::Singleton::get_instance()->registerBookmark(newInfo);
  }

  this->notifyChanged();
}

void
BookmarkManagerDialog::onEditBookmark(QModelIndex index)
{
  auto map = &Suscan::Singleton::get_instance()->getBookmarkMap();

  if (this->ui->bookmarkView->model() == this->proxy)
    index = this->proxy->mapToSource(index);

  int row = index.row();

  if (row >= 0 && row < map->count()) {
    Suscan::Bookmark bm = (*map)[map->keys().at(row)];

    this->editingFrequency = bm.info.frequency;

    this->editDialog->setNameHint(bm.info.name);
    this->editDialog->setFrequencyHint(this->editingFrequency);
    this->editDialog->setColorHint(bm.info.color);
    this->editDialog->setBandwidthHint(bm.info.bandwidth());
    this->editDialog->setModulationHint(bm.info.modulation);

    this->editDialog->show();
  }
}

void
BookmarkManagerDialog::onCellActivated(QModelIndex const &index)
{
  QModelIndex copy = index;

  auto map = &Suscan::Singleton::get_instance()->getBookmarkMap();

  if (this->ui->bookmarkView->model() == this->proxy)
    copy = this->proxy->mapToSource(copy);

  int row = copy.row();

  if (row >= 0 && row < map->count()) {
    Suscan::Bookmark bm = (*map)[map->keys().at(row)];
    emit frequencySelected(static_cast<qint64>(bm.info.frequency));
    if(bm.info.bandwidth() != 0) {
      emit bandwidthSelected(bm.info.bandwidth());
    }
    if(!bm.info.modulation.isEmpty()) {
      emit modulationSelected(bm.info.modulation.toStdString());
    }
  }
}
