//
//    Misc/MultitaskControllerModel.cpp: Table model for Multitask controller
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
#include <MultitaskControllerModel.h>
#include <SuWidgetsHelpers.h>
#include <cmath>

using namespace SigDigger;

MultitaskControllerModel::MultitaskControllerModel(
    QObject *parent,
    Suscan::MultitaskController *ctl) : QAbstractTableModel(parent)
{
  this->controller = ctl;
  ctl->getTaskVector(this->taskVec);

  this->connectAll();
}

void
MultitaskControllerModel::connectAll(void)
{
  connect(
        this->controller,
        SIGNAL(taskAdded(CancellableTask *)),
        this,
        SLOT(onListChanged(void)));

  connect(
        this->controller,
        SIGNAL(taskDone(int)),
        this,
        SLOT(onListChanged(void)));

  connect(
        this->controller,
        SIGNAL(taskCancelled(int)),
        this,
        SLOT(onListChanged(void)));

  connect(
        this->controller,
        SIGNAL(taskError(int, QString)),
        this,
        SLOT(onError(int, QString)));

  connect(
        this->controller,
        SIGNAL(taskProgress(int, qreal, QString)),
        this,
        SLOT(onProgress(int, qreal, QString)));
}

int
MultitaskControllerModel::rowCount(const QModelIndex &) const
{
  return this->taskVec.size();
}

int
MultitaskControllerModel::columnCount(const QModelIndex &) const
{
  return 6;
}

QVariant
MultitaskControllerModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole) {
    Suscan::CancellableTaskContext *ctx;
    if (index.row() < 0 || index.row() >= this->taskVec.size())
      return QVariant();

    ctx = this->taskVec[index.row()];

    switch (index.column()) {
      case 0:
        return ctx->title();

      case 1:
        return ctx->creationTime().toString();

      case 2:
        return ctx->progressMessage();

      case 3:
        return ctx->processingRate() > 0.
            ? SuWidgetsHelpers::formatQuantityNearest(
                ctx->processingRate(),
                2,
                "sp/s")
            : "N/A";

      case 4:
        return ctx->progressValue();

      case 5:
        return QString("Cancel");
    }
  }

  return QVariant();
}

QVariant
MultitaskControllerModel::headerData(int s, Qt::Orientation hor, int role) const
{
  if (hor == Qt::Horizontal && role == Qt::DisplayRole) {
    const char *headers[] = {
      "Description",
      "Creation time",
      "Status",
      "Rate",
      "Progress",
      "Actions"};

    if (s >= 0 && s < 6)
      return headers[s];
  }

  return QVariant();
}

///////////////////////////////// Slots ///////////////////////////////////////
void
MultitaskControllerModel::onListChanged(void)
{
  this->controller->getTaskVector(this->taskVec);
  this->controller->cleanup();
  emit layoutChanged();
}

void
MultitaskControllerModel::onError(int index, QString message)
{
  emit taskError(taskVec[index]->title(), message);
  this->controller->getTaskVector(this->taskVec);
  emit layoutChanged();
}

void
MultitaskControllerModel::onProgress(int index, qreal, QString)
{
  emit dataChanged(
        createIndex(index, 0, nullptr),
        createIndex(index, 3, nullptr));
}
