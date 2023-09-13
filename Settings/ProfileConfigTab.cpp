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
#include <SourceConfigWidgetFactory.h>
#include <time.h>
#include "ProfileConfigTab.h"
#include "SigDiggerHelpers.h"
#include "ui_ProfileConfigTab.h"

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
  m_modified     = true;
  m_needsRestart = m_needsRestart || restart;

  emit changed();
}

void
ProfileConfigTab::populateProfileCombo()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  ui->profileCombo->clear();

  for (auto i = sus->getFirstProfile(); i != sus->getLastProfile(); ++i)
    ui->profileCombo->addItem(
        QString::fromStdString(i->first),
        QVariant::fromValue(i->second));
}

void
ProfileConfigTab::populateRemoteDeviceCombo()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  ui->remoteDeviceCombo->clear();

  for (
       auto i = sus->getFirstNetworkProfile();
       i != sus->getLastNetworkProfile();
       ++i)
    ui->remoteDeviceCombo->addItem(i->label().c_str());

  if (ui->remoteDeviceCombo->currentIndex() != -1)
    ui->remoteDeviceCombo->setCurrentIndex(0);
}


void
ProfileConfigTab::populateCombos()
{
  populateProfileCombo();
  populateRemoteDeviceCombo();
}

void
ProfileConfigTab::makeConfigWidgets()
{
  int index = 0;
  auto sus = Suscan::Singleton::get_instance();

  QList<SigDigger::SourceConfigWidgetFactory *>::const_iterator p   = sus->getFirstSourceConfigWidgetFactory();
  auto end = sus->getLastSourceConfigWidgetFactory();

  ui->sourceTypeCombo->clear();

  while (p != end) {
    SigDigger::SourceConfigWidgetFactory *factory = *p;
    auto *iface = suscan_source_interface_lookup_by_name(factory->name());

    if (iface != nullptr) {
      auto widget = factory->make();

      m_configWidgets.insert(iface->name, widget);
      ui->sourceTypeCombo->addItem(iface->desc, QString(iface->name));
      ui->sourceConfigStack->insertWidget(index++, widget);

      connect(
            widget,
            SIGNAL(changed()),
            this,
            SLOT(onSourceConfigWidgetChanged()));
    }

    ++p;
  }
}

void
ProfileConfigTab::refreshSampRateCtl()
{
  int index;

  index = m_rateHint == SAMPLE_RATE_CTL_HINT_LIST ? 0 : 1;

  if (ui->sampleRateCombo->count() == 0
      || ui->overrideCheck->isChecked())
    index = 1; // User has overriden this

  ui->sampRateStack->setCurrentIndex(index);
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
  ui->overrideCheck->setEnabled(hint == SAMPLE_RATE_CTL_HINT_LIST);
  refreshSampRateCtl();
}

void
ProfileConfigTab::refreshUiState()
{
  int analyzerTypeIndex = ui->analyzerTypeCombo->currentIndex();
  bool netProfile = ui->useNetworkProfileRadio->isChecked();
  bool adjustStartTime = false;

  ui->analyzerParamsStackedWidget->setCurrentIndex(analyzerTypeIndex);

  ui->localPage->setVisible(!remoteSelected());
  ui->remotePage->setVisible(remoteSelected());

  if (!remoteSelected()) {
    // Set PPM spinbox depending on the source being realtime or not.
    ui->ppmSpinBox->setEnabled(m_profile.isRealTime());
    sampRateCtlHint(
          ui->sampleRateCombo->count() > 0
          ? SAMPLE_RATE_CTL_HINT_LIST
          : SAMPLE_RATE_CTL_HINT_MANUAL);

  } else {
    /* Remote analyzer */
    sampRateCtlHint(SAMPLE_RATE_CTL_HINT_MANUAL);

    if (ui->remoteDeviceCombo->count() == 0) {
      if (netProfile)
        netProfile = false;
      ui->useNetworkProfileRadio->setChecked(false);
      ui->useHostPortRadio->setChecked(true);
      ui->useNetworkProfileRadio->setEnabled(false);
    } else {
      ui->useNetworkProfileRadio->setEnabled(true);
    }

    ui->hostEdit->setEnabled(!netProfile);
    ui->portEdit->setEnabled(!netProfile);
    ui->remoteDeviceCombo->setEnabled(netProfile);
    ui->ppmSpinBox->setEnabled(false);
  }

  ui->sourceTimeEdit->setEnabled(adjustStartTime);
  ui->sourceTimeIsUTCCheck->setEnabled(adjustStartTime);

  setSelectedSampleRate(m_profile.getSampleRate());
  refreshTrueSampleRate();
  BLOCKSIG(ui->frequencySpinBox, setValue(m_profile.getFreq()));
}

