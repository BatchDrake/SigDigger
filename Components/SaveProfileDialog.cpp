//
//    SaveProfileDialog.cpp: Save profile dialog
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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

#include "SaveProfileDialog.h"
#include "ui_SaveProfileDialog.h"

#include <QPushButton>

using namespace SigDigger;

SaveProfileDialog::SaveProfileDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SaveProfileDialog)
{
  ui->setupUi(this);

  connect(
        this->ui->profileNameLabel,
        SIGNAL(textChanged(const QString &)),
        this,
        SLOT(onTextChanged()));

  this->onTextChanged();
}

SaveProfileDialog::~SaveProfileDialog()
{
  delete ui;
}

bool
SaveProfileDialog::run(void)
{
  return this->exec() == QDialog::DialogCode::Accepted;
}

QString
SaveProfileDialog::getProfileName(void) const
{
  return this->ui->profileNameLabel->text();
}

void
SaveProfileDialog::setProfileName(QString const &name)
{
  this->ui->profileNameLabel->setText(name);
}

void
SaveProfileDialog::onTextChanged(void)
{
  this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
        this->ui->profileNameLabel->text().size() > 0);
}
