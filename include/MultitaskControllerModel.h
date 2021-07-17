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
#ifndef MULTITASKCONTROLLERMODEL_H
#define MULTITASKCONTROLLERMODEL_H

#include <QAbstractTableModel>
#include <Suscan/MultitaskController.h>

namespace SigDigger {
  class MultitaskControllerModel : public QAbstractTableModel
  {
      Q_OBJECT

      Suscan::MultitaskController *controller;
      QVector<Suscan::CancellableTaskContext *> taskVec;

      void connectAll(void);

    public:
      MultitaskControllerModel(
          QObject *parent,
          Suscan::MultitaskController *ctl);

      int rowCount(const QModelIndex &) const override;
      int columnCount(const QModelIndex &) const override;
      QVariant data(const QModelIndex &, int) const override;
      QVariant headerData(int, Qt::Orientation, int) const override;

    signals:
      void taskError(QString, QString);

    public slots:
      void onListChanged(void);
      void onError(int, QString);
      void onProgress(int, qreal, QString);
  };
}

#endif // MULTITASKCONTROLLERMODEL_H