void
ProfileConfigTab::refreshSampRates()
{
  QList<int> rates;

  ui->sampleRateCombo->clear();

  if (m_currentConfigWidget != nullptr) {
    m_currentConfigWidget->getPreferredRates(rates);

    for (auto p : rates)
      ui->sampleRateCombo->addItem(
            getSampRateString(p),
            QVariant::fromValue<double>(p));

  }

  if (!remoteSelected())
    sampRateCtlHint(
          ui->sampleRateCombo->count() > 0
          ? SAMPLE_RATE_CTL_HINT_LIST
          : SAMPLE_RATE_CTL_HINT_MANUAL);
}

void
ProfileConfigTab::refreshFrequencyLimits()
{
  SUFREQ lnbFreq = ui->lnbSpinBox->value();
  qint64 devMinFreq = -300000000000;
  qint64 devMaxFreq = +300000000000;

  if (m_currentConfigWidget != nullptr)
    m_currentConfigWidget->getNativeFrequencyLimits(devMinFreq, devMaxFreq);

  BLOCKSIG(ui->frequencySpinBox, setMinimum(devMinFreq + lnbFreq));
  BLOCKSIG(ui->frequencySpinBox, setMaximum(devMaxFreq + lnbFreq));
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
  float step = SU_POW(10., SU_FLOOR(SU_LOG(m_profile.getSampleRate())));
  qreal trueRate = static_cast<qreal>(getSelectedSampleRate())
      / getDecimation();
  if (step >= 10.f)
    step /= 10.f;

  ui->trueRateLabel->setText(getSampRateString(trueRate));
}

void
ProfileConfigTab::refreshAnalyzerTypeUi()
{
  int index = m_profile.getInterface() == SUSCAN_SOURCE_LOCAL_INTERFACE ? 0 : 1;

  ui->analyzerTypeCombo->setCurrentIndex(index);
  ui->analyzerParamsStackedWidget->setCurrentIndex(index);
}

int
ProfileConfigTab::findRemoteProfileIndex()
{
  return ui->remoteDeviceCombo->findText(m_profile.label().c_str());
}

