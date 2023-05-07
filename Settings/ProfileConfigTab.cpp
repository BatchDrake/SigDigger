//
//    ProfileCOnfigTab.cpp: Configuration dialog window
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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

#include <QMessageBox>
#include <QFileDialog>

#include <Suscan/Library.h>
#include <SuWidgetsHelpers.h>
#include <time.h>
#include "ProfileConfigTab.h"
#include "SigDiggerHelpers.h"
#include "DeviceTweaks.h"
#include "ui_ProfileConfigTab.h"

#define PROFILE_CONFIG_TAB_MIN_DEVICE_FREQ       0
#define PROFILE_CONFIG_TAB_MAX_DEVICE_FREQ       7.5e9

#define PROFILE_CONFIG_SAMPLE_RATE_MATCH_REL_TOL 1e-6

using namespace SigDigger;

Q_DECLARE_METATYPE(Suscan::Source::Config); // Unicorns
Q_DECLARE_METATYPE(Suscan::Source::Device); // More unicorns


// TODO:
// hasChanged and shouldRestart is based on the idea that the modification
// of certain fields can be used as a hint to whether we should restart
// the source or not. This can be done better: just add a Source::Config
// method that compares two source configs, and determines whether it
// makes sense to restart certain things or not.

void
ProfileConfigTab::configChanged(bool restart)
{
  this->modified     = true;
  this->needsRestart = this->needsRestart || restart;
  emit changed();
}

bool
ProfileConfigTab::shouldDisregardTweaks()
{
  QMessageBox::StandardButton reply;

  if (this->hasTweaks) {
    reply = QMessageBox::question(
          this,
          "Per-device tweaks",
          "This action will clear currently defined device tweaks. Are you sure?",
          QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
      return false;
  }

  this->hasTweaks = false;

  return true;
}

void
ProfileConfigTab::populateProfileCombo()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  this->ui->profileCombo->clear();

  for (auto i = sus->getFirstProfile(); i != sus->getLastProfile(); ++i)
    this->ui->profileCombo->addItem(
        QString::fromStdString(i->first),
        QVariant::fromValue(i->second));
}

void
ProfileConfigTab::populateDeviceCombo()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  int currentIndex = this->ui->deviceCombo->currentIndex();
  int newIndex = -1;
  int p = 0;
  QString prevName, name;

  if (currentIndex != -1)
    prevName = this->ui->deviceCombo->currentText();

  this->ui->deviceCombo->clear();

  for (auto i = sus->getFirstDevice(); i != sus->getLastDevice(); ++i) {
    if (i->isAvailable() && !i->isRemote()) {
      name = QString::fromStdString(i->getDesc());
      if (currentIndex != -1 && newIndex == -1 && name == prevName)
        newIndex = p;

      this->ui->deviceCombo->addItem(
          name,
          QVariant::fromValue<long>(i - sus->getFirstDevice()));
      ++p;
    }
  }

  if (newIndex == -1)
    newIndex = 0;

  this->ui->deviceCombo->setCurrentIndex(newIndex);
}

void
ProfileConfigTab::populateRemoteDeviceCombo()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  this->ui->remoteDeviceCombo->clear();

  for (
       auto i = sus->getFirstNetworkProfile();
       i != sus->getLastNetworkProfile();
       ++i)
    this->ui->remoteDeviceCombo->addItem(i->label().c_str());

  if (this->ui->remoteDeviceCombo->currentIndex() != -1)
    this->ui->remoteDeviceCombo->setCurrentIndex(0);
}


void
ProfileConfigTab::populateCombos()
{
  this->populateProfileCombo();
  this->populateDeviceCombo();
  this->populateRemoteDeviceCombo();

  this->onDeviceChanged(this->ui->deviceCombo->currentIndex());
}

void
ProfileConfigTab::refreshSampRateCtl()
{
  int index;

  index = m_rateHint == SAMPLE_RATE_CTL_HINT_LIST ? 0 : 1;

  if (this->ui->sampleRateCombo->count() == 0
      || this->ui->overrideCheck->isChecked())
    index = 1; // User has overriden this

  this->ui->sampRateStack->setCurrentIndex(index);
}

//
// This method simply informs the UI which is the preferred way to obtain
// a sample rate: whether from a combo box or manually from a spinbox. Sometimes
// that is simply not possible. In those cases, the function may decide
// which widget to display depending on the supported rates.
//
void
ProfileConfigTab::sampRateCtlHint(SampleRateCtlHint hint)
{
  // index 0: list (enable combo)
  // index 1: manual (enable spinbox)

  m_rateHint = hint;

  // Override only makes sense if we provide a combo box
  this->ui->overrideCheck->setEnabled(hint == SAMPLE_RATE_CTL_HINT_LIST);
  this->refreshSampRateCtl();
}

