//
//    LocationConfigTab.h: QTH configuration
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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
#ifndef LOCATIONCONFIGTAB_H
#define LOCATIONCONFIGTAB_H

#include <QWidget>
#include <QMap>
#include <QListWidgetItem>

namespace Ui {
  class LocationConfigTab;
}

namespace SigDigger {
  class LocationConfigTab : public QWidget
  {
    Q_OBJECT

    QMap<QString, int> countryList;
    void populateLocations(void);
    void connectAll(void);

  public:
    void save();
    explicit LocationConfigTab(QWidget *parent = nullptr);
    ~LocationConfigTab();

  private:
    Ui::LocationConfigTab *ui;

  public slots:
    void onLocationSelected(QListWidgetItem *item);
  };
}

#endif // LOCATIONCONFIGTAB_H
