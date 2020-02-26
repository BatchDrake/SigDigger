//
//    ConfigDialog.cpp: Configuration dialog window
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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

#include "ConfigDialog.h"

using namespace SigDigger;

Q_DECLARE_METATYPE(Suscan::Source::Config); // Unicorns
Q_DECLARE_METATYPE(Suscan::Source::Device); // More unicorns

void
ConfigDialog::populateCombos(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  this->ui->profileCombo->clear();
  this->ui->deviceCombo->clear();

  for (auto i = sus->getFirstProfile(); i != sus->getLastProfile(); ++i)
      this->ui->profileCombo->addItem(
            QString::fromStdString(i->first),
            QVariant::fromValue(i->second));

  for (auto i = sus->getFirstDevice(); i != sus->getLastDevice(); ++i)
    if (i->isAvailable())
      this->ui->deviceCombo->addItem(
          QString::fromStdString(i->getDesc()),
          QVariant::fromValue<long>(i - sus->getFirstDevice()));

  if (this->ui->deviceCombo->currentIndex() == -1)
    this->ui->deviceCombo->setCurrentIndex(0);

  this->onDeviceChanged(this->ui->deviceCombo->currentIndex());
}

void
ConfigDialog::refreshUiState(void)
{
  if (this->ui->sdrRadio->isChecked()) {
    this->ui->sdrFrame->setEnabled(true);
    this->ui->fileFrame->setEnabled(false);
    this->ui->sampRateStack->setCurrentIndex(0);
  } else {
    this->ui->sdrFrame->setEnabled(false);
    this->ui->fileFrame->setEnabled(true);
    this->ui->sampRateStack->setCurrentIndex(1);
  }

  this->setSelectedSampleRate(this->profile.getSampleRate());
  this->refreshTrueSampleRate();
}

void
ConfigDialog::refreshAntennas(void)
{
  populateAntennaCombo(this->profile, this->ui->antennaCombo);
}

void
ConfigDialog::refreshSampRates(void)
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

#define APSTOREF(widget, field) \
  this->ui->widget->setText(QString::number(static_cast<qreal>(\
    this->analyzerParams.field)))
#define APSTOREI(widget, field) \
  this->ui->widget->setText(QString::number(this->analyzerParams.field))
#define APSAVEF(widget, field) \
  this->analyzerParams.field = this->ui->widget->text().toFloat()
#define APSAVEI(widget, field) \
  this->analyzerParams.field = this->ui->widget->text().toUInt()

void
ConfigDialog::saveAnalyzerParams(void)
{
  APSAVEF(spectAvgAlphaEdit, spectrumAvgAlpha);
  APSAVEF(sLevelAvgAlphaEdit, sAvgAlpha);
  APSAVEF(nLevelAvgAlphaEdit, nAvgAlpha);
  APSAVEF(snrThresholdEdit, snr);
  APSAVEF(spectrumRefreshEdit, psdUpdateInterval);
  APSAVEF(channelRefreshEdit, channelUpdateInterval);
  APSAVEI(fftSizeEdit, windowSize);

  this->analyzerParams.psdUpdateInterval *= 1e-3f;
  this->analyzerParams.channelUpdateInterval *= 1e-3f;

  if (this->ui->rectangularRadio->isChecked())
    this->analyzerParams.windowFunction = Suscan::AnalyzerParams::NONE;
  else if (this->ui->hammingRadio->isChecked())
    this->analyzerParams.windowFunction = Suscan::AnalyzerParams::HAMMING;
  else if (this->ui->hannRadio->isChecked())
    this->analyzerParams.windowFunction = Suscan::AnalyzerParams::HANN;
  else if (this->ui->flatTopRadio->isChecked())
    this->analyzerParams.windowFunction = Suscan::AnalyzerParams::FLAT_TOP;
  else if (this->ui->blackmannHarrisRadio->isChecked())
    this->analyzerParams.windowFunction = Suscan::AnalyzerParams::BLACKMANN_HARRIS;
}