void
ProfileConfigTab::refreshUiState()
{
  int analyzerTypeIndex = this->ui->analyzerTypeCombo->currentIndex();
  bool netProfile = this->ui->useNetworkProfileRadio->isChecked();
  bool adjustStartTime = false;

  this->ui->analyzerParamsStackedWidget->setCurrentIndex(analyzerTypeIndex);

  if (!this->remoteSelected()) {
    /* Local analyzer */
    if (this->ui->sdrRadio->isChecked()) {
      this->ui->sdrFrame->setEnabled(true);
      this->ui->fileFrame->setEnabled(false);
      this->sampRateCtlHint(SAMPLE_RATE_CTL_HINT_LIST);
      this->ui->ppmSpinBox->setEnabled(true);
    } else {
      this->ui->sdrFrame->setEnabled(false);
      this->ui->fileFrame->setEnabled(true);
      this->ui->ppmSpinBox->setEnabled(false);
      this->sampRateCtlHint(SAMPLE_RATE_CTL_HINT_MANUAL);
      adjustStartTime = true;
    }
  } else {
    /* Remote analyzer */
    this->sampRateCtlHint(SAMPLE_RATE_CTL_HINT_MANUAL);

    if (this->ui->remoteDeviceCombo->count() == 0) {
      if (netProfile)
        netProfile = false;
      this->ui->useNetworkProfileRadio->setChecked(false);
      this->ui->useHostPortRadio->setChecked(true);
      this->ui->useNetworkProfileRadio->setEnabled(false);
    } else {
      this->ui->useNetworkProfileRadio->setEnabled(true);
    }

    this->ui->hostEdit->setEnabled(!netProfile);
    this->ui->portEdit->setEnabled(!netProfile);
    this->ui->remoteDeviceCombo->setEnabled(netProfile);
    this->ui->ppmSpinBox->setEnabled(true);
  }

  this->ui->sourceTimeEdit->setEnabled(adjustStartTime);
  this->ui->sourceTimeIsUTCCheck->setEnabled(adjustStartTime);
  this->setSelectedSampleRate(this->profile.getSampleRate());
  this->refreshTrueSampleRate();
}

void
ProfileConfigTab::refreshAntennas()
{
  SigDiggerHelpers::populateAntennaCombo(
        this->profile,
        this->ui->antennaCombo);
}

void
ProfileConfigTab::refreshSampRates()
{
  Suscan::Source::Device device = this->profile.getDevice();

  this->ui->sampleRateCombo->clear();

  for (
       auto p = device.getFirstSampRate();
       p != device.getLastSampRate();
       ++p) {
    this->ui->sampleRateCombo->addItem(
          getSampRateString(*p),
          QVariant::fromValue<double>(*p));
  }
}

void
ProfileConfigTab::refreshFrequencyLimits()
{
  SUFREQ lnbFreq = this->ui->lnbSpinBox->value();
  SUFREQ devMinFreq = PROFILE_CONFIG_TAB_MIN_DEVICE_FREQ;
  SUFREQ devMaxFreq = PROFILE_CONFIG_TAB_MAX_DEVICE_FREQ;

  if (this->profile.getType() == SUSCAN_SOURCE_TYPE_FILE) {
    devMinFreq = SIGDIGGER_MIN_RADIO_FREQ;
    devMaxFreq = SIGDIGGER_MAX_RADIO_FREQ;
  } else {
    const Suscan::Source::Device *dev = &(this->profile.getDevice());

    if (dev != nullptr) {
      devMinFreq = dev->getMinFreq();
      devMaxFreq = dev->getMaxFreq();
    }
  }
  // DEVFREQ = FREQ - LNB

  this->ui->frequencySpinBox->setMinimum(devMinFreq + lnbFreq);
  this->ui->frequencySpinBox->setMaximum(devMaxFreq + lnbFreq);
}

QString
ProfileConfigTab::getSampRateString(qreal trueRate)
{
  QString rateText;

  if (trueRate < 1e3)
    rateText = QString::number(trueRate) + " sps";
  else if (trueRate < 1e6)
    rateText = QString::number(trueRate * 1e-3) + " ksps";
  else if (trueRate < 1e9)
    rateText = QString::number(trueRate * 1e-6) + " Msps";

  return rateText;
}

void
ProfileConfigTab::refreshTrueSampleRate()
{
  float step = SU_POW(10., SU_FLOOR(SU_LOG(this->profile.getSampleRate())));
  QString rateText;
  qreal trueRate = static_cast<qreal>(this->getSelectedSampleRate())
      / this->getDecimation();
  if (step >= 10.f)
    step /= 10.f;

  this->ui->trueRateLabel->setText(getSampRateString(trueRate));
}

void
ProfileConfigTab::refreshAnalyzerTypeUi()
{
  if (this->profile.getInterface() == SUSCAN_SOURCE_LOCAL_INTERFACE) {
    this->ui->analyzerTypeCombo->setCurrentIndex(0);
  } else {
    this->ui->analyzerTypeCombo->setCurrentIndex(1);
  }

  this->ui->analyzerParamsStackedWidget->setCurrentIndex(
        this->ui->analyzerTypeCombo->currentIndex());
}

int
ProfileConfigTab::findRemoteProfileIndex()
{
  return this->ui->remoteDeviceCombo->findText(this->profile.label().c_str());
}

