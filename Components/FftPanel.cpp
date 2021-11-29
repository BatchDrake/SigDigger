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
#include "FftPanel.h"
#include "ui_FftPanel.h"
#include "SigDiggerHelpers.h"
#include <SuWidgetsHelpers.h>

using namespace SigDigger;

///////////////////////////// Fft panel cnfig ////////////////////////////
#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

void
FftPanelConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(collapsed);
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
  LOAD(timeStamps);
  LOAD(bookmarks);
  LOAD(unitName);
  LOAD(zeroPoint);
  LOAD(gain);
}

Suscan::Object &&
FftPanelConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("FftPanelConfig");

  STORE(collapsed);
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
  STORE(timeStamps);
  STORE(bookmarks);
  STORE(unitName);
  STORE(zeroPoint);
  STORE(gain);

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
  this->setTimeStamps(savedConfig.timeStamps);
  this->setBookmarks(savedConfig.bookmarks);
  this->setUnitName(QString::fromStdString(savedConfig.unitName));
  this->setZeroPoint(savedConfig.zeroPoint);
  this->setGain(savedConfig.gain);
  this->setProperty("collapsed", savedConfig.collapsed);
}

bool
FftPanel::event(QEvent *event)
{
  if (event->type() == QEvent::DynamicPropertyChange) {
    QDynamicPropertyChangeEvent *const propEvent =
        static_cast<QDynamicPropertyChangeEvent*>(event);
    QString propName = propEvent->propertyName();
    if (propName == "collapsed")
      this->panelConfig->collapsed = this->property("collapsed").value<bool>();
  }

  return PersistentWidget::event(event);
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
        this->ui->timeStampsButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onTimeStampsChanged(void)));

  connect(
        this->ui->windowCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onWindowFunctionChanged(void)));

  connect(
        this->ui->bookmarksButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onBookmarksChanged(void)));

  connect(
        this->ui->unitsCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onUnitChanged(void)));

  connect(
        this->ui->zeroPointSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onZeroPointChanged(void)));

  connect(
        this->ui->gainSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onGainChanged(void)));
}

FftPanel::FftPanel(QWidget *parent) :
  PersistentWidget(parent),
  ui(new Ui::FftPanel)
{
  unsigned int i;

  ui->setupUi(this);

  this->assertConfig();

  for (i = 9; i < 21; ++i)
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

  this->populateUnits();

  this->connectAll();

  this->setProperty("collapsed", this->panelConfig->collapsed);
}

void
FftPanel::populateUnits(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  this->ui->unitsCombo->clear();

  for (auto p: sus->getSpectrumUnitMap())
    this->ui->unitsCombo->addItem(QString::fromStdString(p.name));

  this->ui->unitsCombo->setCurrentIndex(0);
  this->ui->zeroPointSpin->setValue(0.0);
}

void
FftPanel::refreshPalettes(void)
{
  SigDiggerHelpers::instance()->populatePaletteCombo(this->ui->paletteCombo);
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
  int selectedIndex = 0;
  unsigned int diff;
  unsigned int bestMatch = 1 << 20; /* Just a big number */
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

    diff = static_cast<unsigned>(
          std::abs(static_cast<int>(*p) - static_cast<int>(this->refreshRate)));
    if (diff < bestMatch) {
      selectedIndex = index;
      bestMatch = diff;
    }

    ++index;
  }

  this->ui->rateCombo->setCurrentIndex(selectedIndex);
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
    qreal rbw = static_cast<qreal>(this->rate) / this->fftSize;
    QString rbwString = SuWidgetsHelpers::formatQuantity(
          rbw,
          2,
          QStringLiteral("Hz"));

    this->ui->rbwLabel->setText("RBW: " + rbwString);
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
  if (this->selected != nullptr)
    return this->selected->getName();

  return SigDiggerHelpers::instance()->getPalette(0)->getName();
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

  return SigDiggerHelpers::instance()->getPalette(0)->getGradient();
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

bool
FftPanel::getTimeStamps(void) const
{
  return this->ui->timeStampsButton->isChecked();
}

bool
FftPanel::getBookmarks(void) const
{
  return this->ui->bookmarksButton->isChecked();
}

enum Suscan::AnalyzerParams::WindowFunction
FftPanel::getWindowFunction(void) const
{
  return static_cast<enum Suscan::AnalyzerParams::WindowFunction>(
        this->ui->windowCombo->currentIndex());
}

