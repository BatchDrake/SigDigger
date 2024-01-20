//
//    FFTWidget.cpp: description
//    Copyright (C) 2022 Gonzalo José Carracedo Carballal
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

#include "FFTWidgetFactory.h"
#include "FFTWidget.h"
#include <QVariant>
#include <QEvent>
#include <SigDiggerHelpers.h>
#include <SuWidgetsHelpers.h>
#include <UIMediator.h>
#include <MainSpectrum.h>
#include "ui_FFTWidget.h"

using namespace SigDigger;

///////////////////////////// Fft panel cnfig ////////////////////////////
#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), field)
#define LOAD(field) field = conf.get(STRINGFY(field), field)

void
FFTWidgetConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(collapsed);
  LOAD(averaging);
  LOAD(panWfRatio);
  LOAD(peakDetect);
  LOAD(peakHold);
  LOAD(filled);
  LOAD(panRangeMin);
  LOAD(panRangeMax);
  LOAD(wfRangeMin);
  LOAD(wfRangeMax);
  LOAD(palette);
  LOAD(zoom);
  LOAD(rangeLock);
  LOAD(channels);
  LOAD(timeSpan);
  LOAD(timeStamps);
  LOAD(utcTimeStamps);
  LOAD(bookmarks);
  LOAD(unitName);
  LOAD(zeroPoint);
  LOAD(gain);
}

Suscan::Object &&
FFTWidgetConfig::serialize()
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("FFTWidgetConfig");

  STORE(collapsed);
  STORE(averaging);
  STORE(panWfRatio);
  STORE(peakDetect);
  STORE(peakHold);
  STORE(filled);
  STORE(panRangeMin);
  STORE(panRangeMax);
  STORE(wfRangeMin);
  STORE(wfRangeMax);
  STORE(palette);
  STORE(zoom);
  STORE(rangeLock);
  STORE(channels);
  STORE(timeSpan);
  STORE(timeStamps);
  STORE(utcTimeStamps);
  STORE(bookmarks);
  STORE(unitName);
  STORE(zeroPoint);
  STORE(gain);

  return persist(obj);
}

///////////////////////////// Fft Panel Config /////////////////////////////////
Suscan::Serializable *
FFTWidget::allocConfig()
{
  return m_panelConfig = new FFTWidgetConfig();
}

void
FFTWidget::applyConfig()
{
  FFTWidgetConfig savedConfig = *m_panelConfig;

  // Analyzer params are kept by the UIMediator, which provides information on
  // how to start up the analyzer.
  auto params = m_mediator->getAnalyzerParams();
  refreshParamControls(*params);

  // Refreshing the current palette config involves having the palette
  // list up to date.
  refreshPalettes();

  setAveraging(savedConfig.averaging);
  setPanWfRatio(savedConfig.panWfRatio);
  setPandRangeMax(savedConfig.panRangeMax);
  setPandRangeMin(savedConfig.panRangeMin);
  setWfRangeMax(savedConfig.wfRangeMax);
  setWfRangeMin(savedConfig.wfRangeMin);
  setPalette(savedConfig.palette);
  setFreqZoom(savedConfig.zoom);
  setPeakHold(savedConfig.peakHold);
  setPeakDetect(savedConfig.peakDetect);
  setFilled(savedConfig.filled);
  setRangeLock(savedConfig.rangeLock);
  setShowChannels(savedConfig.channels);
  setTimeSpan(savedConfig.timeSpan);
  setTimeStamps(savedConfig.timeStamps);
  setTimeStampsUTC(savedConfig.utcTimeStamps);
  setBookmarks(savedConfig.bookmarks);
  setUnitName(QString::fromStdString(savedConfig.unitName));
  setZeroPoint(savedConfig.zeroPoint);
  setGain(savedConfig.gain);

  setProperty("collapsed", savedConfig.collapsed);

  // Apply all spectrum settings, all at once
  refreshSpectrumSettings();
}

