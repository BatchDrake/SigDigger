//
//    StdinSourcePage.cpp: description
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
#include "StdinSourcePage.h"
#include <SuWidgetsHelpers.h>
#include <suscan/util/cfg.h>
#include "ui_StdinSourcePage.h"

using namespace SigDigger;

StdinSourcePage::StdinSourcePage(
    SourceConfigWidgetFactory *factory,
    QWidget *parent) : SourceConfigWidget(factory, parent)
{
  ui = new Ui::StdinSourcePage();

  ui->setupUi(this);

  registerFormat("Complex 32-bit float",           "complex_float32");
  registerFormat("Complex 16-bit signed integer",  "complex_signed16");
  registerFormat("Complex 8-bit signed integer",   "complex_signed8");
  registerFormat("Complex 8-bit unsigned integer", "complex_unsigned8");

  registerFormat("Real 32-bit float",              "float32");
  registerFormat("Real 16-bit signed integer",     "signed16");
  registerFormat("Real 8-bit signed integer",      "signed8");
  registerFormat("Real 8-bit unsigned integer",    "unsigned8");

  connectAll();
}

StdinSourcePage::~StdinSourcePage()
{
  delete ui;
}

void
StdinSourcePage::registerFormat(QString const &name, QString const &desc)
{
  ui->formatCombo->addItem(name, desc);

  if (ui->formatCombo->currentIndex() == -1)
    ui->formatCombo->setCurrentIndex(0);
}

void
StdinSourcePage::connectAll()
{
  connect(
        ui->formatCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onConfigChanged()));

  connect(
        ui->realTimeCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onConfigChanged()));
}

uint64_t
StdinSourcePage::getCapabilityMask() const
{
  uint64_t perms = SUSCAN_ANALYZER_ALL_FILE_PERMISSIONS;

  perms &= ~SUSCAN_ANALYZER_PERM_SEEK;

  // Real-time sources cannot be throttled
  if (ui->realTimeCheck->isChecked())
    perms &= ~SUSCAN_ANALYZER_PERM_THROTTLE;

  return perms;
}

bool
StdinSourcePage::getPreferredRates(QList<int> &) const
{
  return false;
}

void
StdinSourcePage::refreshUi()
{
  if (m_config == nullptr)
    return;

  // Set format
  auto format = m_config->getParam("format");
  QString strFormat = QString::fromStdString(format);
  int index = ui->formatCombo->findData(strFormat);
  if (index == -1)
    index = 0;
  BLOCKSIG(ui->formatCombo, setCurrentIndex(index));

  // Set real time flag
  auto realtime = m_config->getParam("realtime");
  bool rt       = suscan_config_str_to_bool(realtime.c_str(), SU_FALSE) == SU_TRUE;
  BLOCKSIG(ui->realTimeCheck, setChecked(rt));
}

void
StdinSourcePage::activateWidget()
{
  refreshUi();
  emit changed();
}

bool
StdinSourcePage::deactivateWidget()
{
  return true;
}

void
StdinSourcePage::notifySingletonChanges()
{
}

void
StdinSourcePage::setConfigRef(Suscan::Source::Config &cfg)
{
  m_config = &cfg;
  refreshUi();
}


///////////////////////////////////// Slots ////////////////////////////////////
void
StdinSourcePage::onConfigChanged()
{
  if (m_config == nullptr)
    return;

  m_config->setParam("format", ui->formatCombo->currentData().value<QString>().toStdString());
  m_config->setParam("realtime", ui->realTimeCheck->isChecked() ? "yes" : "no");

  emit changed();
}
