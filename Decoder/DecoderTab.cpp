//
//    DecoderTab.cpp: Stream decoder tab UI
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

#include <DecoderTab.h>
#include <LayerItem.h>
#include <Suscan/DecoderFactory.h>
#include "ui_DecoderTab.h"

using namespace SigDigger;

Q_DECLARE_METATYPE(Suscan::DecoderObjects *);

void
DecoderTab::rebuildStack(void)
{
  std::vector<SigDigger::Decoder *> failed;

  this->stack.clear();

  for (auto i = 0; i < this->ui->decoderEditor->size(); ++i) {
    LayerItem &item = this->ui->decoderEditor->get(i);
    Suscan::DecoderObjects *objects =
        item.data().value<Suscan::DecoderObjects *>();

    // Whiteouts are like this
    if (objects != nullptr) {
      item.setFailed(false);
      objects->userData = &item;
      this->stack.push(static_cast<SigDigger::Decoder *>(objects->decoder));
    }
  }

  if (this->termination)
    this->stack.push(this->termination);

  this->ready = this->stack.connect(failed);

  if (!this->ready) {
    for (unsigned i = 0; i < failed.size(); ++i) {
      if (failed[i] != this->termination) {
        LayerItem *item =
          static_cast<LayerItem *>(failed[i]->getDecoderObjects()->userData);
        item->setFailed(true);
      }
    }
  }

  emit changed();
}

void
DecoderTab::connectAll(void)
{
  connect(
        this->ui->decoderEditor,
        SIGNAL(addEntry(void)),
        this,
        SLOT(onAddDecoder(void)));

  connect(
        this->ui->decoderEditor,
        SIGNAL(removeEntry(int)),
        this,
        SLOT(onRemoveDecoder(int)));

  connect(
        this->ui->decoderEditor,
        SIGNAL(reorderEntry(int, int)),
        this,
        SLOT(onMoveDecoder(int, int)));

  connect(
        this->ui->decoderEditor,
        SIGNAL(addEntry(void)),
        this,
        SLOT(onAddDecoder(void)));

  connect(
        this->ui->toggleDecodeButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleDecoder(void)));
}

DecoderTab::DecoderTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::DecoderTab)
{
  ui->setupUi(this);

  this->connectAll();
}

void
DecoderTab::setInputBps(uint8_t bps)
{
  this->stack.setBps(bps);

  this->rebuildStack();
}

uint8_t
DecoderTab::getOutputBps(void) const
{
  return this->stack.getBps();
}

void
DecoderTab::feed(const Symbol *samples, size_t len)
{
  this->stack.feed(samples, len);
  this->stack.flush(); // Optional
}

bool
DecoderTab::isEnabled(void) const
{
  return this->enabled;
}

bool
DecoderTab::isReady(void) const
{
  return this->ready;
}

void
DecoderTab::setTerminationDecoder(SigDigger::Decoder *dec)
{
  this->termination = dec;
  this->rebuildStack();
}

////////////////////////////////// Slots ///////////////////////////////////////
void
DecoderTab::onAddDecoder(void)
{
  if (this->decoderDialog.run()) {
    Suscan::DecoderFactory *factory;

    if ((factory = this->decoderDialog.getSelected()) != nullptr) {
      Suscan::DecoderObjects *objects = factory->make();
      LayerItem item;

      printf("Objects: %p\b", objects);
      item.setData(QVariant::fromValue(objects));
      item.setName(QString::fromStdString(factory->getName()));
      item.setDescription(QString::fromStdString(factory->getDescription()));

      this->ui->decoderEditor->add(item);
      this->rebuildStack();
    }
  }
}

void
DecoderTab::onToggleDecoder(void)
{
  this->enabled = this->ui->toggleDecodeButton->isChecked();
  this->rebuildStack();
  emit toggled();
}

void
DecoderTab::onMoveDecoder(int, int)
{
  this->rebuildStack();
}

void
DecoderTab::onRemoveDecoder(int i)
{
  LayerItem &item = this->ui->decoderEditor->get(i);
  Suscan::DecoderObjects *objects =
      item.data().value<Suscan::DecoderObjects *>();

  printf("Objects: %p\n", objects);
  delete objects;

  // Take this as a whiteout
  item.setData(QVariant::fromValue<Suscan::DecoderObjects *>(nullptr));

  this->rebuildStack();
}

DecoderTab::~DecoderTab()
{
  while (this->ui->decoderEditor->size() > 0)
    this->ui->decoderEditor->remove(0);

  delete ui;
}