void
ConfigDialog::refreshAnalyzerParamsUi(void)
{
  this->analyzerParams.psdUpdateInterval *= 1e3f;
  this->analyzerParams.channelUpdateInterval *= 1e3f;

  APSTOREF(spectAvgAlphaEdit, spectrumAvgAlpha);
  APSTOREF(sLevelAvgAlphaEdit, sAvgAlpha);
  APSTOREF(nLevelAvgAlphaEdit, nAvgAlpha);
  APSTOREF(snrThresholdEdit, snr);
  APSTOREF(spectrumRefreshEdit, psdUpdateInterval);
  APSTOREF(channelRefreshEdit, channelUpdateInterval);
  APSTOREI(fftSizeEdit, windowSize);

  this->analyzerParams.psdUpdateInterval *= 1e-3f;
  this->analyzerParams.channelUpdateInterval *= 1e-3f;

  switch (this->analyzerParams.windowFunction) {
    case Suscan::AnalyzerParams::NONE:
      this->ui->rectangularRadio->setChecked(true);
      break;

    case Suscan::AnalyzerParams::HAMMING:
      this->ui->hammingRadio->setChecked(true);
      break;

    case Suscan::AnalyzerParams::HANN:
      this->ui->hannRadio->setChecked(true);
      break;

    case Suscan::AnalyzerParams::FLAT_TOP:
      this->ui->flatTopRadio->setChecked(true);
      break;

    case Suscan::AnalyzerParams::BLACKMANN_HARRIS:
      this->ui->blackmannHarrisRadio->setChecked(true);
      break;
  }
}

#define CCREFRESH(widget, field) this->ui->widget->setColor(this->colors.field)
#define CCSAVE(widget, field) this->ui->widget->getColor(this->colors.field)

void
ConfigDialog::saveColors(void)
{
  CCSAVE(lcdFgColor, lcdForeground);
  CCSAVE(lcdBgColor, lcdBackground);
  CCSAVE(spectrumFgColor, spectrumForeground);
  CCSAVE(spectrumBgColor, spectrumBackground);
  CCSAVE(spectrumAxesColor, spectrumAxes);
  CCSAVE(spectrumTextColor, spectrumText);
  CCSAVE(constellationFgColor, constellationForeground);
  CCSAVE(constellationBgColor, constellationBackground);
  CCSAVE(constellationAxesColor, constellationAxes);
  CCSAVE(transitionFgColor, transitionForeground);
  CCSAVE(transitionBgColor, transitionBackground);
  CCSAVE(transitionAxesColor, transitionAxes);
  CCSAVE(histogramFgColor, histogramForeground);
  CCSAVE(histogramBgColor, histogramBackground);
  CCSAVE(histogramAxesColor, histogramAxes);
  CCSAVE(histogramModelColor, histogramModel);
}

void
ConfigDialog::refreshColorUi(void)
{
  CCREFRESH(lcdFgColor, lcdForeground);
  CCREFRESH(lcdBgColor, lcdBackground);
  CCREFRESH(spectrumFgColor, spectrumForeground);
  CCREFRESH(spectrumBgColor, spectrumBackground);
  CCREFRESH(spectrumAxesColor, spectrumAxes);
  CCREFRESH(spectrumTextColor, spectrumText);
  CCREFRESH(constellationFgColor, constellationForeground);
  CCREFRESH(constellationBgColor, constellationBackground);
  CCREFRESH(constellationAxesColor, constellationAxes);
  CCREFRESH(transitionFgColor, transitionForeground);
  CCREFRESH(transitionBgColor, transitionBackground);
  CCREFRESH(transitionAxesColor, transitionAxes);
  CCREFRESH(histogramFgColor, histogramForeground);
  CCREFRESH(histogramBgColor, histogramBackground);
  CCREFRESH(histogramAxesColor, histogramAxes);
  CCREFRESH(histogramModelColor, histogramModel);
}