void
ProfileConfigTab::refreshProfileUi()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  bool adjustableSourceTime = false;

  for (auto i = 0; i < this->ui->profileCombo->count(); ++i)
    if (this->ui->profileCombo->itemText(i).toStdString() ==
        this->profile.label()) {
      this->ui->profileCombo->setCurrentIndex(i);
      break;
    }

  this->refreshSampRates();

  this->setDecimation(this->profile.getDecimation());

  switch (this->profile.getType()) {
    case SUSCAN_SOURCE_TYPE_SDR:
      this->ui->sdrRadio->setChecked(true);
      this->sampRateCtlHint(SAMPLE_RATE_CTL_HINT_LIST);
      break;

    case SUSCAN_SOURCE_TYPE_FILE:
      this->ui->fileRadio->setChecked(true);
      this->sampRateCtlHint(SAMPLE_RATE_CTL_HINT_MANUAL);
      break;
  }

  this->setSelectedSampleRate(this->profile.getSampleRate());

  this->ui->iqBalanceCheck->setChecked(this->profile.getIQBalance());
  this->ui->removeDCCheck->setChecked(this->profile.getDCRemove());
  this->ui->loopCheck->setChecked(this->profile.getLoop());

  this->ui->ppmSpinBox->setValue(
        static_cast<double>(this->profile.getPPM()));

  this->ui->bandwidthSpinBox->setValue(
        static_cast<double>(this->profile.getBandwidth()));

  switch (this->profile.getFormat()) {
    case SUSCAN_SOURCE_FORMAT_AUTO:
      this->ui->formatCombo->setCurrentIndex(0);
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_FLOAT32:
      this->ui->formatCombo->setCurrentIndex(1);
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_UNSIGNED8:
      this->ui->formatCombo->setCurrentIndex(2);
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_SIGNED8:
      this->ui->formatCombo->setCurrentIndex(3);
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_SIGNED16:
      this->ui->formatCombo->setCurrentIndex(4);
      break;

    case SUSCAN_SOURCE_FORMAT_WAV:
      this->ui->formatCombo->setCurrentIndex(5);
      break;
  }

  this->ui->pathEdit->setText(QString::fromStdString(this->profile.getPath()));

  this->refreshAnalyzerTypeUi();

  if (this->profile.getInterface() == SUSCAN_SOURCE_LOCAL_INTERFACE) {
    // Set local analyzer interface
    for (auto i = sus->getFirstDevice(); i != sus->getLastDevice(); ++i) {
      if (i->equals(this->profile.getDevice())) {
        int index = this->ui->deviceCombo->findData(
              QVariant::fromValue(
                static_cast<long>(i - sus->getFirstDevice())));
        if (index != -1) {
          this->ui->deviceCombo->setCurrentIndex(index);
          this->savedLocalDeviceIndex = index;
        }

        break;
      }
    }

    if (this->ui->deviceCombo->currentIndex() == -1)
      this->ui->deviceCombo->setCurrentIndex(0);

    if (this->profile.getType() == SUSCAN_SOURCE_TYPE_FILE)
      adjustableSourceTime = true;
  } else {
    bool hasMc;
    std::string host, port, user, pass, mc_if;
    int index;

    host  = this->profile.getParam("host");
    port  = this->profile.getParam("port");
    user  = this->profile.getParam("user");
    pass  = this->profile.getParam("password");
    hasMc = this->profile.hasParam("mc_if");
    mc_if = this->profile.getParam("mc_if");

    // Set remote analyzer interface
    this->ui->hostEdit->setText(host.c_str());

    try {
      this->ui->portEdit->setValue(std::stoi(port));
    } catch (std::invalid_argument &) {
      this->ui->portEdit->setValue(28001);
    }

    this->ui->deviceCombo->setCurrentIndex(-1);

    index = this->findRemoteProfileIndex();
    if (index != -1) {
      this->ui->useNetworkProfileRadio->setChecked(true);
      this->ui->useHostPortRadio->setChecked(false);
      this->ui->remoteDeviceCombo->setCurrentIndex(index);
    } else {
      this->ui->useHostPortRadio->setChecked(true);
      this->ui->useNetworkProfileRadio->setChecked(false);
    }

    this->ui->userEdit->setText(user.c_str());
    this->ui->passEdit->setText(pass.c_str());
    this->ui->mcCheck->setChecked(hasMc);
    this->ui->mcInterfaceEdit->setText(mc_if.c_str());
    this->ui->mcInterfaceEdit->setEnabled(hasMc);
  }

  if (adjustableSourceTime) {
    struct timeval startTime = this->profile.getStartTime();
    qint64 epochMsec = startTime.tv_sec * 1000 + startTime.tv_usec / 1000;
    QDateTime dateTime;
    dateTime.setTimeSpec(
          this->ui->sourceTimeIsUTCCheck->isChecked()
          ? Qt::TimeSpec::UTC
          : Qt::TimeSpec::LocalTime);

    dateTime.setMSecsSinceEpoch(epochMsec);

    this->ui->sourceTimeEdit->setTimeSpec(dateTime.timeSpec());
    this->ui->sourceTimeEdit->setDateTime(dateTime);
  } else {
    this->ui->sourceTimeEdit->setDateTime(
          QDateTime::fromSecsSinceEpoch(
            static_cast<unsigned int>(time(nullptr))));
  }

  this->ui->lnbSpinBox->setValue(this->profile.getLnbFreq());
  this->ui->frequencySpinBox->setValue(this->profile.getFreq());
  this->refreshFrequencyLimits();
  this->refreshUiState();
  this->refreshAntennas();
  this->refreshTrueSampleRate();
}

