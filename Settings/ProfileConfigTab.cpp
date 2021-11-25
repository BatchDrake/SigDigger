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

#include <QFileDialog>
#include <QMessageBox>

#include <Suscan/Library.h>
#include <SuWidgetsHelpers.h>
#include <time.h>
#include "ProfileConfigTab.h"
#include "ui_ProfileConfigTab.h"

#define PROFILE_CONFIG_TAB_MIN_DEVICE_FREQ 0
#define PROFILE_CONFIG_TAB_MAX_DEVICE_FREQ 7.5e9

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

void
ProfileConfigTab::populateCombos(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  this->ui->profileCombo->clear();
  this->ui->deviceCombo->clear();
  this->ui->remoteDeviceCombo->clear();

  for (auto i = sus->getFirstProfile(); i != sus->getLastProfile(); ++i)
      this->ui->profileCombo->addItem(
            QString::fromStdString(i->first),
            QVariant::fromValue(i->second));

  // Populate local devices only
  for (auto i = sus->getFirstDevice(); i != sus->getLastDevice(); ++i)
    if (i->isAvailable() && !i->isRemote())
      this->ui->deviceCombo->addItem(
          QString::fromStdString(i->getDesc()),
          QVariant::fromValue<long>(i - sus->getFirstDevice()));

  if (this->ui->deviceCombo->currentIndex() == -1)
    this->ui->deviceCombo->setCurrentIndex(0);

  // Network devices are traversed here.
  for (
       auto i = sus->getFirstNetworkProfile();
       i != sus->getLastNetworkProfile();
       ++i)
    this->ui->remoteDeviceCombo->addItem(i->label().c_str());

  if (this->ui->remoteDeviceCombo->currentIndex() != -1)
    this->ui->remoteDeviceCombo->setCurrentIndex(0);

  this->onDeviceChanged(this->ui->deviceCombo->currentIndex());
}

