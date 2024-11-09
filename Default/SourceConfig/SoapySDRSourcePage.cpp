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
#include <Suscan/Device.h>
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
  int currentIndex = ui->deviceCombo->currentIndex();
  int newIndex = -1;
  int p = 0;
  QString prevName, name;

  if (currentIndex != -1)
    prevName = ui->deviceCombo->currentText();

  ui->deviceCombo->clear();

  for (auto &dev: Suscan::DeviceFacade::instance()->devices()) {
    if (dev.analyzer() == "local" && dev.source() == "soapysdr") {
      name = QString::fromStdString(dev.label());
      if (currentIndex != -1 && newIndex == -1 && name == prevName)
        newIndex = p;

      ui->deviceCombo->addItem(
          name,
          QVariant::fromValue<uint64_t>(dev.uuid()));
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
SoapySDRSourcePage::saveCurrentSpecs()
{
  auto currentUuid = m_config->getDeviceSpec().uuid();

  if (currentUuid != SUSCAN_DEVICE_UUID_INVALID)
    m_savedSpecs[currentUuid] = m_config->getDeviceSpec();
}

void
SoapySDRSourcePage::setSavedDeviceSpec(uint64_t uuid)
{
  auto prop = Suscan::DeviceFacade::instance()->deviceByUuid(uuid);

  saveCurrentSpecs();

  if (prop != nullptr) {
    if (m_savedSpecs.contains(uuid))
      m_config->setDeviceSpec(m_savedSpecs[uuid]);
    else
      m_config->setDeviceSpec(Suscan::DeviceSpec(*prop));

    auto antennas = prop->antennas();
    if (!antennas.empty() &&
        std::find(antennas.begin(), antennas.end(), m_config->getAntenna())
        == antennas.end())
      m_config->setAntenna(antennas.front());
  }
}

bool
SoapySDRSourcePage::getPreferredRates(QList<int> &list) const
{
  if (m_config == nullptr)
    return false;

  auto dev = m_config->getDeviceSpec();
  auto prop = dev.properties();

  list.clear();

  if (prop != nullptr)
    for (auto &rate : prop->sampleRates())
      list.append(SCAST(int, rate));

  return list.size() > 0;
}

void
SoapySDRSourcePage::refreshUi()
{
  if (m_config == nullptr)
    return;

  BLOCKSIG(ui->bandwidthSpinBox, setValue(
        static_cast<double>(m_config->getBandwidth())));

  uint64_t currUuid = m_config->getDeviceSpec().uuid();
  int index = ui->deviceCombo->findData(QVariant::fromValue(currUuid));

  if (index != -1)
    BLOCKSIG(ui->deviceCombo, setCurrentIndex(index));
  else
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
  return true;
}

void
SoapySDRSourcePage::notifySingletonChanges()
{
  populateDeviceCombo();
}

void
SoapySDRSourcePage::setConfigRef(Suscan::Source::Config &cfg)
{
  populateDeviceCombo();

  m_config = &cfg;
  m_savedSpecs.clear();

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
    saveCurrentSpecs();
    emit changed();
  }
}

void
SoapySDRSourcePage::onDeviceChanged(int index)
{
  if (index != -1) {
    uint64_t selectedUuid = ui->deviceCombo->currentData().value<uint64_t>();
    setSavedDeviceSpec(selectedUuid);
    refreshUi();
    emit changed();
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
