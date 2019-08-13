//
//    Inspector.h: Inspector object
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

#include "InspectorUI.h"
#include "Inspector.h"

using namespace SigDigger;

Inspector::Inspector(
    QWidget *parent,
    const Suscan::InspectorMessage &msg,
    ColorConfig const &config) :
  QWidget(parent),
  config(msg.getCConfig())
{
  this->handle = msg.getHandle();
  this->analyzer = nullptr;

  this->ui = std::make_unique<InspectorUI>(this, &this->config);
  this->ui->setColors(config);

  this->connect(
        this->ui.get(),
        SIGNAL(configChanged()),
        this,
        SLOT(onConfigChanged()));

  this->connect(
        this->ui.get(),
        SIGNAL(setSpectrumSource(unsigned int)),
        this,
        SLOT(onSetSpectrumSource(unsigned int)));

  for (auto p = msg.getSpectrumSources().begin();
       p != msg.getSpectrumSources().end();
       ++p)
    this->ui->addSpectrumSource(*p);

  for (auto p = msg.getEstimators().begin();
       p != msg.getEstimators().end();
       ++p)
    this->ui->addEstimator(*p);
}

void
Inspector::setAnalyzer(Suscan::Analyzer *analyzer)
{
  this->analyzer = analyzer;
  this->ui->setState(
        this->analyzer == nullptr
        ? InspectorUI::DETACHED
        : InspectorUI::ATTACHED);
}

void
Inspector::feed(const SUCOMPLEX *data, unsigned int size)
{
  this->ui->feed(data, size);
}

void
Inspector::feedSpectrum(const SUFLOAT *data, SUSCOUNT len, SUSCOUNT rate)
{
  if (len > 0)
    this->ui->feedSpectrum(data, len, rate);
}

Inspector::~Inspector()
{

}

/////////////////////////////////// Slots /////////////////////////////////////
void
Inspector::onConfigChanged(void)
{
  // TODO: Send config
  if (this->analyzer != nullptr) {
    this->analyzer->setInspectorConfig(
          this->handle,
          this->config,
          static_cast<Suscan::RequestId>(rand()));
  }
}

void
Inspector::onSetSpectrumSource(unsigned int index)
{
  if (this->analyzer != nullptr)
    this->analyzer->setSpectrumSource(
        this->handle,
        index,
        static_cast<Suscan::RequestId>(rand()));
}
