//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
#include "AddTLESourceDialog.h"
#include "ui_AddTLESourceDialog.h"
#include <QPushButton>
#include <regex>

using namespace SigDigger;

bool
AddTLESourceDialog::urlIsValid(QString const &url)
{
  const std::regex pattern(
        "((http|https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}"
        "\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
  std::string asStdString = url.toStdString();

  if (url.isEmpty())
    return false;

  return std::regex_match(asStdString.c_str(), pattern);
}

void
AddTLESourceDialog::connectAll(void)
{
  connect(
        this->ui->nameEdit,
        SIGNAL(textChanged(const QString &)),
        this,
        SLOT(onFieldsChanged(void)));

  connect(
        this->ui->urlEdit,
        SIGNAL(textChanged(const QString &)),
        this,
        SLOT(onFieldsChanged(void)));
}

Suscan::TLESource
AddTLESourceDialog::getTLESource(void) const
{
  Suscan::TLESource tleSrc;

  tleSrc.name = this->ui->nameEdit->text().toStdString();
  tleSrc.url  = this->ui->urlEdit->text().toStdString();

  return tleSrc;
}

bool
AddTLESourceDialog::run(void)
{
  this->setWindowTitle("Add TLE source");

  this->reset();
  this->ui->nameEdit->setFocus();
  return this->exec() == DialogCode::Accepted;
}

void
AddTLESourceDialog::reset(void)
{
  this->ui->nameEdit->setText("");
  this->ui->urlEdit->setText("");
}

AddTLESourceDialog::AddTLESourceDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::AddTLESourceDialog)
{
  ui->setupUi(this);

  this->connectAll();

  this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

AddTLESourceDialog::~AddTLESourceDialog()
{
  delete ui;
}

/////////////////////// Slots ///////////////////////
void
AddTLESourceDialog::onFieldsChanged(void)
{
  bool isValid = false;

  if (this->ui->nameEdit->text().isEmpty()) {
    this->ui->nameEdit->setToolTip("Enter a non-empty string describing the source");
    this->ui->nameEdit->toolTip();
  } else if (!this->urlIsValid(this->ui->urlEdit->text())) {
    this->ui->urlEdit->setToolTip("Enter a valid URL (starting by http:// or https://)");
    this->ui->urlEdit->toolTip();
  } else {
    isValid = true;
  }

  this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);
}