QString
ConfigDialog::getSampRateString(qreal trueRate)
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
ConfigDialog::refreshTrueSampleRate(void)
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
ConfigDialog::adjustSpinUnits(QDoubleSpinBox *sb, QString const &units)
{
  int decimals = sb->decimals();

  switch (decimals) {
    case 0:
      sb->setSuffix("  " + units);
      sb->setMaximum(18e9);
      break;

    case 3:
      sb->setSuffix(" k" + units);
      sb->setMaximum(18e6);
      break;

    case 6:
      sb->setSuffix(" M" + units);
      sb->setMaximum(18e3);
      break;

    case 9:
      sb->setSuffix(" G" + units);
      sb->setMaximum(18);
      break;
  }
}

void
ConfigDialog::incSpinUnits(QDoubleSpinBox *sb, QString const &units)
{
  int decimals = sb->decimals() + 3;

  if (decimals >= 0 && decimals <= 9) {
    qreal value = sb->value() * 1e-3;

    sb->setValue(value);
    sb->setDecimals(decimals);

    adjustSpinUnits(sb, units);
  }
}

void
ConfigDialog::decSpinUnits(QDoubleSpinBox *sb, QString const &units)
{
  int decimals = sb->decimals() - 3;

  if (decimals >= 0 && decimals <= 9) {
    qreal value = sb->value() * 1e3;

    sb->setValue(value);
    sb->setDecimals(decimals);

    adjustSpinUnits(sb, units);
  }
}

bool
ConfigDialog::spinCanIncrease(const QDoubleSpinBox *sb)
{
  return sb->decimals() < 9;
}

bool
ConfigDialog::spinCanDecrease(const QDoubleSpinBox *sb)
{
  return sb->decimals() > 0;
}

void
ConfigDialog::setSpinValue(
    QDoubleSpinBox *sb,
    qreal value,
    QString const &units)
{
  qreal multiplier = 1;
  int decimals = 0;

  if (value >= 1e9) {
    multiplier = 1e-9;
    decimals = 9;
  } else if (value >= 1e6) {
    multiplier =  1e-6;
    decimals = 6;
  } else if (value >= 1e3) {
    multiplier =  1e-3;
    decimals = 3;
  }

  sb->setValue(value * multiplier);
  sb->setDecimals(decimals);
  adjustSpinUnits(sb, units);
}

qreal
ConfigDialog::getSpinValue(QDoubleSpinBox *sb)
{
  qreal value = sb->value();

  switch (sb->decimals()) {
    case 9:
      value *= 1e9;
      break;

    case 6:
      value *= 1e6;
      break;

    case 3:
      value *= 1e3;
      break;
  }

  return value;
}

void
ConfigDialog::refreshProfileUi(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  for (auto i = 0; i < this->ui->profileCombo->count(); ++i)
    if (this->ui->profileCombo->itemText(i).toStdString() ==
        this->profile.label()) {
      this->ui->profileCombo->setCurrentIndex(i);
      break;
    }

  this->refreshSampRates();

  setSpinValue(
        this->ui->frequencySpin,
        this->profile.getFreq(),
        "Hz");

  setSpinValue(
        this->ui->lnbSpin,
        this->profile.getLnbFreq(),
        "Hz");

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
  setSpinValue(
        this->ui->bwSpin,
        static_cast<qreal>(this->profile.getBandwidth()),
        "Hz");

  switch (this->profile.getFormat()) {
    case SUSCAN_SOURCE_FORMAT_AUTO:
      this->ui->formatCombo->setCurrentIndex(0);
      break;

    case SUSCAN_SOURCE_FORMAT_RAW:
      this->ui->formatCombo->setCurrentIndex(1);
      break;

    case SUSCAN_SOURCE_FORMAT_WAV:
      this->ui->formatCombo->setCurrentIndex(2);
      break;
  }

  this->ui->pathEdit->setText(QString::fromStdString(this->profile.getPath()));

  this->ui->deviceCombo->setCurrentIndex(-1);

  for (auto i = sus->getFirstDevice(); i != sus->getLastDevice(); ++i) {
    if (i->equals(this->profile.getDevice())) {
      int index = this->ui->deviceCombo->findData(
            QVariant::fromValue(
              static_cast<long>(i - sus->getFirstDevice())));

      if (index != -1)
        this->ui->deviceCombo->setCurrentIndex(index);

      break;
    }
  }

  this->refreshUiState();
  this->refreshAntennas();

  this->refreshTrueSampleRate();
}

