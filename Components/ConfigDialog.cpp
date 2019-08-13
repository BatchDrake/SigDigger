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

  for (auto i = sus->getFirstProfile(); i != sus->getLastProfile(); ++i)
      this->ui->profileCombo->addItem(
            QString::fromStdString(i->label()),
            QVariant::fromValue(*i));

  for (auto i = sus->getFirstDevice(); i != sus->getLastDevice(); ++i)
    this->ui->deviceCombo->addItem(
        QString::fromStdString(i->getDesc()));
}

void
ConfigDialog::refreshUiState(void)
{
  if (this->ui->sdrRadio->isChecked()) {
    this->ui->sdrFrame->setEnabled(true);
    this->ui->fileFrame->setEnabled(false);
  } else {
    this->ui->sdrFrame->setEnabled(false);
    this->ui->fileFrame->setEnabled(true);
  }
}

void
ConfigDialog::refreshAntennas(void)
{
  populateAntennaCombo(this->profile, this->ui->antennaCombo);
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

void
ConfigDialog::refreshProfileUi(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  this->ui->frequencyLine->setText(
        QString::number(
          static_cast<uint64_t>(this->profile.getFreq())));

  this->ui->sampleRateLine->setText(
        QString::number(
          static_cast<uint64_t>(this->profile.getSampleRate())));

  switch (this->profile.getType()) {
    case SUSCAN_SOURCE_TYPE_SDR:
      this->ui->sdrRadio->setChecked(true);
      break;

    case SUSCAN_SOURCE_TYPE_FILE:
      this->ui->fileRadio->setChecked(true);
      break;
  }

  this->ui->iqBalanceCheck->setChecked(this->profile.getIQBalance());
  this->ui->removeDCCheck->setChecked(this->profile.getDCRemove());
  this->ui->loopCheck->setChecked(this->profile.getLoop());
  this->ui->bwSpin->setValue(
        static_cast<double>(this->profile.getBandwidth()));

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

  // Selecting the source is a bit trickier:
  // We traverse all devices and get their index.
  // Then, we use that information to set the current
  // element in the combobox

  this->ui->deviceCombo->setCurrentIndex(-1);

  for (auto i = sus->getFirstDevice(); i != sus->getLastDevice(); ++i) {
    if (i->equals(this->profile.getDevice())) {
      this->ui->deviceCombo->setCurrentIndex(
            static_cast<int>(i - sus->getFirstDevice()));
    }
  }

  this->refreshUiState();
  this->refreshAntennas();
}

void
ConfigDialog::refreshUi(void)
{
  this->refreshing = true;

  this->refreshColorUi();
  this->refreshProfileUi();

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
        this->ui->frequencyLine,
        SIGNAL(textEdited(const QString &)),
        this,
        SLOT(onLineEditsChanged(const QString &)));

  connect(
        this->ui->sampleRateLine,
        SIGNAL(textEdited(const QString &)),
        this,
        SLOT(onLineEditsChanged(const QString &)));

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
        SIGNAL(accepted()),
        this,
        SLOT(onAccepted()));

  connect(
        this->ui->browseButton,
        SIGNAL(clicked()),
        this,
        SLOT(onBrowseCaptureFile()));
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

  // Setup integer validators
  this->ui->frequencyLine->setValidator(new QDoubleValidator(0.0, 15e6, 0, this));
  this->ui->sampleRateLine->setValidator(new QIntValidator(1, 64000000, this));
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

    SU_ATTEMPT(device = sus->getDeviceAt(
          static_cast<unsigned int>(index)));

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

void
ConfigDialog::onLineEditsChanged(const QString &)
{
  SUFREQ freq;
  unsigned int sampRate;

  if (!this->refreshing) {
    if (sscanf(
          this->ui->frequencyLine->text().toStdString().c_str(),
          "%lg",
          &freq) < 1) {
      this->ui->frequencyLine->setStyleSheet("color: red");
    } else {
      this->ui->frequencyLine->setStyleSheet("");
      this->profile.setFreq(freq);
    }

    if (sscanf(
          this->ui->sampleRateLine->text().toStdString().c_str(),
          "%u",
          &sampRate) < 1) {
      this->ui->sampleRateLine->setStyleSheet("color: red");
    } else {
      this->ui->sampleRateLine->setStyleSheet("");
      this->profile.setSampleRate(sampRate);
    }
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
ConfigDialog::onBandwidthChanged(double value)
{
  if (!this->refreshing)
    this->profile.setBandwidth(static_cast<SUFLOAT>(value));
}

void
ConfigDialog::onAccepted(void)
{
  this->saveColors();
  this->saveAnalyzerParams();
  this->accepted = true;
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
  }
}