void
ProfileConfigTab::refreshUi()
{
  bool adjustableSourceTime = false;

  for (auto i = 0; i < ui->profileCombo->count(); ++i)
    if (ui->profileCombo->itemText(i).toStdString() ==
        m_profile.label()) {
      BLOCKSIG(ui->profileCombo, setCurrentIndex(i));
      break;
    }

  refreshSampRates();
  setDecimation(m_profile.getDecimation());
  selectSourceType(m_profile.getType());
  setSelectedSampleRate(m_profile.getSampleRate());

  BLOCKSIG(ui->lnbSpinBox,       setValue(m_profile.getLnbFreq()));
  BLOCKSIG(ui->frequencySpinBox, setValue(m_profile.getFreq()));
  BLOCKSIG(ui->iqBalanceCheck,   setChecked(m_profile.getIQBalance()));
  BLOCKSIG(ui->removeDCCheck,    setChecked(m_profile.getDCRemove()));
  BLOCKSIG(ui->ppmSpinBox,       setValue(static_cast<double>(m_profile.getPPM())));

  refreshAnalyzerTypeUi();

  if (m_profile.getInterface() == SUSCAN_SOURCE_LOCAL_INTERFACE) {
    adjustableSourceTime = !m_profile.isRealTime();
  } else {
    bool hasMc;
    std::string host, port, user, pass, mc_if;
    int index;

    host  = m_profile.getParam("host");
    port  = m_profile.getParam("port");
    user  = m_profile.getParam("user");
    pass  = m_profile.getParam("password");
    hasMc = m_profile.hasParam("mc_if");
    mc_if = m_profile.getParam("mc_if");

    // Set remote analyzer interface
    BLOCKSIG(ui->hostEdit, setText(host.c_str()));

    try {
      BLOCKSIG(ui->portEdit, setValue(std::stoi(port)));
    } catch (std::invalid_argument &) {
      BLOCKSIG(ui->portEdit, setValue(28001));
    }

    index = findRemoteProfileIndex();
    if (index != -1) {
      BLOCKSIG(ui->useNetworkProfileRadio, setChecked(true));
      BLOCKSIG(ui->useHostPortRadio,       setChecked(false));
      BLOCKSIG(ui->remoteDeviceCombo,      setCurrentIndex(index));
    } else {
      BLOCKSIG(ui->useHostPortRadio,       setChecked(true));
      BLOCKSIG(ui->useNetworkProfileRadio, setChecked(false));
    }

    BLOCKSIG(ui->userEdit,        setText(user.c_str()));
    BLOCKSIG(ui->passEdit,        setText(pass.c_str()));
    BLOCKSIG(ui->mcCheck,         setChecked(hasMc));
    BLOCKSIG(ui->mcInterfaceEdit, setText(mc_if.c_str()));

    ui->mcInterfaceEdit->setEnabled(hasMc);
  }

  if (adjustableSourceTime) {
    struct timeval startTime = m_profile.getStartTime();
    qint64 epochMsec = startTime.tv_sec * 1000 + startTime.tv_usec / 1000;
    QDateTime dateTime;
    dateTime.setTimeSpec(
          ui->sourceTimeIsUTCCheck->isChecked()
          ? Qt::TimeSpec::UTC
          : Qt::TimeSpec::LocalTime);

    dateTime.setMSecsSinceEpoch(epochMsec);

    BLOCKSIG(ui->sourceTimeEdit, setTimeSpec(dateTime.timeSpec()));
    BLOCKSIG(ui->sourceTimeEdit, setDateTime(dateTime));
  } else {
    BLOCKSIG(ui->sourceTimeEdit, setDateTime(
          QDateTime::fromSecsSinceEpoch(
            static_cast<unsigned int>(time(nullptr)))));
  }

  refreshFrequencyLimits();
  refreshUiState();
  refreshTrueSampleRate();
}

void
ProfileConfigTab::save()
{
  // Do nothing. The profile is already updated. The information in this
  // object is so complex it is better off to store the state directly
  // in it.
}

void
ProfileConfigTab::setUnchanged()
{
  m_modified     = false;
  m_needsRestart = false;
}

bool
ProfileConfigTab::hasChanged() const
{
  return m_modified;
}

bool
ProfileConfigTab::shouldRestart() const
{
  return m_needsRestart;
}