void
ProfileConfigTab::refreshUi()
{
  this->refreshing = true;

  this->refreshProfileUi();

  this->refreshing = false;
}

void
ProfileConfigTab::save()
{
  bool modified = this->modified;
  bool needsRestart = this->needsRestart;

  this->profile.setType(
        this->ui->sdrRadio->isChecked()
        ? SUSCAN_SOURCE_TYPE_SDR
        : SUSCAN_SOURCE_TYPE_FILE);

  if (!this->hasTweaks)
    this->onDeviceChanged(this->ui->deviceCombo->currentIndex());

  this->onFormatChanged(this->ui->formatCombo->currentIndex());
  this->onBandwidthChanged(this->ui->bandwidthSpinBox->value());
  this->onCheckButtonsToggled(false);
  this->onSpinsChanged();

  if (!this->hasTweaks)
    this->onAnalyzerTypeChanged(this->ui->analyzerTypeCombo->currentIndex());

  this->modified     = modified;
  this->needsRestart = needsRestart;
}

void
ProfileConfigTab::setUnchanged()
{
  this->modified     = false;
  this->needsRestart = false;
}

bool
ProfileConfigTab::hasChanged() const
{
  return this->modified;
}

bool
ProfileConfigTab::shouldRestart() const
{
  return this->needsRestart;
}

void
ProfileConfigTab::connectAll()
{
  connect(
        this->ui->deviceCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onDeviceChanged(int)));

  connect(
        this->ui->antennaCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onAntennaChanged(int)));

  connect(
        this->ui->loadProfileButton,
        SIGNAL(clicked()),
        this,
        SLOT(onLoadProfileClicked()));

  connect(
        this->ui->sdrRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleSourceType(bool)));

  connect(
        this->ui->fileRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onToggleSourceType(bool)));

  connect(
        this->ui->frequencySpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        this->ui->lnbSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        this->ui->sampleRateSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        this->ui->decimCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        this->ui->sampleRateCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        this->ui->ppmSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        this->ui->removeDCCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onCheckButtonsToggled(bool)));

  connect(
        this->ui->iqBalanceCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onCheckButtonsToggled(bool)));

  connect(
        this->ui->loopCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onCheckButtonsToggled(bool)));

  connect(
        this->ui->bandwidthSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onBandwidthChanged(double)));

  connect(
        this->ui->formatCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onFormatChanged(int)));

  connect(
        this->ui->browseButton,
        SIGNAL(clicked()),
        this,
        SLOT(onBrowseCaptureFile()));

  connect(
        this->ui->saveProfileButton,
        SIGNAL(clicked()),
        this,
        SLOT(onSaveProfile()));

  connect(
        this->ui->analyzerTypeCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onAnalyzerTypeChanged(int)));

  connect(
        this->ui->hostEdit,
        SIGNAL(textEdited(const QString &)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        this->ui->portEdit,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        this->ui->userEdit,
        SIGNAL(textEdited(const QString &)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        this->ui->passEdit,
        SIGNAL(textEdited(const QString &)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        this->ui->mcCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        this->ui->mcInterfaceEdit,
        SIGNAL(textChanged(const QString &)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        this->ui->useNetworkProfileRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onChangeConnectionType()));

  connect(
        this->ui->useHostPortRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onChangeConnectionType()));

  connect(
        this->ui->remoteDeviceCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onRemoteProfileSelected()));

  connect(
        this->ui->refreshButton,
        SIGNAL(clicked()),
        this,
        SLOT(onRefreshRemoteDevices()));

  connect(
        this->ui->ppmSpinBox,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        this->ui->sourceTimeEdit,
        SIGNAL(dateTimeChanged(QDateTime const &)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        this->ui->sourceTimeIsUTCCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onChangeSourceTimeUTC()));

  connect(
        this->ui->deviceTweaksButton,
        SIGNAL(clicked()),
        this,
        SLOT(onDeviceTweaksClicked()));

  connect(
        this->tweaks,
        SIGNAL(accepted()),
        this,
        SLOT(onDeviceTweaksAccepted()));

  connect(
        this->ui->overrideCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onOverrideSampleRate()));
}

void
ProfileConfigTab::setProfile(const Suscan::Source::Config &profile)
{
  this->profile = profile;
  this->hasTweaks = false;

  this->refreshUi();
  this->setUnchanged();
}