void
ProfileConfigTab::refreshUiState(void)
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
      this->ui->sampRateStack->setCurrentIndex(0);
      this->ui->ppmSpinBox->setEnabled(true);
    } else {
      this->ui->sdrFrame->setEnabled(false);
      this->ui->fileFrame->setEnabled(true);
      this->ui->ppmSpinBox->setEnabled(false);
      this->ui->sampRateStack->setCurrentIndex(1);
      adjustStartTime = true;
    }
  } else {
    /* Remote analyzer */
    this->ui->sampRateStack->setCurrentIndex(1);

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
ProfileConfigTab::refreshAntennas(void)
{
  populateAntennaCombo(this->profile, this->ui->antennaCombo);
}

void
ProfileConfigTab::refreshSampRates(void)
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
ProfileConfigTab::refreshFrequencyLimits(void)
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
ProfileConfigTab::refreshTrueSampleRate(void)
{
  float step = SU_POW(10., SU_FLOOR(SU_LOG(this->profile.getSampleRate())));
  QString rateText;
  qreal trueRate = static_cast<qreal>(this->getSelectedSampleRate())
      / this->ui->decimationSpin->value();
  if (step >= 10.f)
    step /= 10.f;

  this->ui->trueRateLabel->setText(getSampRateString(trueRate));
}

void
ProfileConfigTab::refreshAnalyzerTypeUi(void)
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
ProfileConfigTab::findRemoteProfileIndex(void)
{
  return this->ui->remoteDeviceCombo->findText(this->profile.label().c_str());
}

void
ProfileConfigTab::refreshProfileUi(void)
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

  this->ui->decimationSpin->setValue(
        static_cast<int>(this->profile.getDecimation()));

  switch (this->profile.getType()) {
    case SUSCAN_SOURCE_TYPE_SDR:
      this->ui->sdrRadio->setChecked(true);
      this->ui->sampRateStack->setCurrentIndex(0);
      break;

    case SUSCAN_SOURCE_TYPE_FILE:
      this->ui->fileRadio->setChecked(true);
      this->ui->sampRateStack->setCurrentIndex(1);
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

    case SUSCAN_SOURCE_FORMAT_WAV:
      this->ui->formatCombo->setCurrentIndex(3);
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
    std::string value;
    int index;
    // Set remote analyzer interface
    value = this->profile.getParam("host");
    this->ui->hostEdit->setText(value.c_str());

    try {
      value = this->profile.getParam("port").c_str();
      this->ui->portEdit->setValue(std::stoi(value));
    } catch (std::invalid_argument &) {
      this->ui->portEdit->setValue(28001);
    }

    value = this->profile.getParam("user").c_str();
    this->ui->userEdit->setText(value.c_str());


    value = this->profile.getParam("password").c_str();
    this->ui->passEdit->setText(value.c_str());

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
          QDateTime::fromTime_t(
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
ProfileConfigTab::refreshUi(void)
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

  this->onDeviceChanged(this->ui->deviceCombo->currentIndex());
  this->onFormatChanged(this->ui->formatCombo->currentIndex());
  this->onBandwidthChanged(this->ui->bandwidthSpinBox->value());
  this->onCheckButtonsToggled(false);
  this->onSpinsChanged();
  this->onAnalyzerTypeChanged(this->ui->analyzerTypeCombo->currentIndex());

  this->modified     = modified;
  this->needsRestart = needsRestart;
}

void
ProfileConfigTab::setUnchanged(void)
{
  this->modified     = false;
  this->needsRestart = false;
}

bool
ProfileConfigTab::hasChanged(void) const
{
  return this->modified;
}

bool
ProfileConfigTab::shouldRestart(void) const
{
  return this->needsRestart;
}

void
ProfileConfigTab::connectAll(void)
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
        SLOT(onLoadProfileClicked(void)));

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
        SLOT(onSpinsChanged(void)));

  connect(
        this->ui->lnbSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged(void)));

  connect(
        this->ui->sampleRateSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged(void)));

  connect(
        this->ui->decimationSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onSpinsChanged(void)));

  connect(
        this->ui->sampleRateCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onSpinsChanged(void)));

  connect(
        this->ui->ppmSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged(void)));

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
        SIGNAL(clicked(void)),
        this,
        SLOT(onBrowseCaptureFile(void)));

  connect(
        this->ui->saveProfileButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onSaveProfile(void)));

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
        this->ui->useNetworkProfileRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onChangeConnectionType(void)));

  connect(
        this->ui->useHostPortRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onChangeConnectionType(void)));

  connect(
        this->ui->remoteDeviceCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onRemoteProfileSelected(void)));

  connect(
        this->ui->refreshButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onRefreshRemoteDevices(void)));

  connect(
        this->ui->ppmSpinBox,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onSpinsChanged(void)));

  connect(
        this->ui->sourceTimeEdit,
        SIGNAL(dateTimeChanged(QDateTime const &)),
        this,
        SLOT(onSpinsChanged(void)));

  connect(
        this->ui->sourceTimeIsUTCCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onChangeSourceTimeUTC(void)));

  // this->ui->sourceTimeEdit->setDateTime

}

void
ProfileConfigTab::setProfile(const Suscan::Source::Config &profile)
{
  this->profile = profile;
  this->refreshUi();
  this->setUnchanged();
}

void
ProfileConfigTab::setFrequency(qint64 val)
{
  this->profile.setFreq(static_cast<SUFREQ>(val));
}

void
ProfileConfigTab::notifySingletonChanges(void)
{
  this->populateCombos();
  this->refreshUi();
}

bool
ProfileConfigTab::remoteSelected(void) const
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
ProfileConfigTab::getProfile(void) const
{
  return this->profile;
}

void
ProfileConfigTab::updateRemoteParams(void)
{
  this->profile.setParam("host", this->ui->hostEdit->text().toStdString());
  this->profile.setParam("port", std::to_string(this->ui->portEdit->value()));
  this->profile.setParam("user", this->ui->userEdit->text().toStdString());
  this->profile.setParam("password", this->ui->passEdit->text().toStdString());
  this->profile.setParam("label", "User-defined remote profile");
}

