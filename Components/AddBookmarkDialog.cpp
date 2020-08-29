//
//    AddBookmarkDialog.cpp: Add a bookmark
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

#include <AddBookmarkDialog.h>
#include "ui_AddBookmarkDialog.h"
#include <QTimer>

using namespace SigDigger;

AddBookmarkDialog::AddBookmarkDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::AddBookmarkDialog)
{
  ui->setupUi(this);

  this->setFixedSize(window()->sizeHint());

  this->ui->frequencySpinBox->setMinimum(-300e9);
  this->ui->frequencySpinBox->setMaximum(+300e9);
  this->ui->frequencySpinBox->setAutoUnitMultiplierEnabled(true);

  this->setColorHint(QColor::fromRgb(0xff, 0xff, 0xff));
}

void
AddBookmarkDialog::setFrequencyHint(qint64 val)
{
  this->ui->frequencySpinBox->setValue(static_cast<qreal>(val));
}

void
AddBookmarkDialog::setNameHint(QString const &name)
{
  this->ui->nameEdit->setText(name);
}

void
AddBookmarkDialog::setColorHint(QColor const &color)
{
  this->ui->colorButton->setColor(color);
}

qint64
AddBookmarkDialog::frequency(void) const
{
  return static_cast<qint64>(this->ui->frequencySpinBox->value());
}

QString
AddBookmarkDialog::name(void) const
{
  return this->ui->nameEdit->text();
}

QColor
AddBookmarkDialog::color(void) const
{
  return this->ui->colorButton->getColor();
}

void
AddBookmarkDialog::showEvent(QShowEvent *)
{
  QTimer::singleShot(
        0,
        this->ui->nameEdit,
        SLOT(setFocus()));

  QTimer::singleShot(
        0,
        this->ui->nameEdit,
        SLOT(selectAll()));
}

AddBookmarkDialog::~AddBookmarkDialog()
{
  delete ui;
}