void
ProfileConfigTab::setFrequency(qint64 val)
{
  this->profile.setFreq(static_cast<SUFREQ>(val));
}

void
ProfileConfigTab::notifySingletonChanges()
{
  this->populateCombos();
  this->refreshUi();
}

bool
ProfileConfigTab::remoteSelected() const
{
  return this->ui->analyzerTypeCombo->currentIndex() == 1;
}

void
ProfileConfigTab::setGain(std::string const &name, float value)
{
  this->profile.setGain(name, value);
}

float
ProfileConfigTab::getGain(std::string const &name) const
{
  return this->profile.getGain(name);
}

Suscan::Source::Config
ProfileConfigTab::getProfile() const
{
  return this->profile;
}

unsigned
ProfileConfigTab::getDecimation() const
{
  if (this->ui->decimCombo->currentIndex() < 0)
    return 0;

  return 1u << this->ui->decimCombo->currentIndex();
}

void
ProfileConfigTab::setDecimation(unsigned decim)
{
  int i = 0;

  while ((1u << i) < decim && i != this->ui->decimCombo->count() - 1)
    ++i;

  this->ui->decimCombo->setCurrentIndex(i);
}

void
ProfileConfigTab::updateRemoteParams()
{
  this->profile.clearParams();

  this->profile.setParam("host", this->ui->hostEdit->text().toStdString());
  this->profile.setParam("port", std::to_string(this->ui->portEdit->value()));
  this->profile.setParam("user", this->ui->userEdit->text().toStdString());
  this->profile.setParam("password", this->ui->passEdit->text().toStdString());

  if (this->ui->mcCheck->isChecked())
    this->profile.setParam(
        "mc_if",
        this->ui->mcInterfaceEdit->text().toStdString());

  this->profile.setParam("label", "User-defined remote profile");
}

ProfileConfigTab::ProfileConfigTab(QWidget *parent) : ConfigTab(parent, "Source"),
  profile(SUSCAN_SOURCE_TYPE_FILE, SUSCAN_SOURCE_FORMAT_AUTO)
{
  this->ui = new Ui::ProfileConfigTab;
  this->ui->setupUi(this);

  // Set local analyzer as default
  this->ui->analyzerTypeCombo->setCurrentIndex(0);

  this->tweaks = new DeviceTweaks(this);
  this->tweaks->setModal(true);

  // Setup remote device
  this->remoteDevice = Suscan::Source::Device(
          "Remote device",
          "localhost",
          28001,
          "anonymous",
          "");

  // Setup sample rate size
  this->ui->trueRateLabel->setFixedWidth(
        SuWidgetsHelpers::getWidgetTextWidth(
          this->ui->trueRateLabel,
          "XXX.XXX Xsps"));

  // Set limits
  this->ui->lnbSpinBox->setMaximum(300e9);
  this->ui->lnbSpinBox->setMinimum(-300e9);

  // Populate locations
  this->populateCombos();
  this->ui->sampleRateSpinBox->setUnits("sps");
  this->connectAll();
  this->refreshUi();

}

QString
ProfileConfigTab::getBaseName(const QString &path)
{
  int ndx;

  if ((ndx = path.lastIndexOf('/')) != -1)
    return path.right(path.size() - ndx - 1);

  return path;
}


ProfileConfigTab::~ProfileConfigTab()
{
  delete this->ui;
}

//////////////// Slots //////////////////
void
ProfileConfigTab::onLoadProfileClicked()
{
  QVariant data = this->ui->profileCombo->itemData(this->ui->profileCombo->currentIndex());

  if (this->shouldDisregardTweaks()) {
    this->configChanged(true);
    this->profile = data.value<Suscan::Source::Config>();
  }

  this->refreshUi();
}

void
ProfileConfigTab::onToggleSourceType(bool)
{
  if (!this->refreshing) {
    this->configChanged(true);
    if (this->ui->sdrRadio->isChecked()) {
      this->profile.setType(SUSCAN_SOURCE_TYPE_SDR);
    } else {
      this->profile.setType(SUSCAN_SOURCE_TYPE_FILE);
      this->guessParamsFromFileName();
    }

    this->refreshUiState();
    this->refreshFrequencyLimits();
  }
}

void
ProfileConfigTab::onDeviceChanged(int index)
{
  // Remember: only set device if the analyzer type is local
  if (!this->refreshing
      && index != -1
      && !this->remoteSelected()) {
    Suscan::Singleton *sus = Suscan::Singleton::get_instance();
    const Suscan::Source::Device *device;

    if (!this->shouldDisregardTweaks()) {
      this->refreshUi();
      return;
    }

    SU_ATTEMPT(
          device = sus->getDeviceAt(
            static_cast<unsigned int>(
            this->ui->deviceCombo->itemData(index).value<long>())));

    this->configChanged(true);
    this->profile.setDevice(*device);
    this->hasTweaks = false;

    auto begin = device->getFirstAntenna();
    auto end   = device->getLastAntenna();

    // We check whether we can keep the current antenna configuration. If we
    // cannot, just set the first antenna in the list.
    if (device->findAntenna(this->profile.getAntenna()) == end
        && begin != end)
      this->profile.setAntenna(*begin);

    this->refreshUi();

    unsigned sampRate = this->getSelectedSampleRate();
    unsigned decimation = this->getDecimation();
    qreal maxBandwidth = static_cast<qreal>(sampRate)
        / static_cast<qreal>(decimation);

    if (this->ui->bandwidthSpinBox->value() > maxBandwidth)
      this->ui->bandwidthSpinBox->setValue(maxBandwidth);
  }
}

