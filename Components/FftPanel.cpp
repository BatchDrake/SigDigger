//
//    FftPanel.cpp: Dockable FFT panel
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
#include <Suscan/Library.h>
#include "DefaultGradient.h"
#include "FftPanel.h"
#include "ui_FftPanel.h"

using namespace SigDigger;

///////////////////////////// Fft panel cnfig ////////////////////////////
#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

void
FftPanelConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(averaging);
  LOAD(panWfRatio);
  LOAD(peakDetect);
  LOAD(peakHold);
  LOAD(panRangeMin);
  LOAD(panRangeMax);
  LOAD(wfRangeMin);
  LOAD(wfRangeMax);
  LOAD(palette);
  LOAD(zoom);
  LOAD(rangeLock);
  LOAD(timeSpan);
}

Suscan::Object &&
FftPanelConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("FftPanelConfig");

  STORE(averaging);
  STORE(panWfRatio);
  STORE(peakDetect);
  STORE(peakHold);
  STORE(panRangeMin);
  STORE(panRangeMax);
  STORE(wfRangeMin);
  STORE(wfRangeMax);
  STORE(palette);
  STORE(zoom);
  STORE(rangeLock);
  STORE(timeSpan);

  return this->persist(obj);
}

///////////////////////////// Fft Panel Config /////////////////////////////////
Suscan::Serializable *
FftPanel::allocConfig(void)
{
  return this->panelConfig = new FftPanelConfig();
}


void
FftPanel::applyConfig(void)
{
  FftPanelConfig savedConfig = *this->panelConfig;

  this->setAveraging(savedConfig.averaging);
  this->setPanWfRatio(savedConfig.panWfRatio);
  this->setPandRangeMax(savedConfig.panRangeMax);
  this->setPandRangeMin(savedConfig.panRangeMin);
  this->setWfRangeMax(savedConfig.wfRangeMax);
  this->setWfRangeMin(savedConfig.wfRangeMin);
  this->setPalette(savedConfig.palette);
  this->setFreqZoom(savedConfig.zoom);
  this->setPeakHold(savedConfig.peakHold);
  this->setPeakDetect(savedConfig.peakDetect);
  this->setRangeLock(savedConfig.rangeLock);
  this->setTimeSpan(savedConfig.timeSpan);
}

void
FftPanel::deserializePalettes(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  unsigned int ndx = 0;

  for (auto i = sus->getFirstPalette();
       i != sus->getLastPalette();
       i++) {
    ndx = static_cast<unsigned int>(i - sus->getFirstPalette());

    this->palettes.push_back(Palette(*i));

    this->ui->paletteCombo->insertItem(
          static_cast<int>(ndx),
          QIcon(QPixmap::fromImage(this->palettes[ndx].getThumbnail())),
          QString::fromStdString(this->palettes[ndx].getName()),
          QVariant::fromValue(ndx));

    this->palettes[ndx].getThumbnail();
  }
}


void
FftPanel::connectAll(void)
{
  connect(
        this->ui->paletteCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onPaletteChanged(int)));

  connect(
        this->ui->wfRange,
        SIGNAL(valuesChanged(int, int)),
        this,
        SLOT(onWfRangeChanged(int, int)));

  connect(
        this->ui->pandRange,
        SIGNAL(valuesChanged(int, int)),
        this,
        SLOT(onPandRangeChanged(int, int)));

  connect(
        this->ui->fftAvgSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onAveragingChanged(int)));


  connect(
        this->ui->fftAspectSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onAspectRatioChanged(int)));

  connect(
        this->ui->freqZoomSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onFreqZoomChanged(int)));

  connect(
        this->ui->fftSizeCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onFftSizeChanged(void)));

  connect(
        this->ui->rateCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onRefreshRateChanged(void)));

  connect(
        this->ui->timeSpanCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onTimeSpanChanged(void)));

  connect(
        this->ui->lockButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onRangeLockChanged(void)));

  connect(
        this->ui->detectPeakButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onPeakChanged(void)));

  connect(
        this->ui->holdPeakButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onPeakChanged(void)));

  connect(
        this->ui->windowCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onWindowFunctionChanged(void)));
}