bool
FFTWidget::event(QEvent *event)
{
  if (event->type() == QEvent::DynamicPropertyChange) {
    QDynamicPropertyChangeEvent *const propEvent =
        static_cast<QDynamicPropertyChangeEvent*>(event);
    QString propName = propEvent->propertyName();
    if (propName == "collapsed")
      m_panelConfig->collapsed = property("collapsed").value<bool>();
  }

  return ToolWidget::event(event);
}

void
FFTWidget::setState(int, Suscan::Analyzer *analyzer)
{
  if (analyzer != m_analyzer) {
    m_analyzer = analyzer;

    if (m_analyzer != nullptr) {
      connect(
            m_analyzer,
            SIGNAL(analyzer_params(const Suscan::AnalyzerParams &)),
            this,
            SLOT(onAnalyzerParams(const Suscan::AnalyzerParams &)));
      connect(
            m_analyzer,
            SIGNAL(source_info_message(const Suscan::SourceInfoMessage &)),
            this,
            SLOT(onSourceInfoMessage(const Suscan::SourceInfoMessage &)));
    }
  }
}

void
FFTWidget::setProfile(Suscan::Source::Config &config)
{
  m_rate = config.getDecimatedSampleRate();
  updateRbw();
}

void
FFTWidget::connectAll()
{
  connect(
        m_ui->paletteCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onPaletteChanged(int)));

  connect(
        m_ui->wfRange,
        SIGNAL(valuesChanged(int, int)),
        this,
        SLOT(onWfRangeChanged(int, int)));

  connect(
        m_ui->pandRange,
        SIGNAL(valuesChanged(int, int)),
        this,
        SLOT(onPandRangeChanged(int, int)));

  connect(
        m_ui->avgSpinBox,
        SIGNAL(valueChanged(qreal)),
        this,
        SLOT(onAveragingChanged(qreal)));

  connect(
        m_ui->fftAspectSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onAspectRatioChanged(int)));

  connect(
        m_ui->freqZoomSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onFreqZoomChanged(int)));

  connect(
        m_ui->fftSizeCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onFftSizeChanged()));

  connect(
        m_ui->rateCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onRefreshRateChanged()));

  connect(
        m_ui->timeSpanCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onTimeSpanChanged()));

  connect(
        m_ui->lockButton,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onRangeLockChanged()));

  connect(
        m_ui->channelButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onChannelsChanged()));

  connect(
        m_ui->detectPeakButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onPeakChanged()));

  connect(
        m_ui->holdPeakButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onPeakChanged()));

  connect(
        m_ui->filledButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onFilledChanged()));

  connect(
        m_ui->timeStampsButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onTimeStampsChanged()));

  connect(
        m_ui->windowCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onWindowFunctionChanged()));

  connect(
        m_ui->bookmarksButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onBookmarksChanged()));

  connect(
        m_ui->unitsCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onUnitChanged()));

  connect(
        m_ui->zeroPointSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onZeroPointChanged()));

  connect(
        m_ui->gainSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onGainChanged()));

  connect(
        m_spectrum,
        SIGNAL(rangeChanged(float, float)),
        this,
        SLOT(onRangeChanged(float, float)));

  connect(
        m_spectrum,
        SIGNAL(zoomChanged(float)),
        this,
        SLOT(onZoomChanged(float)));

  connect(
        m_ui->timeZoneCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onUTCChanged()));
}

