//
//    DecoderDialog.h: Decoder chooser dialog
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

#include <DecoderDialog.h>
#include "ui_DecoderDialog.h"
#include <Suscan/Library.h>

using namespace SigDigger;

void
DecoderDialog::populate(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  for (auto p = sus->getFirstDecoderFactory();
       p != sus->getLastDecoderFactory();
       ++p) {
    this->cache.push_back(p->second);
    this->ui->decoderList->addItem(
          new QListWidgetItem(
            QIcon(":/decoder.png"),
            QString::fromStdString(p->second->getName()),
            this->ui->decoderList,
            QListWidgetItem::Type));
  }
}

DecoderDialog::DecoderDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DecoderDialog)
{
  ui->setupUi(this);

  this->populate();
}

bool
DecoderDialog::run(void)
{
  return this->exec() == DialogCode::Accepted;
}

Suscan::DecoderFactory *
DecoderDialog::getSelected(void) const
{
  if (this->ui->decoderList->currentRow() != -1)
    return this->cache[static_cast<unsigned>(this->ui->decoderList->currentRow())];

  return nullptr;
}

DecoderDialog::~DecoderDialog()
{
  delete ui;
}
