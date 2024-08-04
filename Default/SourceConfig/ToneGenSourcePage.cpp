//
//    ToneGenSourcePage.cpp: description
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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
#include "ToneGenSourcePage.h"
#include "ui_ToneGenSourcePage.h"

using namespace SigDigger;

ToneGenSourcePage::ToneGenSourcePage(
    SourceConfigWidgetFactory *factory,
    QWidget *parent) : SourceConfigWidget(factory, parent)
{
  ui = new Ui::ToneGenSourcePage();

  ui->setupUi(this);

  connectAll();
}

ToneGenSourcePage::~ToneGenSourcePage()
{
  delete ui;
}

void
ToneGenSourcePage::connectAll()
{
  connect(
        ui->noiseSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onConfigChanged()));

  connect(
        ui->signalSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onConfigChanged()));
}

uint64_t
ToneGenSourcePage::getCapabilityMask() const
{
  uint64_t perms = SUSCAN_ANALYZER_PERM_ALL;

  perms &= ~SUSCAN_ANALYZER_PERM_SEEK;
  perms &= ~SUSCAN_ANALYZER_PERM_THROTTLE;

  return perms;
}

bool
ToneGenSourcePage::getPreferredRates(QList<int> &) const
{
  return false;
}

void
ToneGenSourcePage::refreshUi()
{
  qreal value;
  if (m_config == nullptr)
    return;

  value = SU_DB_RAW(5e-3f);
  auto noise = m_config->getParam("noise");
  if (noise.size() > 0)
    sscanf(noise.c_str(), "%lg", &value);
  ui->noiseSpinBox->setValue(value);

  value = SU_DB_RAW(5e-1f);
  auto signal = m_config->getParam("signal");
  if (signal.size() > 0)
    sscanf(signal.c_str(), "%lg", &value);
  ui->signalSpinBox->setValue(value);
}

void
ToneGenSourcePage::activateWidget()
{
  refreshUi();
}

bool
ToneGenSourcePage::deactivateWidget()
{
  return true;
}

void
ToneGenSourcePage::notifySingletonChanges()
{
}

void
ToneGenSourcePage::setConfigRef(Suscan::Source::Config &cfg)
{
  m_config = &cfg;
  refreshUi();
}


///////////////////////////////////// Slots ////////////////////////////////////
void
ToneGenSourcePage::onConfigChanged()
{
  if (m_config == nullptr)
    return;

  m_config->setParam("noise", std::to_string(ui->noiseSpinBox->value()));
  m_config->setParam("signal", std::to_string(ui->signalSpinBox->value()));

  emit changed();
}