FFTWidget::FFTWidget(FFTWidgetFactory *factory, UIMediator *mediator, QWidget *parent) :
  ToolWidget(factory, mediator, parent),
  m_ui(new Ui::FftPanel)
{
  unsigned int i;

  m_ui->setupUi(this);

  m_mediator = mediator;
  m_spectrum = mediator->getMainSpectrum();

  assertConfig();

  for (i = 9; i < 21; ++i)
    addFftSize(1 << i);

  // Add refresh rates
  addRefreshRate(1);
  addRefreshRate(2);
  addRefreshRate(5);
  addRefreshRate(10);
  addRefreshRate(25);
  addRefreshRate(30);
  addRefreshRate(50);
  addRefreshRate(60);
  addRefreshRate(90);
  addRefreshRate(120);

  // Add Gqrx time spans
  addTimeSpan(0);
  addTimeSpan(15);
  addTimeSpan(30);
  addTimeSpan(1 * 60);
  addTimeSpan(2 * 60);
  addTimeSpan(5 * 60);
  addTimeSpan(10 * 60);
  addTimeSpan(15 * 60);
  addTimeSpan(20 * 60);
  addTimeSpan(30 * 60);

  addTimeSpan(3600);
  addTimeSpan(2 * 3600);
  addTimeSpan(5 * 3600);
  addTimeSpan(10 * 3600);
  addTimeSpan(16 * 3600);
  addTimeSpan(24 * 3600);
  addTimeSpan(48 * 3600);

  populateUnits();

  connectAll();

  setProperty("collapsed", m_panelConfig->collapsed);
}

void
FFTWidget::populateUnits()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  m_ui->unitsCombo->clear();

  for (auto p: sus->getSpectrumUnitMap())
    m_ui->unitsCombo->addItem(QString::fromStdString(p.name));

  m_ui->unitsCombo->setCurrentIndex(0);
  m_ui->zeroPointSpin->setValue(0.0);
}

void
FFTWidget::refreshPalettes()
{
  SigDiggerHelpers::instance()->populatePaletteCombo(m_ui->paletteCombo);
}

void
FFTWidget::updateFftSizes()
{
  int index = 0;
  m_ui->fftSizeCombo->clear();

  for (auto p = m_sizes.begin(); p != m_sizes.end(); ++p) {
    if (*p == 0) {
      QString defString = m_defaultFftSize == 0
          ? ""
          : "  (" + QString::number(m_fftSize) + ")";
      m_ui->fftSizeCombo->addItem("Default" + defString);
    } else {
      m_ui->fftSizeCombo->addItem(QString::number(*p));
    }

    if (*p == m_fftSize)
      m_ui->fftSizeCombo->setCurrentIndex(index);

    ++index;
  }
}

