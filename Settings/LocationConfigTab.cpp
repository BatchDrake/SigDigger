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
#include <QPainter>
#include <QMessageBox>

using namespace SigDigger;

void
LocationConfigTab::connectAll(void)
{
  connect(
        this->ui->cityListWidget,
        SIGNAL(itemDoubleClicked(QListWidgetItem *)),
        this,
        SLOT(onLocationSelected(QListWidgetItem *)));

  connect(
        this->ui->cityNameEdit,
        SIGNAL(textChanged(const QString &)),
        this,
        SLOT(onLocationChanged(void)));

  connect(
        this->ui->countryCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onLocationChanged(void)));

  connect(
        this->ui->latitudeSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onLocationChanged(void)));

  connect(
        this->ui->longitudeSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onLocationChanged(void)));

  connect(
        this->ui->altitudeSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onLocationChanged(void)));

  connect(
        this->ui->searchEdit,
        SIGNAL(textEdited(const QString &)),
        this,
        SLOT(onSearchTextChanged(void)));

  connect(
        this->ui->addToListButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onRegisterLocation(void)));
}

void
LocationConfigTab::repaintCountryList(QString searchText)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  this->ui->cityListWidget->clear();

  for (auto i = sus->getFirstLocation(); i != sus->getLastLocation(); ++i) {
    QString locName = i->getLocationName();
    if (locName.contains(searchText)) {
      QListWidgetItem *item = new QListWidgetItem(locName);
      this->ui->cityListWidget->addItem(item);
    }
  }
}

void
LocationConfigTab::populateLocations(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  int index = 0;

  for (auto i = sus->getFirstLocation(); i != sus->getLastLocation(); ++i) {
    QString country;
    QListWidgetItem *item = new QListWidgetItem(i->getLocationName());
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

void
LocationConfigTab::paintMapCoords(double lat, double lon)
{
  QPixmap bg = QPixmap(":/icons/earthmap.png");
  QSize dim = bg.size();
  QPainter painter(&bg);
  int x, y;
  int hL = dim.width() / 100;

  lat = 90. - lat;
  lon = lon + 180.;

  y = static_cast<int>(lat * dim.height() / 180.);
  x = static_cast<int>(lon * dim.width()  / 360.);


  painter.setPen(QPen(Qt::red, hL / 2));
  painter.drawLine(x - hL, y - hL, x + hL, y + hL);
  painter.drawLine(x - hL, y + hL, x + hL, y - hL);

  this->ui->mapLabel->setPixmap(bg);
}

Suscan::Location
LocationConfigTab::getLocation(void) const
{
  return this->current;
}

void
LocationConfigTab::setLocation(Suscan::Location const &loc)
{
  this->current = loc;

  this->ui->latitudeSpinBox->setValue(loc.site.lat);
  this->ui->longitudeSpinBox->setValue(loc.site.lon);
  this->ui->altitudeSpinBox->setValue(loc.site.height * 1e3);
  this->ui->cityNameEdit->setText(QString::fromStdString(loc.name));

  if (this->countryList.find(QString::fromStdString(loc.country)) !=
      this->countryList.end())
    this->ui->countryCombo->setCurrentIndex(
        this->countryList[QString::fromStdString(loc.country)]);

  this->modified = false;
}

bool
LocationConfigTab::hasChanged(void) const
{
  return this->modified;
}

void
LocationConfigTab::save(void)
{
  this->current.name    = this->ui->cityNameEdit->text().toStdString();
  this->current.country = this->ui->countryCombo->currentText().toStdString();

  this->current.site.height = this->ui->altitudeSpinBox->value() * 1e-3;
  this->current.site.lat    = this->ui->latitudeSpinBox->value();
  this->current.site.lon    = this->ui->longitudeSpinBox->value();
}

LocationConfigTab::LocationConfigTab(QWidget *parent) :
  ConfigTab(parent, "Location"),
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

void
LocationConfigTab::onLocationChanged(void)
{
  this->modified = true;
  this->paintMapCoords(
        this->ui->latitudeSpinBox->value(),
        this->ui->longitudeSpinBox->value());
  emit changed();
}

void
LocationConfigTab::onSearchTextChanged(void)
{
  this->repaintCountryList(this->ui->searchEdit->text());
}

void
LocationConfigTab::onRegisterLocation(void)
{
  Suscan::Location loc;
  auto sus = Suscan::Singleton::get_instance();
  auto locMap = Suscan::Singleton::get_instance()->getLocationMap();


  loc.name        = this->ui->cityNameEdit->text().toStdString();
  loc.country     = this->ui->countryCombo->currentText().toStdString();
  loc.site.height = this->ui->altitudeSpinBox->value() * 1e-3;
  loc.site.lat    = this->ui->latitudeSpinBox->value();
  loc.site.lon    = this->ui->longitudeSpinBox->value();

  QString locName = loc.getLocationName();
  if (locMap.find(locName) != locMap.end()) {
    QMessageBox::warning(
          this,
          "Register location",
          "Location "
          + locName
          + " already exists. Please choose a different name.");
  } else {
    sus->registerLocation(loc);
    QListWidgetItem *item = new QListWidgetItem(loc.getLocationName());
    this->ui->cityListWidget->addItem(item);
    this->ui->cityListWidget->scrollToBottom();
  }
}