FftPanel::FftPanel(QWidget *parent) :
  PersistentWidget(parent),
  ui(new Ui::FftPanel)
{
  unsigned int i;

  ui->setupUi(this);

  this->palettes.push_back(Palette("Suscan", wf_gradient));

  this->assertConfig();

  for (i = 9; i < 17; ++i)
    this->addFftSize(1 << i);

  // Add refresh rates
  this->addRefreshRate(1);
  this->addRefreshRate(2);
  this->addRefreshRate(5);
  this->addRefreshRate(10);
  this->addRefreshRate(25);
  this->addRefreshRate(30);
  this->addRefreshRate(50);
  this->addRefreshRate(60);

  // Add Gqrx time spans
  this->addTimeSpan(0);
  this->addTimeSpan(5 * 60);
  this->addTimeSpan(10 * 60);
  this->addTimeSpan(15 * 60);
  this->addTimeSpan(20 * 60);
  this->addTimeSpan(30 * 60);

  this->addTimeSpan(3600);
  this->addTimeSpan(2 * 3600);
  this->addTimeSpan(5 * 3600);
  this->addTimeSpan(10 * 3600);
  this->addTimeSpan(16 * 3600);
  this->addTimeSpan(24 * 3600);
  this->addTimeSpan(48 * 3600);

  this->connectAll();
}

void
FftPanel::updateFftSizes(void)
{
  int index = 0;
  this->ui->fftSizeCombo->clear();

  for (auto p = this->sizes.begin(); p != this->sizes.end(); ++p) {
    if (*p == 0) {
      QString defString = this->defaultFftSize == 0
          ? ""
          : "  (" + QString::number(this->fftSize) + ")";
      this->ui->fftSizeCombo->addItem("Default" + defString);
    } else {
      this->ui->fftSizeCombo->addItem(QString::number(*p));
    }

    if (*p == this->fftSize)
      this->ui->fftSizeCombo->setCurrentIndex(index);

    ++index;
  }
}

void
FftPanel::updateRefreshRates(void)
{
  int index = 0;
  this->ui->rateCombo->clear();

  for (auto p = this->refreshRates.begin(); p != this->refreshRates.end(); ++p) {
    if (*p == 0) {
      QString defString = this->defaultRefreshRate == 0
          ? ""
          : "  (" + QString::number(this->refreshRate) + " fps)";
      this->ui->rateCombo->addItem("Default" + defString);
    } else {
      this->ui->rateCombo->addItem(QString::number(*p) + " fps");
    }

    if (*p == this->refreshRate)
      this->ui->rateCombo->setCurrentIndex(index);

    ++index;
  }
}

static QString
secondsToString(unsigned int seconds)
{
  if (seconds < 60)
    return QString::number(seconds) + " seconds";
  if (seconds < 3600)
    return QString::number(seconds / 60) + " minutes";

  return QString::number(seconds / 3600) + " hours";
}

void
FftPanel::updateTimeSpans(void)
{
  int index = 0;
  this->ui->timeSpanCombo->clear();

  for (auto p = this->timeSpans.begin(); p != this->timeSpans.end(); ++p) {
    if (*p == 0) {
      this->ui->timeSpanCombo->addItem("Auto");
    } else {
      this->ui->timeSpanCombo->addItem(secondsToString(*p));
    }

    if (*p == this->panelConfig->timeSpan)
      this->ui->timeSpanCombo->setCurrentIndex(index);

    ++index;
  }
}

void
FftPanel::addFftSize(unsigned int size)
{
  this->sizes.push_back(size);
}

void
FftPanel::addTimeSpan(unsigned int span)
{
  this->timeSpans.push_back(span);
}

void
FftPanel::addRefreshRate(unsigned int rate)
{
  this->refreshRates.push_back(rate);
}

void
FftPanel::updateRbw(void)
{
  if (this->rate == 0 || this->fftSize == 0) {
    this->ui->rbwLabel->setText("RBW: N/A");
  } else {
    unsigned int rbw = this->rate / this->fftSize;
    this->ui->rbwLabel->setText("RBW: " + QString::number(rbw) + " Hz");
  }
}

FftPanel::~FftPanel()
{
  delete ui;
}