void
ConfigDialog::refreshUnitButtons(void)
{
  this->ui->incFreqUnitsButton->setEnabled(
        spinCanIncrease(this->ui->frequencySpin));
  this->ui->decFreqUnitsButton->setEnabled(
        spinCanDecrease(this->ui->frequencySpin));

  this->ui->incLNBUnitsButton->setEnabled(
        spinCanIncrease(this->ui->lnbSpin));
  this->ui->decLNBUnitsButton->setEnabled(
        spinCanDecrease(this->ui->lnbSpin));

  this->ui->incSampRateUnitsButton->setEnabled(
        spinCanIncrease(this->ui->sampleRateSpin));
  this->ui->decSampRateUnitsButton->setEnabled(
        spinCanDecrease(this->ui->sampleRateSpin));
}

void
ConfigDialog::refreshUi(void)
{
  this->refreshing = true;

  this->refreshColorUi();
  this->refreshProfileUi();
  this->refreshUnitButtons();

  this->refreshing = false;
}

void
ConfigDialog::connectAll(void)
{
  connect(
        this->ui->deviceCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onDeviceChanged(int)));

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
        this->ui->frequencySpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged(void)));

  connect(
        this->ui->lnbSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onSpinsChanged(void)));

  connect(
        this->ui->sampleRateSpin,
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
        this->ui->bwSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onBandwidthChanged(double)));

  connect(
        this->ui->formatCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onFormatChanged(int)));

  connect(
        this,
        SIGNAL(accepted(void)),
        this,
        SLOT(onAccepted(void)));

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
        this->ui->incFreqUnitsButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onIncFreqUnits(void)));

  connect(
        this->ui->decFreqUnitsButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onDecFreqUnits(void)));

  connect(
        this->ui->incLNBUnitsButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onIncLNBUnits(void)));

  connect(
        this->ui->decLNBUnitsButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onDecLNBUnits(void)));

  connect(
        this->ui->incSampRateUnitsButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onIncSampRateUnits(void)));

  connect(
        this->ui->decSampRateUnitsButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onDecSampRateUnits(void)));
}

void
ConfigDialog::setAnalyzerParams(const Suscan::AnalyzerParams &params)
{
  this->analyzerParams = params;
  this->refreshAnalyzerParamsUi();
}

void
ConfigDialog::setProfile(const Suscan::Source::Config &profile)
{
  this->profile = profile;
  this->refreshUi();
}

void
ConfigDialog::setFrequency(qint64 val)
{
  this->profile.setFreq(val);
}

void
ConfigDialog::notifySingletonChanges(void)
{
  this->populateCombos();
  this->refreshUi();
}

void
ConfigDialog::setGain(std::string const &name, float value)
{
  this->profile.setGain(name, value);
}

float
ConfigDialog::getGain(std::string const &name)
{
  return this->profile.getGain(name);
}

Suscan::AnalyzerParams
ConfigDialog::getAnalyzerParams(void)
{
  return this->analyzerParams;
}

Suscan::Source::Config
ConfigDialog::getProfile(void)
{
  return this->profile;
}

void
ConfigDialog::setColors(ColorConfig const &config)
{
  this->colors = config;
  this->refreshUi();
}