void
ProfileConfigTab::onFormatChanged(int index)
{
  if (!this->refreshing) {
    this->configChanged(true);
    switch (index) {
      case 0:
        this->profile.setFormat(SUSCAN_SOURCE_FORMAT_AUTO);
        break;

      case 1:
        this->profile.setFormat(SUSCAN_SOURCE_FORMAT_RAW_FLOAT32);
        break;

      case 2:
        this->profile.setFormat(SUSCAN_SOURCE_FORMAT_RAW_UNSIGNED8);
        break;

      case 3:
        this->profile.setFormat(SUSCAN_SOURCE_FORMAT_RAW_SIGNED8);
        break;

      case 4:
        this->profile.setFormat(SUSCAN_SOURCE_FORMAT_RAW_SIGNED16);
        break;

      case 5:
        this->profile.setFormat(SUSCAN_SOURCE_FORMAT_WAV);
        break;
    }
  }
}

void
ProfileConfigTab::onAntennaChanged(int)
{
  if (!this->refreshing) {
    this->configChanged();
    this->profile.setAntenna(
          this->ui->antennaCombo->currentText().toStdString());
  }
}

void
ProfileConfigTab::onAnalyzerTypeChanged(int index)
{
  if (!this->refreshing) {
    if (!this->shouldDisregardTweaks()) {
      this->refreshUi();
      return;
    }

    switch (index) {
      case 0:
        this->profile.setInterface(SUSCAN_SOURCE_LOCAL_INTERFACE);
        this->onDeviceChanged(this->savedLocalDeviceIndex);
        break;

      case 1:
        this->savedLocalDeviceIndex = this->ui->deviceCombo->currentIndex();
        this->profile.setInterface(SUSCAN_SOURCE_REMOTE_INTERFACE);
        this->onChangeConnectionType();
        this->onRemoteParamsChanged();
        break;
    }

    this->refreshUiState();
  }
}

void
ProfileConfigTab::onRemoteParamsChanged()
{
  if (this->remoteSelected()) {
    this->ui->mcInterfaceEdit->setEnabled(this->ui->mcCheck->isChecked());
    this->configChanged(true);
    this->profile.setDevice(this->remoteDevice);
    this->updateRemoteParams();
  }
}

void
ProfileConfigTab::onCheckButtonsToggled(bool)
{
  if (!this->refreshing) {
    if (this->profile.getDCRemove() != this->ui->removeDCCheck->isChecked()) {
      this->profile.setDCRemove(this->ui->removeDCCheck->isChecked());
      this->configChanged();
    }

    if (this->profile.getIQBalance() != this->ui->iqBalanceCheck->isChecked()) {
      this->profile.setIQBalance(this->ui->iqBalanceCheck->isChecked());
      this->configChanged();
    }

    if (this->profile.getLoop() != this->ui->loopCheck->isChecked()) {
      this->profile.setLoop(this->ui->loopCheck->isChecked());
      this->configChanged(true);
    }
  }
}

unsigned int
ProfileConfigTab::getSelectedSampleRate() const
{
  unsigned int sampRate = 0;

  if (this->ui->sampRateStack->currentIndex() == 0) {
    // Index 0: Sample Rate Combo
    if (this->ui->sampleRateCombo->currentIndex() != -1) {
      qreal selectedValue =
          this->ui->sampleRateCombo->currentData().value<qreal>();
      sampRate = static_cast<unsigned>(selectedValue);
    }
  } else {
    // Index 1: Sample Rate Spin
    sampRate = static_cast<unsigned>(
          this->ui->sampleRateSpinBox->value());
  }

  return sampRate;
}

void
ProfileConfigTab::setSelectedSampleRate(unsigned int rate)
{
  // Set sample rate in both places
  qreal dist = std::numeric_limits<qreal>::infinity();
  qreal bestRate = std::numeric_limits<qreal>::infinity();

  int bestIndex = -1;
  for (auto i = 0; i < this->ui->sampleRateCombo->count(); ++i) {
    qreal value = this->ui->sampleRateCombo->itemData(i).value<qreal>();
    if (fabs(value - rate) < dist) {
      bestIndex = i;
      bestRate = rate;
      dist = fabs(value - rate);
    }
  }

  // bestIndex different from -1 means that there is at least one optioon

  if (bestIndex != -1) {
    bool optionIsGood =
        dist / bestRate < PROFILE_CONFIG_SAMPLE_RATE_MATCH_REL_TOL;

    if (optionIsGood)
      this->ui->sampleRateCombo->setCurrentIndex(bestIndex);
    else
      this->ui->overrideCheck->setChecked(true);
  }


  this->ui->sampleRateSpinBox->setValue(rate);
}