//////////////////////////////////// Getters //////////////////////////////////
std::string
FftPanel::getPalette(void) const
{
  unsigned int index =
      static_cast<unsigned int>(
        this->ui->paletteCombo->currentIndex());

  if (index < this->palettes.size())
    return this->palettes[index].getName();

  return "Suscan";
}

float
FftPanel::getPandRangeMin(void) const
{
  return this->ui->pandRange->minimumValue();
}

float
FftPanel::getPandRangeMax(void) const
{
  return this->ui->pandRange->maximumValue();
}

float
FftPanel::getWfRangeMin(void) const
{
  return this->ui->wfRange->minimumValue();
}

float
FftPanel::getWfRangeMax(void) const
{
  return this->ui->wfRange->maximumValue();
}

float
FftPanel::getAveraging(void) const
{
  float avg = (1000 - this->ui->fftAvgSlider->value()) / 1000.f;
  return avg;
}

float
FftPanel::getPanWfRatio(void) const
{
  return this->ui->fftAspectSlider->value() / 100.f;
}

const QColor *
FftPanel::getPaletteGradient(void) const
{
  if (this->selected != nullptr)
    return this->selected->getGradient();
  return this->palettes[0].getGradient();
}

unsigned int
FftPanel::getFreqZoom(void) const
{
  return static_cast<unsigned int>(this->ui->freqZoomSlider->value());
}

unsigned int
FftPanel::getFftSize(void) const
{
  return this->fftSize;
}

unsigned int
FftPanel::getTimeSpan(void) const
{
  return this->panelConfig->timeSpan;
}

unsigned int
FftPanel::getRefreshRate(void) const
{
  return this->refreshRate;
}

bool
FftPanel::getPeakDetect(void) const
{
  return this->ui->detectPeakButton->isChecked();
}

bool
FftPanel::getPeakHold(void) const
{
  return this->ui->holdPeakButton->isChecked();
}

bool
FftPanel::getRangeLock(void) const
{
  return this->ui->lockButton->isChecked();
}

enum Suscan::AnalyzerParams::WindowFunction
FftPanel::getWindowFunction(void) const
{
  return static_cast<enum Suscan::AnalyzerParams::WindowFunction>(
        this->ui->windowCombo->currentIndex());
}

///////////////////////////////// Setters //////////////////////////////////////
bool
FftPanel::setPalette(std::string const &str)
{
  unsigned int i;

  for (i = 0; i < this->palettes.size(); ++i) {
    if (this->palettes[i].getName().compare(str) == 0) {
      this->ui->paletteCombo->setCurrentIndex(static_cast<int>(i));
      this->selected = &this->palettes[i];
      this->panelConfig->palette = str;
      return true;
    }
  }

  return false;
}

void
FftPanel::setFreqZoom(int zoom)
{
  this->ui->freqZoomSlider->setValue(zoom);
  this->ui->freqZoomLabel->setText(
        zoom < 1 ? "<1x" : QString::number(zoom) + "x");
  this->panelConfig->zoom = zoom;
}

void
FftPanel::setPandRangeMin(float min)
{
  this->ui->pandRange->setMinimumValue(static_cast<int>(min));
  this->panelConfig->panRangeMin = min;
}

void
FftPanel::setPandRangeMax(float max)
{
  this->ui->pandRange->setMaximumValue(static_cast<int>(max));
  this->panelConfig->panRangeMax = max;
}

void
FftPanel::setWfRangeMin(float min)
{
  this->ui->wfRange->setMinimumValue(static_cast<int>(min));
  this->panelConfig->wfRangeMin = min;
}

void
FftPanel::setWfRangeMax(float max)
{
  this->ui->wfRange->setMaximumValue(static_cast<int>(max));
  this->panelConfig->wfRangeMax = max;
}

void
FftPanel::setAveraging(float avg)
{
  int val = 1000 - static_cast<int>(avg * 1000.f);

  this->ui->fftAvgSlider->setValue(val);
  this->panelConfig->averaging = avg;
}

void
FftPanel::setPanWfRatio(float ratio)
{
  this->ui->fftAspectSlider->setValue(static_cast<int>(ratio * 100.f));
  this->panelConfig->panWfRatio = ratio;
}

void
FftPanel::setDefaultFftSize(unsigned int size)
{
  this->defaultFftSize = size;
  this->updateFftSizes();
}

