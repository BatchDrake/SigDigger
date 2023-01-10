//
//    DeviceTweaks.cpp: Tweak device-specific settings
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
#include "DeviceTweaks.h"
#include "ui_DeviceTweaks.h"

using namespace SigDigger;

void
DeviceTweaks::connectAll(void)
{
  QPushButton *resetButton = this->ui->buttonBox->button(
        QDialogButtonBox::Reset);

  connect(
        this->ui->addButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onAddEntry(void)));

  connect(
        this->ui->removeButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onRemoveEntry(void)));

  connect(
        this->ui->tableWidget->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this,
        SLOT(onSelectionChanged(const QItemSelection &, const QItemSelection &)));

  connect(
        this->ui->tableWidget,
        SIGNAL(itemChanged(QTableWidgetItem *)),
        this,
        SLOT(onChanged()));


  connect(
        resetButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onReset()));
}

void
DeviceTweaks::refreshUi(void)
{
  this->ui->tableWidget->model()->removeRows(
        0,
        this->ui->tableWidget->model()->rowCount());

  // Insert current data
  if (this->profile != nullptr) {
    int i = 0;
    auto list = this->profile->getParamList();

    for (auto p : list) {
      this->ui->tableWidget->insertRow(i);
      this->ui->tableWidget->setItem(
            i,
            0,
            new QTableWidgetItem(p.first.c_str()));
      this->ui->tableWidget->setItem(
            i,
            1,
            new QTableWidgetItem(p.second.c_str()));
      ++i;
    }
  }

  this->ui->removeButton->setEnabled(false);
  this->setChanged(false);
}

void
DeviceTweaks::setChanged(bool changed)
{
  this->changed = changed;

  if (this->profile != nullptr) {
    std::string desc = this->profile->getDevice().getDesc();

    if (desc.size() == 0)
      desc = "(No device description)";

    if (changed)
      this->setWindowTitle(QString::fromStdString(desc) + " [changed]");
    else
      this->setWindowTitle(QString::fromStdString(desc));
  }
}

void
DeviceTweaks::commitConfig(void)
{
  if (this->profile != nullptr) {
    auto model = this->ui->tableWidget->model();
    int rows = model->rowCount();

    this->profile->clearParams();

    for (int i = 0; i < rows; ++i) {
      auto keyItem = this->ui->tableWidget->item(i, 0);
      auto valItem = this->ui->tableWidget->item(i, 1);
      if (keyItem != nullptr) { 
        auto key = keyItem->text().toStdString();

        if (valItem != nullptr) {
          auto val = valItem->text().toStdString();
          this->profile->setParam(key, val);
        } else {
          this->profile->setParam(key, "");
        }
      }
    }
  }
}

void
DeviceTweaks::setProfile(Suscan::Source::Config *profile)
{
  this->profile = profile;
  this->refreshUi();
}

bool
DeviceTweaks::hasChanged(void) const
{
  return this->changed;
}

DeviceTweaks::DeviceTweaks(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DeviceTweaks)
{
  ui->setupUi(this);
  this->connectAll();
}

DeviceTweaks::~DeviceTweaks()
{
  delete ui;
}

/////////////////////////////////// Slots //////////////////////////////////////
void
DeviceTweaks::onAddEntry(void)
{
  this->ui->tableWidget->insertRow(this->ui->tableWidget->rowCount());
  this->changed = true;
}

void
DeviceTweaks::onRemoveEntry(void)
{
  QList<QTableWidgetSelectionRange> ranges =
      this->ui->tableWidget->selectedRanges();

  if (!ranges.isEmpty()) {
    int begin = ranges.at(0).topRow();
    int count = ranges.at(0).rowCount();

    do
      this->ui->tableWidget->removeRow(begin);
    while (--count);

    this->setChanged(true);
  }
}

void
DeviceTweaks::onReset(void)
{
  this->refreshUi();
}

void
DeviceTweaks::onSelectionChanged(const QItemSelection &, const QItemSelection &)
{
  this->ui->removeButton->setEnabled(
        !this->ui->tableWidget->selectedItems().isEmpty());
}

void
DeviceTweaks::onChanged(void)
{
  this->setChanged(true);
}
