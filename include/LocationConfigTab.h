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

#include <ConfigTab.h>
#include <QMap>
#include <QListWidgetItem>
#include <Suscan/Library.h>

namespace Ui {
  class LocationConfigTab;
}

namespace SigDigger {
  class LocationConfigTab : public ConfigTab
  {
    Q_OBJECT

    Suscan::Location current;
    bool modified = false;
    QMap<QString, int> countryList;
    void paintMapCoords(double x, double y);
    void populateLocations(void);
    void repaintCountryList(QString searchText = "");
    void connectAll(void);

  public:
    void save() override;
    bool hasChanged(void) const override;

    Suscan::Location getLocation(void) const;
    void setLocation(Suscan::Location const &);

    explicit LocationConfigTab(QWidget *parent = nullptr);
    ~LocationConfigTab();

  private:
    Ui::LocationConfigTab *ui;

  public slots:
    void onSearchTextChanged(void);
    void onLocationSelected(QListWidgetItem *item);
    void onLocationChanged(void);
    void onRegisterLocation(void);

  };
}

#endif // LOCATIONCONFIGTAB_H