void
ProfileConfigTab::onSpinsChanged()
{
  if (!this->refreshing) {
    SUFREQ freq;
    SUFREQ lnbFreq;
    SUFLOAT ppm;
    SUFLOAT maxBandwidth;
    time_t timeStamp;
    time_t timeStampUsec;
    bool adjustBandwidth = false;

    unsigned int sampRate;
    unsigned int decimation;

    lnbFreq = this->ui->lnbSpinBox->value();
    this->refreshFrequencyLimits();
    freq = this->ui->frequencySpinBox->value();
    sampRate = this->getSelectedSampleRate();
    decimation = this->getDecimation();
    ppm = static_cast<SUFLOAT>(this->ui->ppmSpinBox->value());
    maxBandwidth = static_cast<SUFLOAT>(sampRate)
        / static_cast<SUFLOAT>(decimation) ;
    timeStamp = this->ui->sourceTimeEdit->dateTime().toSecsSinceEpoch();
    timeStampUsec = 1000 * (
          this->ui->sourceTimeEdit->dateTime().toMSecsSinceEpoch() % 1000);

    if (!sufeq(this->profile.getFreq(), freq, .5f)) {
      this->profile.setFreq(freq);
      this->configChanged();
    }

    if (!sufeq(this->profile.getLnbFreq(), lnbFreq, .5f)) {
      this->profile.setLnbFreq(lnbFreq);
      this->configChanged();
    }

    if (this->profile.getSampleRate() != sampRate) {
      this->profile.setSampleRate(sampRate);
      this->configChanged(true);
      adjustBandwidth = true;
    }

    if (this->profile.getDecimation() != decimation) {
      this->profile.setDecimation(decimation);
      this->configChanged(true);
      adjustBandwidth = true;
    }

    if (!sufeq(this->profile.getPPM(), ppm, .5f)) {
      this->profile.setPPM(ppm);
      this->configChanged();
    }

    if (this->ui->sourceTimeEdit->isEnabled()
        && (this->profile.getStartTime().tv_sec != timeStamp
        || this->profile.getStartTime().tv_usec != timeStampUsec)) {
      struct timeval tv;
      tv.tv_sec  = timeStamp;
      tv.tv_usec = timeStampUsec;
      this->profile.setStartTime(tv);
      this->configChanged(true);
    }

    if (adjustBandwidth && this->profile.getBandwidth() > maxBandwidth)
      this->ui->bandwidthSpinBox->setValue(static_cast<qreal>(maxBandwidth));

    this->refreshTrueSampleRate();
  }
}

void
ProfileConfigTab::onBandwidthChanged(double)
{
  if (!this->refreshing) {
    this->profile.setBandwidth(
        static_cast<SUFLOAT>(
          this->ui->bandwidthSpinBox->value()));
    this->configChanged();
  }
}

void
ProfileConfigTab::guessParamsFromFileName()
{
  SigDiggerHelpers *hlp = SigDiggerHelpers::instance();
  CaptureFileParams params;

  if (hlp->guessCaptureFileParams(params, this->profile.getPath().c_str())) {
    if (params.haveTm)
      this->profile.setStartTime(params.tv);

    if (params.haveFs)
      this->profile.setSampleRate(params.fs);

    if (params.haveFc)
      this->profile.setFreq(params.fc);

    if (params.haveFs || params.haveFc || params.haveTm)
      this->refreshUi();
  }
}

void
ProfileConfigTab::onBrowseCaptureFile()
{
  QString title;
  QFileInfo fi(this->ui->pathEdit->text());
  QStringList formats;
  QString selected = "All files (*)";

  switch (this->profile.getFormat()) {
    case SUSCAN_SOURCE_FORMAT_AUTO:
      title = "Open capture file";
      formats
          << "Raw complex 32-bit float (*.raw *.cf32)"
          << "Raw complex 8-bit unsigned (*.u8 *.cu8)"
          << "Raw complex 8-bit signed (*.s8 *.cs8)"
          << "Raw complex 16-bit signed (*.s16 *.cs16)"
          << "WAV files (*.wav)"
          << "All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_FLOAT32:
      title = "Open I/Q file";
      formats
          << "Raw complex 32-bit float (*.raw *.cf32)"
          << "All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_UNSIGNED8:
      title = "Open I/Q file";
      formats
          << "Raw complex 8-bit unsigned (*.u8 *.cu8)"
          << "All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_SIGNED8:
      title = "Open I/Q file";
      formats
          << "Raw complex 8-bit signed (*.s8 *.cs8)"
          << "All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_SIGNED16:
      title = "Open I/Q file";
      formats
          << "Raw complex 16-bit signed (*.s16 *.cs16)"
          << "All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_WAV:
      title = "Open WAV file";
      formats
          << "WAV files (*.wav)"
          << "All files (*)";
      break;
  }

  for (auto p : formats)
    if (p.contains("*." + fi.suffix()))
      selected = p;

  QString path = QFileDialog::getOpenFileName(
         this,
         title,
         fi.absolutePath(),
         formats.join(";;"),
         &selected);

  if (!path.isEmpty()) {
    this->ui->pathEdit->setText(path);
    this->configChanged(true);
    this->profile.setPath(path.toStdString());
    this->guessParamsFromFileName();
  }
}