void
ProfileConfigTab::connectAll()
{

  connect(
        ui->loadProfileButton,
        SIGNAL(clicked()),
        this,
        SLOT(onLoadProfileClicked()));

  connect(
        ui->sourceTypeCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onChangeSourceType(int)));

  connect(
        ui->frequencySpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        ui->lnbSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        ui->sampleRateSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        ui->decimCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        ui->sampleRateCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        ui->ppmSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        ui->removeDCCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onCheckButtonsToggled(bool)));

  connect(
        ui->iqBalanceCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onCheckButtonsToggled(bool)));

  connect(
        ui->saveProfileButton,
        SIGNAL(clicked()),
        this,
        SLOT(onSaveProfile()));

  connect(
        ui->analyzerTypeCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onAnalyzerTypeChanged(int)));

  connect(
        ui->hostEdit,
        SIGNAL(textEdited(const QString &)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        ui->portEdit,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        ui->userEdit,
        SIGNAL(textEdited(const QString &)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        ui->passEdit,
        SIGNAL(textEdited(const QString &)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        ui->mcCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        ui->mcInterfaceEdit,
        SIGNAL(textChanged(const QString &)),
        this,
        SLOT(onRemoteParamsChanged()));

  connect(
        ui->useNetworkProfileRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onChangeConnectionType()));

  connect(
        ui->useHostPortRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onChangeConnectionType()));

  connect(
        ui->remoteDeviceCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onRemoteProfileSelected()));

  connect(
        ui->refreshButton,
        SIGNAL(clicked()),
        this,
        SLOT(onRefreshRemoteDevices()));

  connect(
        ui->ppmSpinBox,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        ui->sourceTimeEdit,
        SIGNAL(dateTimeChanged(QDateTime const &)),
        this,
        SLOT(onSpinsChanged()));

  connect(
        ui->sourceTimeIsUTCCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onChangeSourceTimeUTC()));

  connect(
        ui->overrideCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onOverrideSampleRate()));
}

void
ProfileConfigTab::setProfile(const Suscan::Source::Config &profile)
{
  m_currentConfigWidget = nullptr;
  m_currentConfigIndex = -1;

  setUnchanged();
  loadProfile(profile);
}

void
ProfileConfigTab::setFrequency(qint64 val)
{
  m_profile.setFreq(val);
  BLOCKSIG(ui->frequencySpinBox, setValue(val));
}

void
ProfileConfigTab::notifySingletonChanges()
{
  for (auto p : m_configWidgets)
    p->notifySingletonChanges();

  populateCombos();
  refreshUi();
}

bool
ProfileConfigTab::remoteSelected() const
{
  return ui->analyzerTypeCombo->currentIndex() == 1;
}

void
ProfileConfigTab::setGain(std::string const &name, float value)
{
  m_profile.setGain(name, value);
}

float
ProfileConfigTab::getGain(std::string const &name) const
{
  return m_profile.getGain(name);
}

Suscan::Source::Config
ProfileConfigTab::getProfile() const
{
  return m_profile;
}

unsigned
ProfileConfigTab::getDecimation() const
{
  if (ui->decimCombo->currentIndex() < 0)
    return 0;

  return 1u << ui->decimCombo->currentIndex();
}

void
ProfileConfigTab::setDecimation(unsigned decim)
{
  int i = 0;

  while ((1u << i) < decim && i != ui->decimCombo->count() - 1)
    ++i;

  BLOCKSIG(ui->decimCombo, setCurrentIndex(i));
}

void
ProfileConfigTab::updateRemoteParams()
{
  m_profile.clearParams();

  m_profile.setParam("host", ui->hostEdit->text().toStdString());
  m_profile.setParam("port", std::to_string(ui->portEdit->value()));
  m_profile.setParam("user", ui->userEdit->text().toStdString());
  m_profile.setParam("password", ui->passEdit->text().toStdString());

  if (ui->mcCheck->isChecked())
    m_profile.setParam(
        "mc_if",
        ui->mcInterfaceEdit->text().toStdString());

  m_profile.setParam("label", "User-defined remote profile");
}

ProfileConfigTab::ProfileConfigTab(QWidget *parent) :
  ConfigTab(parent, "Source"),
  m_profile("soapysdr", SUSCAN_SOURCE_FORMAT_AUTO)
{
  ui = new Ui::ProfileConfigTab;
  ui->setupUi(this);

  m_saveProfileDialog = new SaveProfileDialog(this);

  // Set local analyzer as default
  ui->analyzerTypeCombo->setCurrentIndex(0);

  // Setup remote device
  m_remoteDevice = Suscan::Source::Device(
          "Remote device",
          "localhost",
          28001,
          "anonymous",
          "");

  // Setup sample rate size
  ui->trueRateLabel->setFixedWidth(
        SuWidgetsHelpers::getWidgetTextWidth(
          ui->trueRateLabel,
          "XXX.XXX Xsps"));

  // Set limits
  ui->lnbSpinBox->setMaximum(300e9);
  ui->lnbSpinBox->setMinimum(-300e9);

  // Make configuration widgets
  makeConfigWidgets();

  // Populate locations
  populateCombos();
  ui->sampleRateSpinBox->setUnits("sps");
  connectAll();
  refreshUi();

}

