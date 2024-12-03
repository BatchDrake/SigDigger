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
DeviceTweaks::connectAll()
{
  QPushButton *resetButton = ui->buttonBox->button(
        QDialogButtonBox::Reset);

  connect(
        ui->addButton,
        SIGNAL(clicked()),
        this,
        SLOT(onAddEntry()));

  connect(
        ui->removeButton,
        SIGNAL(clicked()),
        this,
        SLOT(onRemoveEntry()));

  connect(
        ui->tableWidget->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this,
        SLOT(onSelectionChanged(const QItemSelection &, const QItemSelection &)));

  connect(
        ui->tableWidget,
        SIGNAL(itemChanged(QTableWidgetItem *)),
        this,
        SLOT(onChanged()));


  connect(
        resetButton,
        SIGNAL(clicked()),
        this,
        SLOT(onReset()));
}

void
DeviceTweaks::refreshUi()
{
  ui->tableWidget->model()->removeRows(
        0,
        ui->tableWidget->model()->rowCount());

  // Insert current data
  if (m_profile != nullptr) {
    int i = 0;
    auto list = m_profile->getParamList();

    for (auto p : list) {
      ui->tableWidget->insertRow(i);
      ui->tableWidget->setItem(
            i,
            0,
            new QTableWidgetItem(p.first.c_str()));
      ui->tableWidget->setItem(
            i,
            1,
            new QTableWidgetItem(p.second.c_str()));
      ++i;
    }
  }

  ui->removeButton->setEnabled(false);
  setChanged(false);
}

void
DeviceTweaks::setChanged(bool changed)
{
  m_changed = changed;

  if (m_profile != nullptr) {
    auto spec = m_profile->getDeviceSpec();
    auto props = spec.properties();

    std::string desc = props == nullptr ? "(device not found)" : props->label();

    if (changed)
      setWindowTitle(QString::fromStdString(desc) + " [changed]");
    else
      setWindowTitle(QString::fromStdString(desc));
  }
}

void
DeviceTweaks::commitConfig()
{
  if (m_profile != nullptr) {
    auto model = ui->tableWidget->model();
    int rows = model->rowCount();

    m_profile->clearParams();

    for (int i = 0; i < rows; ++i) {
      auto keyItem = ui->tableWidget->item(i, 0);
      auto valItem = ui->tableWidget->item(i, 1);
      if (keyItem != nullptr) { 
        auto key = keyItem->text().toStdString();

        if (valItem != nullptr) {
          auto val = valItem->text().toStdString();
          m_profile->setParam(key, val);
        } else {
          m_profile->setParam(key, "");
        }
      }
    }
  }
}

void
DeviceTweaks::setProfile(Suscan::Source::Config *profile)
{
  m_profile = profile;
  refreshUi();
}

bool
DeviceTweaks::hasChanged() const
{
  return m_changed;
}

DeviceTweaks::DeviceTweaks(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DeviceTweaks)
{
  ui->setupUi(this);
  connectAll();
}

DeviceTweaks::~DeviceTweaks()
{
  delete ui;
}

/////////////////////////////////// Slots //////////////////////////////////////
void
DeviceTweaks::onAddEntry()
{
  ui->tableWidget->insertRow(ui->tableWidget->rowCount());
  m_changed = true;
}

void
DeviceTweaks::onRemoveEntry()
{
  QList<QTableWidgetSelectionRange> ranges =
      ui->tableWidget->selectedRanges();

  if (!ranges.isEmpty()) {
    int begin = ranges.at(0).topRow();
    int count = ranges.at(0).rowCount();

    do
      ui->tableWidget->removeRow(begin);
    while (--count);

    setChanged(true);
  }
}

void
DeviceTweaks::onReset()
{
  refreshUi();
}

void
DeviceTweaks::onSelectionChanged(const QItemSelection &, const QItemSelection &)
{
  ui->removeButton->setEnabled(
        !ui->tableWidget->selectedItems().isEmpty());
}

void
DeviceTweaks::onChanged()
{
  setChanged(true);
}