void
FFTWidget::updateRefreshRates()
{
  int index = 0;
  int selectedIndex = 0;
  unsigned int diff;
  unsigned int bestMatch = 1 << 20; /* Just a big number */
  m_ui->rateCombo->clear();

  for (auto p = m_refreshRates.begin(); p != m_refreshRates.end(); ++p) {
    if (*p == 0) {
      QString defString = m_defaultRefreshRate == 0
          ? ""
          : "  (" + QString::number(m_refreshRate) + " fps)";
      m_ui->rateCombo->addItem("Default" + defString);
    } else {
      m_ui->rateCombo->addItem(QString::number(*p) + " fps");
    }

    diff = SCAST(unsigned,
          std::abs(SCAST(int, *p) - SCAST(int, m_refreshRate)));
    if (diff < bestMatch) {
      selectedIndex = index;
      bestMatch = diff;
    }

    ++index;
  }

  m_ui->rateCombo->setCurrentIndex(selectedIndex);
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
FFTWidget::updateTimeSpans()
{
  int index = 0;
  m_ui->timeSpanCombo->clear();

  for (auto p = m_timeSpans.begin(); p != m_timeSpans.end(); ++p) {
    if (*p == 0) {
      m_ui->timeSpanCombo->addItem("Auto");
    } else {
      m_ui->timeSpanCombo->addItem(secondsToString(*p));
    }

    if (*p == m_panelConfig->timeSpan)
      m_ui->timeSpanCombo->setCurrentIndex(index);

    ++index;
  }
}

void
FFTWidget::addFftSize(unsigned int size)
{
  m_sizes.push_back(size);
}

void
FFTWidget::addTimeSpan(unsigned int span)
{
  m_timeSpans.push_back(span);
}

void
FFTWidget::addRefreshRate(unsigned int rate)
{
  m_refreshRates.push_back(rate);
}

void
FFTWidget::updateRbw()
{
  if (m_rate == 0 || m_fftSize == 0) {
    m_ui->rbwLabel->setText("RBW: N/A");
  } else {
    qreal rbw = static_cast<qreal>(m_rate) / m_fftSize;
    QString rbwString = SuWidgetsHelpers::formatQuantity(
          rbw,
          2,
          QStringLiteral("Hz"));

    m_ui->rbwLabel->setText("RBW: " + rbwString);
  }
}

FFTWidget::~FFTWidget()
{
  delete m_ui;
}

//////////////////////////////////// Getters //////////////////////////////////
std::string
FFTWidget::getPalette() const
{
  if (m_selected != nullptr)
    return m_selected->getName();

  return SigDiggerHelpers::instance()->getPalette(0)->getName();
}

float
FFTWidget::getPandRangeMin() const
{
  return m_ui->pandRange->minimumValue();
}

float
FFTWidget::getPandRangeMax() const
{
  return m_ui->pandRange->maximumValue();
}

float
FFTWidget::getWfRangeMin() const
{
  return m_ui->wfRange->minimumValue();
}

float
FFTWidget::getWfRangeMax() const
{
  return m_ui->wfRange->maximumValue();
}

float
FFTWidget::getAveraging() const
{
  return m_ui->avgSpinBox->value();
}

float
FFTWidget::getPanWfRatio() const
{
  return m_ui->fftAspectSlider->value() / 100.f;
}

const QColor *
FFTWidget::getPaletteGradient() const
{
  if (m_selected != nullptr)
    return m_selected->getGradient();

  return SigDiggerHelpers::instance()->getPalette(0)->getGradient();
}

float
FFTWidget::getFreqZoom() const
{
  return m_panelConfig->zoom;
}

unsigned int
FFTWidget::getFftSize() const
{
  return m_fftSize;
}

unsigned int
FFTWidget::getTimeSpan() const
{
  return m_panelConfig->timeSpan;
}

unsigned int
FFTWidget::getRefreshRate() const
{
  return m_refreshRate;
}

bool
FFTWidget::getPeakDetect() const
{
  return m_ui->detectPeakButton->isChecked();
}

bool
FFTWidget::getPeakHold() const
{
  return m_ui->holdPeakButton->isChecked();
}

bool
FFTWidget::getRangeLock() const
{
  return m_ui->lockButton->isChecked();
}

bool
FFTWidget::getShowChannels() const
{
  return m_ui->channelButton->isChecked();
}

bool
FFTWidget::getTimeStamps() const
{
  return m_ui->timeStampsButton->isChecked();
}

bool
FFTWidget::getBookmarks() const
{
  return m_ui->bookmarksButton->isChecked();
}

bool
FFTWidget::getFilled() const
{
  return m_ui->filledButton->isChecked();
}

enum Suscan::AnalyzerParams::WindowFunction
FFTWidget::getWindowFunction() const
{
  return static_cast<enum Suscan::AnalyzerParams::WindowFunction>(
        m_ui->windowCombo->currentIndex());
}

void
FFTWidget::applySourceInfo(Suscan::AnalyzerSourceInfo const &info)
{
  m_ui->fftSizeCombo->setEnabled(
        info.testPermission(SUSCAN_ANALYZER_PERM_SET_FFT_SIZE));

  m_ui->rateCombo->setEnabled(
        info.testPermission(SUSCAN_ANALYZER_PERM_SET_FFT_FPS));

  m_ui->windowCombo->setEnabled(
        info.testPermission(SUSCAN_ANALYZER_PERM_SET_FFT_WINDOW));
}

QString
FFTWidget::getUnitName() const
{
  return m_ui->unitsCombo->currentText();
}

float
FFTWidget::getZeroPoint() const
{
  return SCAST(float, m_ui->zeroPointSpin->value());
}

float
FFTWidget::getGain() const
{
  return SCAST(float, m_ui->gainSpinBox->value());
}

float
FFTWidget::getCompleteZeroPoint() const
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  float currZP = SCAST(float, m_ui->zeroPointSpin->value());

  std::string name = m_ui->unitsCombo->currentText().toStdString();
  auto it = sus->getSpectrumUnitFrom(name);

  if (it != sus->getLastSpectrumUnit())
    return it->zeroPoint + currZP;
  else
    return currZP;
}