void
ProfileConfigTab::onSaveProfile()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  std::string name = "My " + this->profile.label();
  std::string candidate = name;
  unsigned int i = 1;

  while (sus->getProfile(candidate) != nullptr)
    candidate = name + " (" + std::to_string(i++) + ")";

  this->saveProfileDialog.setProfileName(QString::fromStdString(candidate));

  if (this->saveProfileDialog.run()) {
    candidate = this->saveProfileDialog.getProfileName().toStdString();

    if (sus->getProfile(candidate) != nullptr) {
      QMessageBox::warning(
            this,
            "Profile already exists",
            "There is already a profile named " +
            this->saveProfileDialog.getProfileName() +
            " please choose a different one.",
            QMessageBox::Ok);
      return;
    }

    this->profile.setLabel(candidate);
    sus->saveProfile(this->profile);
    this->populateProfileCombo();
  }
}

void
ProfileConfigTab::onChangeConnectionType()
{
  if (this->ui->useNetworkProfileRadio->isChecked()) {
    this->onRemoteProfileSelected();
    this->ui->useHostPortRadio->setChecked(false);
  }

  if (this->ui->useHostPortRadio->isChecked()) {
    this->onRemoteParamsChanged();
    this->ui->useNetworkProfileRadio->setChecked(false);
  }

  this->configChanged(true);
  this->refreshUiState();
}

void
ProfileConfigTab::onRefreshRemoteDevices()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  int countBefore = this->ui->remoteDeviceCombo->count();
  int countAfter;

  sus->refreshNetworkProfiles();
  this->populateRemoteDeviceCombo();

  countAfter = this->ui->remoteDeviceCombo->count();

  if (countAfter > countBefore) {
    this->ui->useNetworkProfileRadio->setChecked(true);
    this->onChangeConnectionType();
  } else {
    this->refreshUiState();
  }

  this->onRemoteParamsChanged();
}

void
ProfileConfigTab::onRemoteProfileSelected()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  if (this->ui->useNetworkProfileRadio->isChecked()) {
    QHash<QString, Suscan::Source::Config>::const_iterator it;
    std::string user, pass, mc, mc_if;
    bool hasMc;

    // Save multicast config
    hasMc = this->ui->mcCheck->isChecked();
    mc_if = this->ui->mcInterfaceEdit->text().toStdString();

    it = sus->getNetworkProfileFrom(this->ui->remoteDeviceCombo->currentText());

    if (it != sus->getLastNetworkProfile()) {
      user = it->getParam("user");
      pass = it->getParam("password");

      if (user.length() == 0)
        user = this->ui->userEdit->text().toStdString();
      if (user.length() == 0)
        user = "anonymous";

      if (pass.length() == 0)
        pass = this->ui->passEdit->text().toStdString();

      this->configChanged(true);
      this->setProfile(*it);

      // Provide a better hint for username if the server announced none
      this->ui->userEdit->setText(user.c_str());
      this->ui->passEdit->setText(pass.c_str());

      // Restore mc config
      this->ui->mcCheck->setChecked(hasMc);
      this->ui->mcInterfaceEdit->setText(mc_if.c_str());
      this->ui->mcInterfaceEdit->setEnabled(hasMc);

      this->onRemoteParamsChanged();
    }
  }
}

void
ProfileConfigTab::onChangeSourceTimeUTC()
{
  QDateTime dateTime = this->ui->sourceTimeEdit->dateTime();
  qint64 epochMsec   = dateTime.toMSecsSinceEpoch();

  dateTime.setTimeSpec(
        this->ui->sourceTimeIsUTCCheck->isChecked()
        ? Qt::TimeSpec::UTC
        : Qt::TimeSpec::LocalTime);

  dateTime.setMSecsSinceEpoch(epochMsec);

  this->ui->sourceTimeEdit->setTimeSpec(dateTime.timeSpec());
  this->ui->sourceTimeEdit->setDateTime(dateTime);
}

void
ProfileConfigTab::onDeviceTweaksClicked()
{
  this->tweaks->setProfile(&this->profile);
  this->tweaks->exec();
}

void
ProfileConfigTab::onDeviceTweaksAccepted()
{
  if (this->tweaks->hasChanged()) {
    this->tweaks->commitConfig();
    this->configChanged(true);
    this->hasTweaks = true;
  }
}

void
ProfileConfigTab::onOverrideSampleRate()
{
  if (!this->ui->overrideCheck->isChecked()) {
    this->refreshSampRateCtl();
    this->onSpinsChanged();
  }

  this->refreshUiState();
}