QString
ProfileConfigTab::getBaseName(const QString &path)
{
  int ndx;

  if ((ndx = path.lastIndexOf('/')) != -1)
    return path.right(path.size() - ndx - 1);

  return path;
}

bool
ProfileConfigTab::selectSourceType(std::string const &type)
{
  QString asQString = QString::fromStdString(type);
  SourceConfigWidget *next = nullptr;

  if (m_configWidgets.find(asQString) != m_configWidgets.end())
    next = m_configWidgets[asQString];

  if (next != m_currentConfigWidget) {
    // Previous widget present, call deactivate
    if (!tryLeaveCurrentConfigWidget())
      return false;

    // New widget, set current widget and call activate
    m_currentConfigWidget = next;

    if (next != nullptr) {
      ui->sourceConfigGroupBox->setVisible(true);
      ui->sourceConfigStack->setCurrentWidget(next);
      BLOCKSIG(
            ui->sourceTypeCombo,
            setCurrentIndex(ui->sourceConfigStack->currentIndex()));
      next->activateWidget();
    } else {
      ui->sourceConfigGroupBox->setVisible(false);
    }

    // Refresh sample rate combo
    refreshSampRates();
  }

  return true;
}

bool
ProfileConfigTab::tryLeaveCurrentConfigWidget()
{
  if (m_currentConfigWidget != nullptr)
    if (!m_currentConfigWidget->deactivateWidget())
      return false;

  m_currentConfigWidget = nullptr;

  return true;
}

void
ProfileConfigTab::loadProfile(Suscan::Source::Config const &config)
{
  m_profile = config;

  for (auto p: m_configWidgets)
    p->setConfigRef(m_profile);

  refreshUi();
}

ProfileConfigTab::~ProfileConfigTab()
{
  delete ui;
}

//////////////// Slots //////////////////
void
ProfileConfigTab::onLoadProfileClicked()
{
  QVariant data = ui->profileCombo->itemData(ui->profileCombo->currentIndex());

  if (tryLeaveCurrentConfigWidget()) {
    configChanged(true);
    loadProfile(data.value<Suscan::Source::Config>());
  }
}

void
ProfileConfigTab::onChangeSourceType(int)
{
  QVariant data = ui->sourceTypeCombo->currentData();
  QString name = data.value<QString>();

  if (name == nullptr)
    return;

  if (selectSourceType(name.toStdString())) {
    m_profile.setType(name.toStdString());
    configChanged(true);
    refreshUiState();
    refreshFrequencyLimits();
  } else {
    BLOCKSIG(ui->sourceTypeCombo, setCurrentIndex(m_currentConfigIndex));
  }
}

void
ProfileConfigTab::onSourceConfigWidgetChanged()
{
  SigDigger::SourceConfigWidget *widget =
      static_cast<SigDigger::SourceConfigWidget *>(QObject::sender());

  if (widget == m_currentConfigWidget) {
    // Changes from the current config widget. Maybe the preferred list of
    // rates has changed?

    configChanged(true);
    refreshSampRates();
    refreshUiState(); // maybe sample rate and/or freq changed
  }
}

void
ProfileConfigTab::onAnalyzerTypeChanged(int index)
{
  switch (index) {
    case 0:
      m_profile.setInterface(SUSCAN_SOURCE_LOCAL_INTERFACE);
      selectSourceType(m_profile.getType());
      break;

    case 1:
      if (!tryLeaveCurrentConfigWidget()) {
        refreshUi();
        return;
      }

      m_profile.setInterface(SUSCAN_SOURCE_REMOTE_INTERFACE);
      onChangeConnectionType();
      onRemoteParamsChanged();
      break;
  }

  configChanged(true);
  refreshUiState();
}

