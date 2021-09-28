//
//    LocationConfigTab.cpp: QTH configuration
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
#include "LocationConfigTab.h"
#include "ui_LocationConfigTab.h"
#include <Suscan/Library.h>

using namespace SigDigger;

void
LocationConfigTab::connectAll(void)
{
  connect(
        this->ui->cityListWidget,
        SIGNAL(itemDoubleClicked(QListWidgetItem *)),
        this,
        SLOT(onLocationSelected(QListWidgetItem *)));
}

void
LocationConfigTab::populateLocations(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  int index = 0;

  for (auto i = sus->getFirstLocation(); i != sus->getLastLocation(); ++i) {
    QString country;
    QListWidgetItem *item = new QListWidgetItem;
    item->setText(i->getLocationName());
    this->ui->cityListWidget->addItem(item);

    country = QString::fromStdString(i->country);

    if (this->countryList.find(country) == this->countryList.end())
      this->countryList.insert(country, -1);
  }

  for (auto p = this->countryList.begin(); p != this->countryList.end(); ++p) {
    *p = index++;
    this->ui->countryCombo->insertItem(*p, p.key());
  }
}

LocationConfigTab::LocationConfigTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::LocationConfigTab)
{
  ui->setupUi(this);

  // Configure search box
  this->ui->searchEdit->setClearButtonEnabled(true);
  this->ui->searchEdit->addAction(
        QIcon(":/icons/system-search.png"),
        QLineEdit::LeadingPosition);

  // Populate combos
  this->populateLocations();

  this->connectAll();
}

LocationConfigTab::~LocationConfigTab()
{
  delete ui;
}

//////////////////////////////// Slots /////////////////////////////////////////
void
LocationConfigTab::onLocationSelected(QListWidgetItem *item)
{
  QString fullName = item->text();
  auto locMap = Suscan::Singleton::get_instance()->getLocationMap();

  if (locMap.find(fullName) != locMap.end()) {
    auto loc = locMap[fullName];
    QString country = QString::fromStdString(loc.country);
    int index = -1;

    this->ui->latitudeSpinBox->setValue(loc.site.lat);
    this->ui->longitudeSpinBox->setValue(loc.site.lon);
    this->ui->altitudeSpinBox->setValue(loc.site.height * 1e3);
    this->ui->cityNameEdit->setText(QString::fromStdString(loc.name));

    if (this->countryList.find(country) != this->countryList.end()
        && (index = this->countryList[country]) != -1)
      this->ui->countryCombo->setCurrentIndex(index);
  }
}