float
FFTWidget::getdBPerUnit() const
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  std::string name = m_ui->unitsCombo->currentText().toStdString();
  auto it = sus->getSpectrumUnitFrom(name);

  if (it != sus->getLastSpectrumUnit())
    return it->dBPerUnit;
  else
    return 1.;
}
///////////////////////////////// Setters //////////////////////////////////////
bool
FFTWidget::setPalette(std::string const &str)
{
  int index = SigDiggerHelpers::instance()->getPaletteIndex(str);

  if (index < 0)
    return false;

  m_ui->paletteCombo->setCurrentIndex(index);
  m_selected = SigDiggerHelpers::instance()->getPalette(index);
  m_panelConfig->palette = str;

  return true;
}

void
FFTWidget::setFreqZoom(float zoom)
{
  float logZoom = SU_LOG(zoom) / SU_LOG(2);
  int sliderPos = SCAST(int, 100 * logZoom);

  m_ui->freqZoomSlider->setValue(sliderPos);
  if (zoom > 0.5f && zoom < 2) {
    m_ui->freqZoomLabel->setText(
          zoom <= 0.5f
          ? "/" + QString::number(SCAST(qreal, 1 / zoom), 'g', 2)
          : QString::number(SCAST(qreal, zoom), 'g', 2) + "x");
  } else {
    m_ui->freqZoomLabel->setText(
          zoom <= 0.5f
          ? "/" + QString::number(SCAST(int, 1 / zoom))
          : QString::number(SCAST(int, zoom)) + "x");
  }

  m_panelConfig->zoom = zoom;
}

void
FFTWidget::setPandRangeMin(float min)
{
  m_ui->pandRange->setMinimumValue(SCAST(int, min));
  m_panelConfig->panRangeMin = min;
}

void
FFTWidget::setPandRangeMax(float max)
{
  m_ui->pandRange->setMaximumValue(SCAST(int, max));
  m_panelConfig->panRangeMax = max;
}

void
FFTWidget::setWfRangeMin(float min)
{
  m_ui->wfRange->setMinimumValue(SCAST(int, min));
  m_panelConfig->wfRangeMin = min;
}

void
FFTWidget::setWfRangeMax(float max)
{
  m_ui->wfRange->setMaximumValue(SCAST(int, max));
  m_panelConfig->wfRangeMax = max;
}

void
FFTWidget::setAveraging(float avg)
{
  m_ui->avgSpinBox->setValue(avg);
  m_panelConfig->averaging = avg;
}

void
FFTWidget::setPanWfRatio(float ratio)
{
  m_ui->fftAspectSlider->setValue(SCAST(int, ratio * 100.f));
  m_panelConfig->panWfRatio = ratio;
}

void
FFTWidget::setDefaultFftSize(unsigned int size)
{
  m_defaultFftSize = size;
  updateFftSizes();
}

void
FFTWidget::setDefaultRefreshRate(unsigned int rate)
{
  m_defaultRefreshRate = rate;
  updateFftSizes();
}

void
FFTWidget::setFftSize(unsigned int size)
{
  m_fftSize = size;
  updateRbw();
  updateFftSizes();
}

void
FFTWidget::setRefreshRate(unsigned int rate)
{
  m_refreshRate = rate;
  updateRefreshRates();
}

void
FFTWidget::setTimeSpan(unsigned int span)
{
  m_panelConfig->timeSpan = span;
  updateTimeSpans();
}

void
FFTWidget::setSampleRate(unsigned int rate)
{
  m_rate = rate;
  updateRbw();
}

void
FFTWidget::setPeakHold(bool hold)
{
  m_ui->holdPeakButton->setChecked(hold);
  m_panelConfig->peakHold = hold;
}