void
ProfileConfigTab::onRemoteParamsChanged()
{
  if (remoteSelected()) {
    ui->mcInterfaceEdit->setEnabled(ui->mcCheck->isChecked());
    configChanged(true);
    m_profile.setDevice(m_remoteDevice);
    updateRemoteParams();
  }
}

void
ProfileConfigTab::onCheckButtonsToggled(bool)
{
  if (m_profile.getDCRemove() != ui->removeDCCheck->isChecked()) {
    m_profile.setDCRemove(ui->removeDCCheck->isChecked());
    configChanged();
  }

  if (m_profile.getIQBalance() != ui->iqBalanceCheck->isChecked()) {
    m_profile.setIQBalance(ui->iqBalanceCheck->isChecked());
    configChanged();
  }
}

unsigned int
ProfileConfigTab::getSelectedSampleRate() const
{
  unsigned int sampRate = 0;

  if (ui->sampRateStack->currentIndex() == 0) {
    // Index 0: Sample Rate Combo
    if (ui->sampleRateCombo->currentIndex() != -1) {
      qreal selectedValue =
          ui->sampleRateCombo->currentData().value<qreal>();
      sampRate = static_cast<unsigned>(selectedValue);
    }
  } else {
    // Index 1: Sample Rate Spin
    sampRate = static_cast<unsigned>(
          ui->sampleRateSpinBox->value());
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
  for (auto i = 0; i < ui->sampleRateCombo->count(); ++i) {
    qreal value = ui->sampleRateCombo->itemData(i).value<qreal>();
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
      BLOCKSIG(ui->sampleRateCombo, setCurrentIndex(bestIndex));
    else
      BLOCKSIG(ui->overrideCheck,   setChecked(true));
  }

  BLOCKSIG(ui->sampleRateSpinBox, setValue(rate));
}

void
ProfileConfigTab::onSpinsChanged()
{
  SUFREQ freq;
  SUFREQ lnbFreq;
  SUFLOAT ppm;

  time_t timeStamp;
  time_t timeStampUsec;

  unsigned int sampRate;
  unsigned int decimation;

  lnbFreq = ui->lnbSpinBox->value();
  refreshFrequencyLimits();
  freq = ui->frequencySpinBox->value();
  sampRate = getSelectedSampleRate();
  decimation = getDecimation();
  ppm = static_cast<SUFLOAT>(ui->ppmSpinBox->value());
  timeStamp = ui->sourceTimeEdit->dateTime().toSecsSinceEpoch();
  timeStampUsec = 1000 * (
        ui->sourceTimeEdit->dateTime().toMSecsSinceEpoch() % 1000);

  if (!sufeq(m_profile.getFreq(), freq, .5f)) {
    m_profile.setFreq(freq);
    configChanged();
  }

  if (!sufeq(m_profile.getLnbFreq(), lnbFreq, .5f)) {
    m_profile.setLnbFreq(lnbFreq);
    configChanged();
  }

  if (m_profile.getSampleRate() != sampRate) {
    m_profile.setSampleRate(sampRate);
    configChanged(true);
  }

  if (m_profile.getDecimation() != decimation) {
    m_profile.setDecimation(decimation);
    configChanged(true);
  }

  if (!sufeq(m_profile.getPPM(), ppm, .5f)) {
    m_profile.setPPM(ppm);
    configChanged();
  }

  if (ui->sourceTimeEdit->isEnabled()
      && (m_profile.getStartTime().tv_sec != timeStamp
      || m_profile.getStartTime().tv_usec != timeStampUsec)) {
    struct timeval tv;
    tv.tv_sec  = timeStamp;
    tv.tv_usec = timeStampUsec;
    m_profile.setStartTime(tv);
    configChanged(true);
  }

  refreshTrueSampleRate();
}

void
ProfileConfigTab::onSaveProfile()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  std::string name = "My " + m_profile.label();
  std::string candidate = name;
  unsigned int i = 1;

  while (sus->getProfile(candidate) != nullptr)
    candidate = name + " (" + std::to_string(i++) + ")";

  m_saveProfileDialog->setProfileName(QString::fromStdString(candidate));

  if (m_saveProfileDialog->run()) {
    candidate = m_saveProfileDialog->getProfileName().toStdString();

    if (sus->getProfile(candidate) != nullptr) {
      QMessageBox::warning(
            this,
            "Profile already exists",
            "There is already a profile named " +
            m_saveProfileDialog->getProfileName() +
            " please choose a different one.",
            QMessageBox::Ok);
      return;
    }

    m_profile.setLabel(candidate);
    sus->saveProfile(m_profile);
    populateProfileCombo();
  }
}