void
FftPanel::setDefaultRefreshRate(unsigned int rate)
{
  this->defaultRefreshRate = rate;
  this->updateFftSizes();
}

void
FftPanel::setFftSize(unsigned int size)
{
  this->fftSize = size;
  this->updateRbw();
  this->updateFftSizes();
}

void
FftPanel::setRefreshRate(unsigned int rate)
{
  this->refreshRate = rate;
  this->updateRefreshRates();
}

void
FftPanel::setTimeSpan(unsigned int span)
{
  this->panelConfig->timeSpan = span;
  this->updateTimeSpans();
}

void
FftPanel::setSampleRate(unsigned int rate)
{
  this->rate = rate;
  this->updateRbw();
}

void
FftPanel::setPeakHold(bool hold)
{
  this->ui->holdPeakButton->setChecked(hold);
  this->panelConfig->peakHold = hold;
}

void
FftPanel::setPeakDetect(bool detect)
{
  this->ui->detectPeakButton->setChecked(detect);
  this->panelConfig->peakDetect = detect;
}

void
FftPanel::setRangeLock(bool lock)
{
  if (lock) {
    this->setWfRangeMin(this->getPandRangeMin());
    this->setWfRangeMax(this->getPandRangeMax());
  }

  this->ui->lockButton->setChecked(lock);
  this->panelConfig->rangeLock = lock;
}

void
FftPanel::setWindowFunction(enum Suscan::AnalyzerParams::WindowFunction func)
{
  // I'm sorry.
  this->ui->windowCombo->setCurrentIndex(static_cast<int>(func));
}

///////////////////////////////// Slots ///////////////////////////////////////
void
FftPanel::onPandRangeChanged(int min, int max)
{
  this->setPandRangeMin(min);
  this->setPandRangeMax(max);

  if (this->getRangeLock()) {
    this->setWfRangeMin(min);
    this->setWfRangeMax(max);
  }

  emit rangesChanged();
}

void
FftPanel::onWfRangeChanged(int min, int max)
{
  this->setWfRangeMin(min);
  this->setWfRangeMax(max);

  if (this->getRangeLock()) {
    this->setPandRangeMin(min);
    this->setPandRangeMax(max);
  }

  emit rangesChanged();
}

void
FftPanel::onAveragingChanged(int)
{
  this->setAveraging(this->getAveraging());
  emit averagerChanged();
}

void
FftPanel::onAspectRatioChanged(int)
{
  this->setPanWfRatio(this->getPanWfRatio());
  emit rangesChanged();
}

void
FftPanel::onFreqZoomChanged(int)
{
  this->setFreqZoom(this->getFreqZoom());
  emit rangesChanged();
}

void
FftPanel::onPaletteChanged(int index)
{
  this->selected = &this->palettes[static_cast<unsigned>(index)];
  this->panelConfig->palette = this->selected->getName();
  emit paletteChanged();
}

void
FftPanel::onFftSizeChanged(void)
{
  this->fftSize =
      this->sizes[
        static_cast<unsigned>(this->ui->fftSizeCombo->currentIndex())];

  if (this->fftSize == 0)
    this->fftSize = this->defaultFftSize;

  this->updateRbw();

  emit fftSizeChanged();
}

void
FftPanel::onRefreshRateChanged(void)
{
  this->refreshRate =
      this->refreshRates[
        static_cast<unsigned>(this->ui->rateCombo->currentIndex())];

  if (this->refreshRate == 0)
    this->refreshRate = this->defaultRefreshRate;

  emit refreshRateChanged();
}

void
FftPanel::onTimeSpanChanged(void)
{
  this->panelConfig->timeSpan =
      this->timeSpans[
        static_cast<unsigned>(this->ui->timeSpanCombo->currentIndex())];

  emit timeSpanChanged();
}

void
FftPanel::onRangeLockChanged(void)
{
  this->setRangeLock(this->getRangeLock());
}

void
FftPanel::onPeakChanged(void)
{
  this->setPeakHold(this->getPeakHold());
  this->setPeakDetect(this->getPeakDetect());

  emit rangesChanged();
}

void
FftPanel::onWindowFunctionChanged(void)
{
  emit windowFunctionChanged();
}