QString
FftPanel::getUnitName(void) const
{
  return this->ui->unitsCombo->currentText();
}

float
FftPanel::getZeroPoint(void) const
{
  return static_cast<float>(this->ui->zeroPointSpin->value());
}

float
FftPanel::getGain(void) const
{
  return static_cast<float>(this->ui->gainSpinBox->value());
}

float
FftPanel::getCompleteZeroPoint(void) const
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  float currZP = static_cast<float>(this->ui->zeroPointSpin->value());

  std::string name = this->ui->unitsCombo->currentText().toStdString();
  auto it = sus->getSpectrumUnitFrom(name);

  if (it != sus->getLastSpectrumUnit())
    return it->zeroPoint + currZP;
  else
    return currZP;
}

float
FftPanel::getdBPerUnit(void) const
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  std::string name = this->ui->unitsCombo->currentText().toStdString();
  auto it = sus->getSpectrumUnitFrom(name);

  if (it != sus->getLastSpectrumUnit())
    return it->dBPerUnit;
  else
    return 1.;
}
///////////////////////////////// Setters //////////////////////////////////////
bool
FftPanel::setPalette(std::string const &str)
{
  int index = SigDiggerHelpers::instance()->getPaletteIndex(str);

  if (index < 0)
    return false;

  this->ui->paletteCombo->setCurrentIndex(index);
  this->selected = SigDiggerHelpers::instance()->getPalette(index);
  this->panelConfig->palette = str;

  return true;
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
FftPanel::setTimeStamps(bool value)
{
  this->ui->timeStampsButton->setChecked(value);
  this->panelConfig->timeStamps = value;
}

void
FftPanel::setBookmarks(bool value)
{
  this->ui->bookmarksButton->setChecked(value);
  this->panelConfig->bookmarks = value;
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

bool
FftPanel::setUnitName(QString name)
{
  int index = this->ui->unitsCombo->findText(name);

  if (index == -1)
    return false;

  this->ui->unitsCombo->setCurrentIndex(index);
  this->ui->zeroPointSpin->setSuffix(" " + name);

  this->panelConfig->unitName = name.toStdString();

  return true;
}

void
FftPanel::setZeroPoint(float zp)
{
  this->ui->zeroPointSpin->setValue(static_cast<double>(zp));
  this->panelConfig->zeroPoint = zp;
}

void
FftPanel::setGain(float gain)
{
  this->ui->gainSpinBox->setValue(static_cast<double>(gain));
  this->panelConfig->gain = gain;
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
  this->selected = SigDiggerHelpers::instance()->getPalette(index);
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

void
FftPanel::onTimeStampsChanged(void)
{
  this->setTimeStamps(this->getTimeStamps());
  emit timeStampsChanged();
}

void
FftPanel::onBookmarksChanged(void)
{
  this->setBookmarks(this->getBookmarks());
  emit bookmarksChanged();
}

void
FftPanel::onUnitChanged(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  float currZPdB = this->zeroPointToDb();
  float newZp;
  std::string name = this->ui->unitsCombo->currentText().toStdString();
  auto it = sus->getSpectrumUnitFrom(name);

  this->ui->zeroPointSpin->setSuffix(" " + QString::fromStdString(name));

  if (it != sus->getLastSpectrumUnit()) {
    this->currentUnit = *it;
  } else {
    this->currentUnit.name      = "dBFS";
    this->currentUnit.dBPerUnit = 1.f;
    this->currentUnit.zeroPoint = 0;
  }

  this->panelConfig->unitName = this->currentUnit.name;

  newZp = this->dbToZeroPoint(currZPdB);

  emit unitChanged(
        QString::fromStdString(this->currentUnit.name),
        this->currentUnit.dBPerUnit,
        this->currentUnit.zeroPoint + newZp);

  this->setZeroPoint(newZp);
}

void
FftPanel::onZeroPointChanged(void)
{
  float currZP = static_cast<float>(this->ui->zeroPointSpin->value());

  this->panelConfig->zeroPoint = currZP;
  emit zeroPointChanged(this->currentUnit.zeroPoint + currZP);
}

void
FftPanel::onGainChanged(void)
{
  this->panelConfig->gain = static_cast<float>(this->ui->gainSpinBox->value());
  emit gainChanged(static_cast<float>(this->ui->gainSpinBox->value()));
}