void
FFTWidget::setFilled(bool filled)
{
  m_ui->filledButton->setChecked(filled);
  m_panelConfig->filled = filled;
}

void
FFTWidget::setPeakDetect(bool detect)
{
  m_ui->detectPeakButton->setChecked(detect);
  m_panelConfig->peakDetect = detect;
}

void
FFTWidget::setTimeStamps(bool value)
{
  m_ui->timeStampsButton->setChecked(value);
  m_panelConfig->timeStamps = value;
}

void
FFTWidget::setTimeStampsUTC(bool utc)
{
  m_ui->timeZoneCombo->setCurrentIndex(utc ? 1 : 0);
  m_panelConfig->utcTimeStamps = utc;
}

void
FFTWidget::setBookmarks(bool value)
{
  m_ui->bookmarksButton->setChecked(value);
  m_panelConfig->bookmarks = value;
}

void
FFTWidget::setRangeLock(bool lock)
{
  if (lock) {
    setWfRangeMin(getPandRangeMin());
    setWfRangeMax(getPandRangeMax());
  }

  m_ui->lockButton->setChecked(lock);
  m_panelConfig->rangeLock = lock;
}

void
FFTWidget::setShowChannels(bool show)
{
  m_ui->channelButton->setChecked(show);
  m_panelConfig->channels = show;
}


bool
FFTWidget::setUnitName(QString name)
{
  int index = m_ui->unitsCombo->findText(name);

  if (index == -1)
    return false;

  m_ui->unitsCombo->setCurrentIndex(index);
  m_ui->zeroPointSpin->setSuffix(" " + name);

  m_panelConfig->unitName = name.toStdString();

  return true;
}

void
FFTWidget::setZeroPoint(float zp)
{
  m_ui->zeroPointSpin->setValue(static_cast<double>(zp));
  m_panelConfig->zeroPoint = zp;
}

void
FFTWidget::setGain(float gain)
{
  m_ui->gainSpinBox->setValue(static_cast<double>(gain));
  m_panelConfig->gain = gain;
}

void
FFTWidget::setWindowFunction(enum Suscan::AnalyzerParams::WindowFunction func)
{
  // I'm sorry.
  m_ui->windowCombo->setCurrentIndex(SCAST(int, func));
}

void
FFTWidget::refreshSpectrumScaleSettings()
{
  bool blocking = m_spectrum->blockSignals(true);

  m_spectrum->setPandapterRange(
        getPandRangeMin(),
        getPandRangeMax());

  m_spectrum->setWfRange(
        getWfRangeMin(),
        getWfRangeMax());

  m_spectrum->setPanWfRatio(getPanWfRatio());
  m_spectrum->setZoom(getFreqZoom());
  m_spectrum->setTimeSpan(getTimeSpan());

  m_spectrum->blockSignals(blocking);
}

void
FFTWidget::refreshSpectrumAxesSettings()
{
  bool blocking = m_spectrum->blockSignals(true);

  m_spectrum->setGain(m_panelConfig->gain);

  m_spectrum->blockSignals(blocking);
}

void
FFTWidget::refreshSpectrumRepresentationSettings()
{
  bool blocking = m_spectrum->blockSignals(true);

  m_spectrum->setPeakDetect(getPeakDetect());
  m_spectrum->setPeakHold(getPeakHold());
  m_spectrum->setFilled(getFilled());

  m_spectrum->setBookmarks(getBookmarks());
  m_spectrum->setShowChannels(getShowChannels());
  m_spectrum->blockSignals(blocking);
}

void
FFTWidget::refreshSpectrumWaterfallSettings()
{
  bool blocking = m_spectrum->blockSignals(true);

  m_spectrum->setTimeStamps(getTimeStamps());
  m_spectrum->setPaletteGradient(getPaletteGradient());

  m_spectrum->blockSignals(blocking);
}

void
FFTWidget::refreshSpectrumUnits()
{
  m_spectrum->setUnits(
        getUnitName(),
        getdBPerUnit(),
        getCompleteZeroPoint());
}