ProfileConfigTab::ProfileConfigTab(QWidget *parent) : ConfigTab(parent, "Source"),
  profile(SUSCAN_SOURCE_TYPE_FILE, SUSCAN_SOURCE_FORMAT_AUTO)
{
  this->ui = new Ui::ProfileConfigTab;
  this->ui->setupUi(this);

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
ProfileConfigTab::onLoadProfileClicked(void)
{
  QVariant data = this->ui->profileCombo->itemData(this->ui->profileCombo->currentIndex());

  this->configChanged(true);
  this->profile = data.value<Suscan::Source::Config>();

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

    SU_ATTEMPT(
          device = sus->getDeviceAt(
            static_cast<unsigned int>(
            this->ui->deviceCombo->itemData(index).value<long>())));

    this->configChanged(true);
    this->profile.setDevice(*device);
    auto begin = device->getFirstAntenna();
    auto end   = device->getLastAntenna();

    // We check whether we can keep the current antenna configuration. If we
    // cannot, just set the first antenna in the list.
    if (device->findAntenna(this->profile.getAntenna()) == end
        && begin != end)
      this->profile.setAntenna(*begin);

    this->refreshUi();

    unsigned sampRate = this->getSelectedSampleRate();
    unsigned decimation = static_cast<unsigned>(this->ui->decimationSpin->value());
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
ProfileConfigTab::onRemoteParamsChanged(void)
{
  if (this->remoteSelected()) {
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
ProfileConfigTab::getSelectedSampleRate(void) const
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
  int bestIndex = -1;
  for (auto i = 0; i < this->ui->sampleRateCombo->count(); ++i) {
    qreal value = this->ui->sampleRateCombo->itemData(i).value<qreal>();
    if (fabs(value - rate) < dist) {
      bestIndex = i;
      dist = fabs(value - rate);
    }
  }

  if (bestIndex != -1)
    this->ui->sampleRateCombo->setCurrentIndex(bestIndex);

  this->ui->sampleRateSpinBox->setValue(rate);
}

void
ProfileConfigTab::onSpinsChanged(void)
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
    decimation = static_cast<unsigned>(this->ui->decimationSpin->value());
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

    if (this->profile.getStartTime().tv_sec != timeStamp
        || this->profile.getStartTime().tv_usec != timeStampUsec) {
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
ProfileConfigTab::guessParamsFromFileName(void)
{
  QFileInfo fi(QString::fromStdString(this->profile.getPath()));
  std::string baseName = fi.baseName().toStdString();
  SUFREQ fc;
  unsigned int fs;
  unsigned int date, time;
  bool haveFc   = false;
  bool haveFs   = false;
  bool haveDate = false;
  bool haveTime = false;
  bool isUTC    = false;
  bool haveTm   = false;
  struct tm tm;
  struct timeval tv = {0, 0};

  memset(&tm, 0, sizeof(struct tm));

  if (sscanf(
        baseName.c_str(),
        "sigdigger_%08d_%06dZ_%d_%lg_float32_iq",
        &date,
        &time,
        &fs,
        &fc) == 4) {
    haveFc   = true;
    haveFs   = true;
    haveDate = true;
    haveTime = true;
    isUTC    = true;
  } else if (sscanf(
        baseName.c_str(),
        "sigdigger_%d_%lg_float32_iq",
        &fs,
        &fc) == 2) {
    haveFc = true;
    haveFs = true;
  } else if (sscanf(
        baseName.c_str(),
        "gqrx_%08d_%06d_%lg_%d_fc",
        &date,
        &time,
        &fc,
        &fs) == 4) {
    haveFc   = true;
    haveFs   = true;
    haveDate = true;
    haveTime = true;
  } else if (sscanf(
        baseName.c_str(),
        "SDRSharp_%08d_%06dZ_%lg_IQ",
        &date,
        &time,
        &fc) == 3) {
    haveFc   = true;
    haveDate = true;
    haveTime = true;
  } else if (sscanf(
        baseName.c_str(),
        "HDSDR_%08d_%06dZ_%lgkHz",
        &date,
        &time,
        &fc) == 3) {
    fc      *= 1e3;
    haveFc   = true;
    haveDate = true;
    haveTime = true;
    isUTC    = true;
  } else if (sscanf(
        baseName.c_str(),
        "baseband_%lgHz_%02d-%02d-%02d_%02d-%02d-%04d",
        &fc,
        &tm.tm_hour,
        &tm.tm_min,
        &tm.tm_sec,
        &tm.tm_mday,
        &tm.tm_mon,
        &tm.tm_year) == 7) {
    tm.tm_year -= 1900;
    tm.tm_mon  -= 1;

    haveFc   = true;
    haveTm   = true;
    isUTC    = true;
  }

  if (haveDate || haveTime) {
    haveTm = true;
    if (haveDate) {
      tm.tm_year = date / 10000 - 1900;
      tm.tm_mon  = ((date / 100) % 100) - 1;
      tm.tm_mday = date % 100;
    }

    if (haveTime) {
      tm.tm_hour = time / 10000;
      tm.tm_min  = (time / 100) % 100;
      tm.tm_sec  = time % 100;
    }
  }

  if (haveTm) {
    if (isUTC) {
      char *tz = getenv("TZ");
      std::string oldTz;

      if (tz != nullptr)
        oldTz = tz;

      tm.tm_isdst = 0;
      setenv("TZ", "", 1);
      tzset();
      tv.tv_sec = mktime(&tm);
      if (tz != nullptr)
        setenv("TZ", oldTz.c_str(), 1);
      else
        unsetenv("TZ");
      tzset();
    } else {
      tm.tm_isdst = -1;
      tv.tv_sec = mktime(&tm);
    }

    this->profile.setStartTime(tv);
  }

  if (haveFs)
    this->profile.setSampleRate(fs);

  if (haveFc)
    this->profile.setFreq(fc);

  if (haveFs || haveFc || haveTm)
    this->refreshUi();
}

void
ProfileConfigTab::onBrowseCaptureFile(void)
{
  QString format;
  QString title;
  QFileInfo fi(this->ui->pathEdit->text());

  switch (this->profile.getFormat()) {
    case SUSCAN_SOURCE_FORMAT_AUTO:
      title = "Open capture file";
      format = "I/Q files (*.raw);;WAV files (*.wav);;All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_FLOAT32:
      title = "Open I/Q file";
      format = "I/Q files (*.raw);;All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_RAW_UNSIGNED8:
      title = "Open I/Q file";
      format = "I/Q files (*.raw);;All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_WAV:
      title = "Open WAV file";
      format = "WAV files (*.wav);;All files (*)";
      break;
  }

  QString path = QFileDialog::getOpenFileName(
         this,
         title,
         fi.absolutePath(),
         format);

  if (!path.isEmpty()) {
    this->ui->pathEdit->setText(path);
    this->configChanged(true);
    this->profile.setPath(path.toStdString());
    this->guessParamsFromFileName();
  }
}

void
ProfileConfigTab::onSaveProfile(void)
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
    this->populateCombos();
  }
}

void
ProfileConfigTab::onChangeConnectionType(void)
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
ProfileConfigTab::onRefreshRemoteDevices(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  int countBefore = this->ui->remoteDeviceCombo->count();
  int countAfter;

  sus->refreshNetworkProfiles();
  this->populateCombos();

  countAfter = this->ui->remoteDeviceCombo->count();

  if (countAfter > countBefore) {
    this->ui->useNetworkProfileRadio->setChecked(true);
    this->onChangeConnectionType();
  } else {
    this->refreshUiState();
  }
}

void
ProfileConfigTab::onRemoteProfileSelected(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  if (this->ui->useNetworkProfileRadio->isChecked()) {
    QHash<QString, Suscan::Source::Config>::const_iterator it;

    it = sus->getNetworkProfileFrom(this->ui->remoteDeviceCombo->currentText());

    if (it != sus->getLastNetworkProfile()) {
      this->configChanged(true);
      this->setProfile(*it);

      // Provide a better hint for username if the server announced none
      if (this->profile.getParam("user").length() == 0)
        this->ui->userEdit->setText("anonymous");
      this->updateRemoteParams();
    }
  }
}

void
ProfileConfigTab::onChangeSourceTimeUTC(void)
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
