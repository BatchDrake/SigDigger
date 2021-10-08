//
//    TLESourceTab.cpp: TLE source tab
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
#include "TLESourceTab.h"
#include "ui_TLESourceTab.h"
#include <QMessageBox>

using namespace SigDigger;

void
TLESourceTab::save()
{
  this->tleSourceConfig.autoDownloadOnStartup =
      this->ui->autoDownloadCheck->isChecked();
}

void
TLESourceTab::refreshUi()
{
  this->ui->autoDownloadCheck->setChecked(this->tleSourceConfig.autoDownloadOnStartup);

  this->ui->removeTLESourceButton->setEnabled(
        this->ui->tleSourceTable->currentRow() >= 0);
  this->ui->downloadButton->setEnabled(
        !this->downloading && this->ui->tleSourceTable->rowCount() > 0);
  this->ui->abortDownloadButton->setEnabled(this->downloading);
}

void
TLESourceTab::setTleSourceConfig(TLESourceConfig const &config)
{
  this->tleSourceConfig = config;
  this->refreshUi();
  this->modified = false;
}

TLESourceConfig
TLESourceTab::getTleSourceConfig(void) const
{
  return this->tleSourceConfig;
}

bool
TLESourceTab::hasChanged(void) const
{
  return this->modified;
}

void
TLESourceTab::connectAll(void)
{
  connect(
        this->ui->autoDownloadCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onConfigChanged(void)));

  connect(
        this->ui->addTLESourceButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onAddTLESource(void)));

  connect(
        this->ui->removeTLESourceButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onRemoveTLESource(void)));

  connect(
        this->ui->tleSourceTable,
        SIGNAL(cellClicked(int, int)),
        this,
        SLOT(onTLESelectionChanged(void)));
}

void
TLESourceTab::populateTLESourceTable(void)
{
  auto sus = Suscan::Singleton::get_instance();
  for (auto src : sus->getTLESourceMap()) {
    int index = this->ui->tleSourceTable->rowCount();
    this->ui->tleSourceTable->insertRow(index);
    this->ui->tleSourceTable->setItem(
          index,
          0,
          new QTableWidgetItem(QString::fromStdString(src.name)));

    this->ui->tleSourceTable->setItem(
          index,
          1,
          new QTableWidgetItem(QString::fromStdString(src.url)));
  }
  this->ui->tleSourceTable->resizeColumnsToContents();
}


TLESourceTab::TLESourceTab(QWidget *parent) :
  ConfigTab(parent, "TLE Sources"),
  ui(new Ui::TLESourceTab)
{
  ui->setupUi(this);

  this->addDialog = new AddTLESourceDialog(this);
  this->populateTLESourceTable();
  this->connectAll();
  this->refreshUi();
}

TLESourceTab::~TLESourceTab()
{
  delete ui;
}

////////////////////////////////// Slots ///////////////////////////////////////
void
TLESourceTab::onConfigChanged(void)
{
  this->modified = true;
  emit changed();
}

void
TLESourceTab::onAddTLESource(void)
{
  if (this->addDialog->run()) {
    auto sus = Suscan::Singleton::get_instance();
    Suscan::TLESource src = this->addDialog->getTLESource();
    if (sus->registerTLESource(src)) {
      int index = this->ui->tleSourceTable->rowCount();
      this->ui->tleSourceTable->insertRow(index);
      this->ui->tleSourceTable->setItem(
            index,
            0,
            new QTableWidgetItem(QString::fromStdString(src.name)));

      this->ui->tleSourceTable->setItem(
            index,
            1,
            new QTableWidgetItem(QString::fromStdString(src.url)));

      this->ui->tleSourceTable->resizeColumnsToContents();
      this->ui->tleSourceTable->scrollToBottom();
      this->refreshUi();
    } else {
      QMessageBox::warning(
            this,
            "There is another source named " + QString::fromStdString(src.name)
            + ". please choose a different name.",
            "Add TLE source");
    }
  }
}

void
TLESourceTab::onRemoveTLESource(void)
{
  if (this->ui->tleSourceTable->currentRow() >= 0) {
    auto sus = Suscan::Singleton::get_instance();

    QString text = this->ui->tleSourceTable->itemAt(
          0,
          this->ui->tleSourceTable->currentRow())->text();
    if (sus->removeTLESource(text.toStdString())) {
      this->ui->tleSourceTable->removeRow(
            this->ui->tleSourceTable->currentRow());
      this->refreshUi();
    } else {
      QMessageBox::warning(
            this,
            "Source " + text + " is a default source and cannot be removed.",
            "Remove TLE source");
    }
  }
}

void
TLESourceTab::onTLESelectionChanged(void)
{
  this->refreshUi();
}
