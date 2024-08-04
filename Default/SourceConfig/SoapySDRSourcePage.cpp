//
//    SoapySDRSourcePage.cpp: description
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

#include "SoapySDRSourcePage.h"
#include "ui_SoapySDRSourcePage.h"
#include "DeviceTweaks.h"
#include "SigDiggerHelpers.h"

#include <QMessageBox>

using namespace SigDigger;

SoapySDRSourcePage::SoapySDRSourcePage(
    SourceConfigWidgetFactory *factory,
    QWidget *parent) : SourceConfigWidget(factory, parent)
{
  ui = new Ui::SoapySDRSourcePage();

  ui->setupUi(this);

  m_tweaks = new DeviceTweaks(this);
  m_tweaks->setModal(true);

  connectAll();
}

SoapySDRSourcePage::~SoapySDRSourcePage()
{
  delete ui;
}

void
SoapySDRSourcePage::connectAll()
{
  connect(
        ui->deviceCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onDeviceChanged(int)));

  connect(
        ui->antennaCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onAntennaChanged(int)));

  connect(
        ui->bandwidthSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onBandwidthChanged(double)));

  connect(
        m_tweaks,
        SIGNAL(accepted()),
        this,
        SLOT(onDeviceTweaksAccepted()));

  connect(
        ui->deviceTweaksButton,
        SIGNAL(clicked()),
        this,
        SLOT(onDeviceTweaksClicked()));
}

void
SoapySDRSourcePage::populateDeviceCombo()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  int currentIndex = ui->deviceCombo->currentIndex();
  int newIndex = -1;
  int p = 0;
  QString prevName, name;

  if (currentIndex != -1)
    prevName = ui->deviceCombo->currentText();

  ui->deviceCombo->clear();

  for (auto i = sus->getFirstDevice(); i != sus->getLastDevice(); ++i) {
    if (i->isAvailable() && !i->isRemote()) {
      name = QString::fromStdString(i->getDesc());
      if (currentIndex != -1 && newIndex == -1 && name == prevName)
        newIndex = p;

      ui->deviceCombo->addItem(
          name,
          QVariant::fromValue<long>(i - sus->getFirstDevice()));
      ++p;
    }
  }

  if (newIndex == -1)
    newIndex = 0;

  ui->deviceCombo->setCurrentIndex(newIndex);
}

void
SoapySDRSourcePage::refreshAntennas()
{
  SigDiggerHelpers::populateAntennaCombo(*m_config, ui->antennaCombo);
}

uint64_t
SoapySDRSourcePage::getCapabilityMask() const
{
  uint64_t perms = SUSCAN_ANALYZER_PERM_ALL;

  perms &= ~SUSCAN_ANALYZER_PERM_SEEK;
  perms &= ~SUSCAN_ANALYZER_PERM_THROTTLE;

  return perms;
}

void
SoapySDRSourcePage::selectDeviceByIndex(int index)
{
  // Remember: only set device if the analyzer type is local
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  const Suscan::Source::Device *device;

  SU_ATTEMPT(
        device = sus->getDeviceAt(
          static_cast<unsigned int>(
          ui->deviceCombo->itemData(index).value<long>())));

  m_savedLocalDeviceIndex = index;
  m_config->setDevice(*device);
  m_hasTweaks = false;

  auto begin = device->getFirstAntenna();
  auto end   = device->getLastAntenna();

  // We check whether we can keep the current antenna configuration. If we
  // cannot, just set the first antenna in the list.
  if (device->findAntenna(m_config->getAntenna()) == end
      && begin != end)
    m_config->setAntenna(*begin);
}

bool
SoapySDRSourcePage::getPreferredRates(QList<int> &list) const
{
  if (m_config == nullptr)
    return false;

  auto dev = m_config->getDevice();

  list.clear();

  for (auto p = dev.getFirstSampRate(); p != dev.getLastSampRate(); ++p)
    list.append(SCAST(int, *p));

  return list.size() > 0;
}

void
SoapySDRSourcePage::refreshUi()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  if (m_config == nullptr)
    return;

  BLOCKSIG(ui->bandwidthSpinBox, setValue(
        static_cast<double>(m_config->getBandwidth())));

  if (m_savedLocalDeviceIndex < 0) {
    for (auto i = sus->getFirstDevice(); i != sus->getLastDevice(); ++i) {
      if (i->equals(m_config->getDevice())) {
        int index = ui->deviceCombo->findData(
              QVariant::fromValue(
                static_cast<long>(i - sus->getFirstDevice())));
        if (index != -1) {
          BLOCKSIG(ui->deviceCombo, setCurrentIndex(index));
          m_savedLocalDeviceIndex = index;
        }

        break;
      }
    }
  } else {
    selectDeviceByIndex(m_savedLocalDeviceIndex);
  }

  if (ui->deviceCombo->currentIndex() == -1)
    BLOCKSIG(ui->deviceCombo, setCurrentIndex(0));

  refreshAntennas();
}

void
SoapySDRSourcePage::activateWidget()
{
  refreshUi();
}

bool
SoapySDRSourcePage::deactivateWidget()
{
  if (shouldDisregardTweaks()) {
    m_savedLocalDeviceIndex = ui->deviceCombo->currentIndex();
    return true;
  }

  return false;
}

void
SoapySDRSourcePage::notifySingletonChanges()
{
  populateDeviceCombo();
}

bool
SoapySDRSourcePage::shouldDisregardTweaks()
{
  QMessageBox::StandardButton reply;

  if (m_hasTweaks) {
    reply = QMessageBox::question(
          this,
          "Per-device tweaks",
          "This action will clear currently defined device tweaks. Are you sure?",
          QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
      return false;
  }

  m_hasTweaks = false;

  return true;
}

void
SoapySDRSourcePage::setConfigRef(Suscan::Source::Config &cfg)
{
  m_config = &cfg;
  m_hasTweaks = false;
  m_savedLocalDeviceIndex = -1;

  populateDeviceCombo();
  refreshUi();
}


///////////////////////////////////// Slots ////////////////////////////////////
void
SoapySDRSourcePage::onDeviceTweaksClicked()
{
  if (m_config == nullptr)
    return;

  m_tweaks->setProfile(m_config);
  m_tweaks->exec();
}

void
SoapySDRSourcePage::onDeviceTweaksAccepted()
{
  if (m_config == nullptr)
    return;

  if (m_tweaks->hasChanged()) {
    m_tweaks->commitConfig();
    m_hasTweaks = true;

    emit changed();
  }
}

void
SoapySDRSourcePage::onDeviceChanged(int index)
{
  if (shouldDisregardTweaks()) {
    selectDeviceByIndex(index);
    refreshUi();
    emit changed();
  } else if (m_savedLocalDeviceIndex >= 0) {
    BLOCKSIG(ui->deviceCombo, setCurrentIndex(m_savedLocalDeviceIndex));
  }
}

void
SoapySDRSourcePage::onAntennaChanged(int)
{
  m_config->setAntenna(
        ui->antennaCombo->currentText().toStdString());
  emit changed();
}

void
SoapySDRSourcePage::onBandwidthChanged(double)
{
  m_config->setBandwidth(
      static_cast<SUFLOAT>(
        ui->bandwidthSpinBox->value()));
  emit changed();
}