void
FFTWidget::refreshSpectrumSettings()
{
  refreshSpectrumScaleSettings();
  refreshSpectrumAxesSettings();
  refreshSpectrumRepresentationSettings();
  refreshSpectrumUnits();
  refreshSpectrumWaterfallSettings();
}

void
FFTWidget::updateAnalyzerParams()
{
  if (m_analyzer != nullptr) {
    auto params = m_mediator->getAnalyzerParams();
    m_analyzer->setParams(*params);
  }
}

void
FFTWidget::refreshParamControls(Suscan::AnalyzerParams const &params)
{
  bool old = blockSignals(true);

  setWindowFunction(params.windowFunction);
  setFftSize(params.windowSize);
  setRefreshRate(SCAST(unsigned, 1.f / params.psdUpdateInterval));
  blockSignals(old);
}

///////////////////////////////// Slots ///////////////////////////////////////
void
FFTWidget::onPandRangeChanged(int min, int max)
{
  setPandRangeMin(min);
  setPandRangeMax(max);

  if (getRangeLock()) {
    setWfRangeMin(min);
    setWfRangeMax(max);
  }

  refreshSpectrumScaleSettings();
}

void
FFTWidget::onWfRangeChanged(int min, int max)
{
  setWfRangeMin(min);
  setWfRangeMax(max);

  if (getRangeLock()) {
    setPandRangeMin(min);
    setPandRangeMax(max);
  }

  refreshSpectrumScaleSettings();
}

void
FFTWidget::onAveragingChanged(qreal)
{
  float avg = getAveraging();

  auto averager = m_mediator->getSpectrumAverager();

  setAveraging(avg);

  if (avg <= 1.f)
    averager->setAlpha(1.);
  else
    averager->setAlpha(SU_SPLPF_ALPHA(avg - 1));
}

void
FFTWidget::onAspectRatioChanged(int)
{
  setPanWfRatio(getPanWfRatio());

  refreshSpectrumScaleSettings();
}

void
FFTWidget::onFreqZoomChanged(int)
{
  float newLogZoom =
      SCAST(float, m_ui->freqZoomSlider->value()) / 100.f;
  float newZoom = SU_POW(2.f, newLogZoom);

  if (!sufreleq(m_panelConfig->zoom, newZoom, 1e-4f)) {
    setFreqZoom(newZoom);
    refreshSpectrumScaleSettings();
  }
}

void
FFTWidget::onPaletteChanged(int index)
{
  m_selected = SigDiggerHelpers::instance()->getPalette(index);
  m_panelConfig->palette = m_selected->getName();

  refreshSpectrumWaterfallSettings();
}

void
FFTWidget::onFftSizeChanged()
{
  auto params = m_mediator->getAnalyzerParams();

  m_fftSize =
      m_sizes[
        SCAST(unsigned, m_ui->fftSizeCombo->currentIndex())];

  if (m_fftSize == 0)
    m_fftSize = m_defaultFftSize;

  updateRbw();

  params->windowSize = getFftSize();

  updateAnalyzerParams();
}

void
FFTWidget::onRefreshRateChanged()
{
  auto params = m_mediator->getAnalyzerParams();

  m_refreshRate =
      m_refreshRates[
        SCAST(unsigned, m_ui->rateCombo->currentIndex())];

  if (m_refreshRate == 0)
    m_refreshRate = m_defaultRefreshRate;

  params->psdUpdateInterval = 1.f / getRefreshRate();

  m_spectrum->setExpectedRate(getRefreshRate());

  updateAnalyzerParams();
}

void
FFTWidget::onTimeSpanChanged()
{
  m_panelConfig->timeSpan =
      m_timeSpans[
        SCAST(unsigned, m_ui->timeSpanCombo->currentIndex())];

  refreshSpectrumScaleSettings();
}

void
FFTWidget::onRangeLockChanged()
{
  setRangeLock(getRangeLock());
}