ColorConfig
ConfigDialog::getColors(void)
{
  return this->colors;
}

ConfigDialog::ConfigDialog(QWidget *parent) :
  QDialog(parent),
  profile(SUSCAN_SOURCE_TYPE_FILE, SUSCAN_SOURCE_FORMAT_AUTO)
{
  this->ui = new Ui_Config();
  this->ui->setupUi(this);
  this->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
  this->layout()->setSizeConstraint(QLayout::SetFixedSize);

  // Setup integer validators
  this->ui->fftSizeEdit->setValidator(new QIntValidator(1, 1 << 20, this));
  this->ui->spectrumRefreshEdit->setValidator(new QIntValidator(1, 1 << 20, this));
  this->ui->channelRefreshEdit->setValidator(new QIntValidator(1, 1 << 20, this));

  // Setup double validators
  this->ui->spectAvgAlphaEdit->setValidator(new QDoubleValidator(0., 1., 10, this));
  this->ui->sLevelAvgAlphaEdit->setValidator(new QDoubleValidator(0., 1., 10, this));
  this->ui->nLevelAvgAlphaEdit->setValidator(new QDoubleValidator(0., 1., 10, this));
  this->ui->snrThresholdEdit->setValidator(new QDoubleValidator(0., 10., 10, this));

  this->populateCombos();
  this->connectAll();
  this->refreshUi();
}

QString
ConfigDialog::getBaseName(const QString &path)
{
  int ndx;

  if ((ndx = path.lastIndexOf('/')) != -1)
    return path.right(path.size() - ndx - 1);

  return path;
}


ConfigDialog::~ConfigDialog()
{
  delete this->ui;
}

//////////////// Slots //////////////////
void
ConfigDialog::onLoadProfileClicked(void)
{
  QVariant data = this->ui->profileCombo->itemData(this->ui->profileCombo->currentIndex());

  this->profile = data.value<Suscan::Source::Config>();

  this->refreshUi();
}

void
ConfigDialog::onToggleSourceType(bool)
{
  if (!this->refreshing) {
    if (this->ui->sdrRadio->isChecked())
      this->profile.setType(SUSCAN_SOURCE_TYPE_SDR);
    else
      this->profile.setType(SUSCAN_SOURCE_TYPE_FILE);

    this->refreshUiState();
  }
}

void
ConfigDialog::onDeviceChanged(int index)
{
  if (!this->refreshing && index != -1) {
    Suscan::Singleton *sus = Suscan::Singleton::get_instance();

    const Suscan::Source::Device *device;

    SU_ATTEMPT(
          device = sus->getDeviceAt(
            static_cast<unsigned int>(
            this->ui->deviceCombo->itemData(index).value<long>())));

    this->profile.setDevice(*device);

    this->refreshUi();
  }
}

void
ConfigDialog::onFormatChanged(int index)
{
  if (!this->refreshing) {
    switch (index) {
      case 0:
        this->profile.setFormat(SUSCAN_SOURCE_FORMAT_AUTO);
        break;

      case 1:
        this->profile.setFormat(SUSCAN_SOURCE_FORMAT_RAW);
        break;

      case 2:
        this->profile.setFormat(SUSCAN_SOURCE_FORMAT_WAV);
        break;
    }
  }
}

void
ConfigDialog::onCheckButtonsToggled(bool)
{
  if (!this->refreshing) {
    this->profile.setDCRemove(this->ui->removeDCCheck->isChecked());
    this->profile.setIQBalance(this->ui->iqBalanceCheck->isChecked());
    this->profile.setLoop(this->ui->loopCheck->isChecked());
  }
}

unsigned int
ConfigDialog::getSelectedSampleRate(void) const
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
    sampRate = static_cast<unsigned>(getSpinValue(this->ui->sampleRateSpin));
  }

  return sampRate;
}