void
ProfileConfigTab::onChangeConnectionType()
{
  if (ui->useNetworkProfileRadio->isChecked()) {
    onRemoteProfileSelected();
    ui->useHostPortRadio->setChecked(false);
  }

  if (ui->useHostPortRadio->isChecked()) {
    onRemoteParamsChanged();
    ui->useNetworkProfileRadio->setChecked(false);
  }

  configChanged(true);
  refreshUiState();
}

void
ProfileConfigTab::onRefreshRemoteDevices()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  int countBefore = ui->remoteDeviceCombo->count();
  int countAfter;

  sus->refreshNetworkProfiles();
  populateRemoteDeviceCombo();

  countAfter = ui->remoteDeviceCombo->count();

  if (countAfter > countBefore) {
    ui->useNetworkProfileRadio->setChecked(true);
    onChangeConnectionType();
  } else {
    refreshUiState();
  }

  onRemoteParamsChanged();
}

void
ProfileConfigTab::onRemoteProfileSelected()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  if (ui->useNetworkProfileRadio->isChecked()) {
    QHash<QString, Suscan::Source::Config>::const_iterator it;
    std::string user, pass, mc, mc_if;
    bool hasMc;

    // Save multicast config
    hasMc = ui->mcCheck->isChecked();
    mc_if = ui->mcInterfaceEdit->text().toStdString();

    it = sus->getNetworkProfileFrom(ui->remoteDeviceCombo->currentText());

    if (it != sus->getLastNetworkProfile()) {
      user = it->getParam("user");
      pass = it->getParam("password");

      if (user.length() == 0)
        user = ui->userEdit->text().toStdString();
      if (user.length() == 0)
        user = "anonymous";

      if (pass.length() == 0)
        pass = ui->passEdit->text().toStdString();

      configChanged(true);
      setProfile(*it);

      // Provide a better hint for username if the server announced none
      ui->userEdit->setText(user.c_str());
      ui->passEdit->setText(pass.c_str());

      // Restore mc config
      ui->mcCheck->setChecked(hasMc);
      ui->mcInterfaceEdit->setText(mc_if.c_str());
      ui->mcInterfaceEdit->setEnabled(hasMc);

      onRemoteParamsChanged();
    }
  }
}

void
ProfileConfigTab::onChangeSourceTimeUTC()
{
  QDateTime dateTime = ui->sourceTimeEdit->dateTime();
  qint64 epochMsec   = dateTime.toMSecsSinceEpoch();

  dateTime.setTimeSpec(
        ui->sourceTimeIsUTCCheck->isChecked()
        ? Qt::TimeSpec::UTC
        : Qt::TimeSpec::LocalTime);

  dateTime.setMSecsSinceEpoch(epochMsec);

  ui->sourceTimeEdit->setTimeSpec(dateTime.timeSpec());
  ui->sourceTimeEdit->setDateTime(dateTime);
}

void
ProfileConfigTab::onOverrideSampleRate()
{
  if (!ui->overrideCheck->isChecked()) {
    refreshSampRateCtl();
    onSpinsChanged();
  }

  refreshUiState();
}