void
FFTWidget::onChannelsChanged()
{
  setShowChannels(getShowChannels());
  refreshSpectrumRepresentationSettings();
}

void
FFTWidget::onPeakChanged()
{
  setPeakHold(getPeakHold());
  setPeakDetect(getPeakDetect());

  refreshSpectrumRepresentationSettings();
}

void
FFTWidget::onFilledChanged()
{
  setFilled(getFilled());

  refreshSpectrumRepresentationSettings();
}

void
FFTWidget::onWindowFunctionChanged()
{
  auto params = m_mediator->getAnalyzerParams();
  params->windowFunction = getWindowFunction();

  updateAnalyzerParams();
}

void
FFTWidget::onTimeStampsChanged()
{
  setTimeStamps(getTimeStamps());

  refreshSpectrumWaterfallSettings();
}

void
FFTWidget::onBookmarksChanged()
{
  setBookmarks(getBookmarks());

  refreshSpectrumRepresentationSettings();
}

void
FFTWidget::onUnitChanged()
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  float currZPdB = zeroPointToDb();
  float newZp;
  std::string name = m_ui->unitsCombo->currentText().toStdString();
  auto it = sus->getSpectrumUnitFrom(name);

  m_ui->zeroPointSpin->setSuffix(" " + QString::fromStdString(name));

  if (it != sus->getLastSpectrumUnit()) {
    m_currentUnit = *it;
  } else {
    m_currentUnit.name      = "dBFS";
    m_currentUnit.dBPerUnit = 1.f;
    m_currentUnit.zeroPoint = 0;
  }

  m_panelConfig->unitName = m_currentUnit.name;

  newZp = dbToZeroPoint(currZPdB);

  m_spectrum->setUnits(
        QString::fromStdString(m_currentUnit.name),
        m_currentUnit.dBPerUnit,
        m_currentUnit.zeroPoint + newZp);

  setZeroPoint(newZp);
}

void
FFTWidget::onZeroPointChanged()
{
  float currZP = SCAST(float, m_ui->zeroPointSpin->value());

  m_panelConfig->zeroPoint = currZP;

  m_spectrum->setZeroPoint(m_currentUnit.zeroPoint + currZP);
}

void
FFTWidget::onGainChanged()
{
  m_panelConfig->gain = SCAST(float, m_ui->gainSpinBox->value());

  refreshSpectrumAxesSettings();
}

void
FFTWidget::onAnalyzerParams(const Suscan::AnalyzerParams &params)
{
  refreshParamControls(params);
}

void
FFTWidget::onSourceInfoMessage(Suscan::SourceInfoMessage const &msg)
{
  m_rate = SCAST(unsigned, msg.info()->getSampleRate());
  updateRbw();
}

void
FFTWidget::onRangeChanged(float min, float max)
{
  bool specState = m_spectrum->signalsBlocked();
  bool fftState  = m_ui->pandRange->signalsBlocked();

  m_spectrum->blockSignals(true);
  m_ui->pandRange->blockSignals(true);
  m_ui->wfRange->blockSignals(true);

  m_spectrum->setPandapterRange(min, max);
  setPandRangeMin(std::floor(min));
  setPandRangeMax(std::floor(max));

  if (getRangeLock()) {
    m_spectrum->setWfRange(min, max);
    setWfRangeMin(std::floor(min));
    setWfRangeMax(std::floor(max));
  }

  m_spectrum->blockSignals(specState);
  m_ui->pandRange->blockSignals(fftState);
  m_ui->wfRange->blockSignals(fftState);
}

void
FFTWidget::onZoomChanged(float level)
{
  bool oldState = m_ui->freqZoomSlider->blockSignals(true);
  setFreqZoom(level);
  m_ui->freqZoomSlider->blockSignals(oldState);
}

void
FFTWidget::onUTCChanged()
{
  setTimeStampsUTC(m_ui->timeZoneCombo->currentIndex() != 0);
  m_spectrum->setTimeStampsUTC(m_panelConfig->utcTimeStamps);
}