void
ConfigDialog::setSelectedSampleRate(unsigned int rate)
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

  setSpinValue(this->ui->sampleRateSpin, rate, "sps");
}

void
ConfigDialog::onSpinsChanged(void)
{
  if (!this->refreshing) {
    SUFREQ freq;
    SUFREQ lnbFreq;
    unsigned int sampRate;

    freq = getSpinValue(this->ui->frequencySpin);
    lnbFreq = getSpinValue(this->ui->lnbSpin);
    sampRate = this->getSelectedSampleRate();

    this->profile.setFreq(freq);
    this->profile.setLnbFreq(lnbFreq);
    this->profile.setSampleRate(sampRate);
    this->profile.setDecimation(
          static_cast<unsigned>(this->ui->decimationSpin->value()));

    if (sampRate < getSpinValue(this->ui->bwSpin))
      setSpinValue(this->ui->bwSpin, sampRate, "Hz");

    this->refreshTrueSampleRate();
  }
}

bool
ConfigDialog::run(void)
{
  this->accepted = false;

  this->exec();

  return this->accepted;
}

void
ConfigDialog::onBandwidthChanged(double)
{
  if (!this->refreshing)
    this->profile.setBandwidth(
        static_cast<SUFLOAT>(getSpinValue(this->ui->bwSpin)));
}

void
ConfigDialog::onAccepted(void)
{
  this->saveColors();
  this->saveAnalyzerParams();
  this->accepted = true;
}

void
ConfigDialog::guessParamsFromFileName(void)
{
  QFileInfo fi(QString::fromStdString(this->profile.getPath()));
  std::string baseName = fi.baseName().toStdString();
  SUFREQ fc;
  unsigned int fs;
  unsigned int date, time;
  bool haveFc = false;
  bool haveFs = false;

  if (sscanf(
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
    haveFc = true;
    haveFs = true;
  } else if (sscanf(
        baseName.c_str(),
        "SDRSharp_%08d_%06dZ_%lg_IQ",
        &date,
        &time,
        &fc) == 3) {
    haveFc = true;
  }

  if (haveFs)
    this->profile.setSampleRate(fs);

  if (haveFc)
    this->profile.setFreq(fc);

  if (haveFs || haveFc)
    this->refreshUi();
}

void
ConfigDialog::onBrowseCaptureFile(void)
{
  QString format;
  QString title;

  switch (this->profile.getFormat()) {
    case SUSCAN_SOURCE_FORMAT_AUTO:
      title = "Open capture file";
      format = "I/Q files (*.raw);;WAV files (*.wav);;All files (*)";
      break;

    case SUSCAN_SOURCE_FORMAT_RAW:
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
         QString(),
         format);


  if (!path.isEmpty()) {
    this->ui->pathEdit->setText(path);
    this->profile.setPath(path.toStdString());
    this->guessParamsFromFileName();
  }
}

void
ConfigDialog::onSaveProfile(void)
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
ConfigDialog::onIncFreqUnits(void)
{
  incSpinUnits(this->ui->frequencySpin, "Hz");
  this->refreshUnitButtons();
}

void
ConfigDialog::onDecFreqUnits(void)
{
  decSpinUnits(this->ui->frequencySpin, "Hz");
  this->refreshUnitButtons();
}


void
ConfigDialog::onIncLNBUnits(void)
{
  incSpinUnits(this->ui->lnbSpin, "Hz");
  this->refreshUnitButtons();
}

void
ConfigDialog::onDecLNBUnits(void)
{
  decSpinUnits(this->ui->lnbSpin, "Hz");
  this->refreshUnitButtons();
}


void
ConfigDialog::onIncSampRateUnits(void)
{
  incSpinUnits(this->ui->sampleRateSpin, "sps");
  this->refreshUnitButtons();
}

void
ConfigDialog::onDecSampRateUnits(void)
{
  decSpinUnits(this->ui->sampleRateSpin, "sps");
  this->refreshUnitButtons();
}

