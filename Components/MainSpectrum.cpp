//
//    MainSpectrum.cpp: Main spectrum component
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

#include "MainSpectrum.h"
#include "ui_MainSpectrum.h"
#include <Suscan/Library.h>
#include <QTimeSlider.h>

#include "Waterfall.h"
#include "GLWaterfall.h"

using namespace SigDigger;

#define WATERFALL_CALL(call)        \
  do {                              \
    if (this->wf != nullptr)        \
      this->wf->call;               \
    else if (this->glWf != nullptr) \
      this->glWf->call;             \
  } while (false);                  \

namespace SigDigger {
  class SuscanBookmarkSource : public BookmarkSource {
    public:
      virtual QList<BookmarkInfo> getBookmarksInRange(qint64, qint64) override;
  };
}

QList<BookmarkInfo>
SuscanBookmarkSource::getBookmarksInRange(qint64 start, qint64 end)
{
  QList<BookmarkInfo> list;

  auto p = Suscan::Singleton::get_instance()->getBookmarkFrom(start);

  while (p != Suscan::Singleton::get_instance()->getLastBookmark()) {
    try {
      if (p->info.frequency <= end) {
        list.push_back(p->info);
      }

    } catch (Suscan::Exception const &) { }
    ++p;
  }

  return list;
}

MainSpectrum::MainSpectrum(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::MainSpectrum)
{
  ui->setupUi(this);

  this->connectAll();

  this->setFreqs(0, 0);
  this->lastFreqUpdate.start();
  this->bookmarkSource = new SuscanBookmarkSource();
}

MainSpectrum::~MainSpectrum()
{
  for (auto p : this->FATs)
    delete p;

  delete this->bookmarkSource;

  delete ui;
}

void
MainSpectrum::connectWf(void)
{
  connect(
        this->wf,
        SIGNAL(newFilterFreq(int, int)),
        this,
        SLOT(onWfBandwidthChanged(int, int)));

  connect(
        this->wf,
        SIGNAL(newDemodFreq(qint64, qint64)),
        this,
        SLOT(onWfLoChanged(void)));

  connect(
        this->wf,
        SIGNAL(pandapterRangeChanged(float, float)),
        this,
        SLOT(onRangeChanged(float, float)));

  connect(
        this->wf,
        SIGNAL(newCenterFreq(qint64)),
        this,
        SLOT(onNewCenterFreq(qint64)));

  connect(
        this->wf,
        SIGNAL(newZoomLevel(float)),
        this,
        SLOT(onNewZoomLevel(float)));

  connect(
        this->wf,
        SIGNAL(newModulation(QString)),
        this,
        SLOT(onNewModulation(QString)));
}

void
MainSpectrum::connectGLWf(void)
{
  connect(
        this->glWf,
        SIGNAL(newFilterFreq(int, int)),
        this,
        SLOT(onWfBandwidthChanged(int, int)));

  connect(
        this->glWf,
        SIGNAL(newDemodFreq(qint64, qint64)),
        this,
        SLOT(onWfLoChanged(void)));

  connect(
        this->glWf,
        SIGNAL(pandapterRangeChanged(float, float)),
        this,
        SLOT(onRangeChanged(float, float)));

  connect(
        this->glWf,
        SIGNAL(newCenterFreq(qint64)),
        this,
        SLOT(onNewCenterFreq(qint64)));

  connect(
        this->glWf,
        SIGNAL(newZoomLevel(float)),
        this,
        SLOT(onNewZoomLevel(float)));

  connect(
        this->glWf,
        SIGNAL(newModulation(QString)),
        this,
        SLOT(onNewModulation(QString)));
}

void
MainSpectrum::connectAll(void)
{
  connect(
        this->ui->fcLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onFrequencyChanged(void)));

  connect(
        this->ui->lnbLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onLnbFrequencyChanged(void)));

  connect(
        this->ui->loLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onLoChanged(void)));
}

void
MainSpectrum::addToolWidget(QWidget *widget, QString const &title)
{
  int widthHint = widget->sizeHint().width();
  bool collapsed = widget->property("collapsed").value<bool>();

  this->ui->multiToolBox->addItem(
        new MultiToolBoxItem(
          title,
          widget,
          !collapsed));

  if (this->maxToolWidth < widthHint)
    this->maxToolWidth = widthHint;
}

void
MainSpectrum::feed(float *data, int size, struct timeval const &tv, bool looped)
{
  QDateTime dateTime;

  dateTime.setMSecsSinceEpoch(tv.tv_sec * 1000 + tv.tv_usec / 1000);

  WATERFALL_CALL(setNewFftData(data, size, dateTime, looped));

  if (!this->resAdjusted) {
    this->resAdjusted = true;
    int res = static_cast<int>(
          round(static_cast<qreal>(this->cachedRate) / size));
    if (res < 1)
      res = 1;
    WATERFALL_CALL(setClickResolution(res));
    WATERFALL_CALL(setFilterClickResolution(res));
  }
}

void
MainSpectrum::updateLimits(void)
{
  qint64 minFreq = this->noLimits ? 0         : this->minFreq;
  qint64 maxFreq = this->noLimits ? 300000000 : this->maxFreq;

  qint64 minLcd = minFreq + this->getLnbFreq();
  qint64 maxLcd = maxFreq + this->getLnbFreq();


  // Center frequency LCD limits
  this->ui->fcLcd->setMinSilent(minLcd);
  this->ui->fcLcd->setMaxSilent(maxLcd);

  // Demod frequency LCD limits
  minLcd = this->ui->fcLcd->getValue() - this->cachedRate / 2;
  maxLcd = this->ui->fcLcd->getValue() + this->cachedRate / 2;

  this->ui->loLcd->setMinSilent(minLcd);
  this->ui->loLcd->setMaxSilent(maxLcd);
}

void
MainSpectrum::setFrequencyLimits(qint64 min, qint64 max)
{
  this->minFreq = min;
  this->maxFreq = max;

  this->updateLimits();
}

void
MainSpectrum::refreshUi(void)
{
  QString modeText = "  Capture mode: ";

  switch (this->mode) {
    case UNAVAILABLE:
      modeText += "N/A";
      break;

    case CAPTURE:
      modeText += "LIVE";
      break;

    case REPLAY:
      modeText += "REPLAY";
      break;
  }

  this->ui->captureModeLabel->setText(modeText);

  if (this->throttling)
    this->ui->throttlingLabel->setText("  Throttling: ON");
  else
    this->ui->throttlingLabel->setText("  Throttling: OFF");
}

void
MainSpectrum::setThrottling(bool value)
{
  this->throttling = value;
  this->refreshUi();
}

void
MainSpectrum::setTimeSpan(quint64 span)
{
  WATERFALL_CALL(setWaterfallSpan(span * 1000));
}

void
MainSpectrum::setCaptureMode(CaptureMode mode)
{
  this->mode = mode;
  this->refreshUi();
}

int
MainSpectrum::getFrequencyUnits(qint64 freq)
{
  if (freq < 0)
    freq = -freq;

  if (freq < 1000)
    return 1;

  if (freq < 1000000)
    return 1000;

  if (freq < 1000000000)
    return 1000000;

  return 1000000000;
}

void
MainSpectrum::notifyHalt(void)
{
  WATERFALL_CALL(setRunningState(false));
  WATERFALL_CALL(setClickResolution(1));
  WATERFALL_CALL(setFilterClickResolution(1));
  this->resAdjusted = false;
}

void
MainSpectrum::setCenterFreq(qint64 freq)
{
  this->setFreqs(freq, this->getLnbFreq());
}

void
MainSpectrum::setLoFreq(qint64 loFreq)
{
  if (loFreq != this->getLoFreq()) {
    this->ui->loLcd->setValue(loFreq + this->getCenterFreq());
    WATERFALL_CALL(setFilterOffset(loFreq));
    emit loChanged(loFreq);
  }
}

void
MainSpectrum::setLnbFreq(qint64 lnbFreq)
{
  this->setFreqs(this->getCenterFreq(), lnbFreq);
}

void
MainSpectrum::setGracePeriod(qint64 period)
{
  this->freqGracePeriod = period;
}

void
MainSpectrum::setFreqs(qint64 freq, qint64 lnbFreq, bool silent)
{
  qint64 newLo = this->ui->loLcd->getValue() - freq;

  if (silent
      && !this->lastFreqUpdate.hasExpired(this->freqGracePeriod))
    return;

  if (silent) {
    this->ui->lnbLcd->setValueSilent(lnbFreq);
    this->ui->fcLcd->setValueSilent(freq);
  } else {
    this->lastFreqUpdate.start();
    this->ui->lnbLcd->setValue(lnbFreq);
    this->ui->fcLcd->setValue(freq);
  }

  WATERFALL_CALL(setCenterFreq(freq));
  WATERFALL_CALL(setFreqUnits(getFrequencyUnits(freq)));

  this->updateLimits();
  this->setLoFreq(newLo);
}

void
MainSpectrum::setPaletteGradient(const QColor *table)
{
  WATERFALL_CALL(setPalette(table));
}

void
MainSpectrum::setPandapterRange(float min, float max)
{
  WATERFALL_CALL(setPandapterRange(min, max));
}

void
MainSpectrum::setWfRange(float min, float max)
{
  WATERFALL_CALL(setWaterfallRange(min, max));
}

void
MainSpectrum::setPanWfRatio(float ratio)
{
  WATERFALL_CALL(setPercent2DScreen(static_cast<int>(ratio * 100)));
}

void
MainSpectrum::setPeakHold(bool hold)
{
  WATERFALL_CALL(setPeakHold(hold));
}

void
MainSpectrum::setPeakDetect(bool det)
{
  WATERFALL_CALL(setPeakDetection(det, 5));
}

void
MainSpectrum::setExpectedRate(int rate)
{
  WATERFALL_CALL(setExpectedRate(rate));
}

void
MainSpectrum::setTimeStamps(bool enabled)
{
  WATERFALL_CALL(setTimeStampsEnabled(enabled));
  WATERFALL_CALL(updateOverlay());
}

void
MainSpectrum::setBookmarks(bool enabled)
{
  WATERFALL_CALL(setBookmarksEnabled(enabled));
  WATERFALL_CALL(updateOverlay());
}

void
MainSpectrum::setColorConfig(ColorConfig const &cfg)
{
  QString styleSheet =
      "background-color: " + cfg.lcdBackground.name() + "; \
      color: " + cfg.lcdForeground.name() + "; \
      font-size: 12px; \
      font-family: Monospace; \
      font-weight: bold;";

  this->ui->fcLcd->setForegroundColor(cfg.lcdForeground);
  this->ui->fcLcd->setBackgroundColor(cfg.lcdBackground);
  this->ui->loLcd->setForegroundColor(cfg.lcdForeground);
  this->ui->loLcd->setBackgroundColor(cfg.lcdBackground);
  this->ui->lnbLcd->setForegroundColor(cfg.lcdForeground);
  this->ui->lnbLcd->setBackgroundColor(cfg.lcdBackground);

  this->ui->loLabel->setStyleSheet(styleSheet);
  this->ui->lnbLabel->setStyleSheet(styleSheet);
  this->ui->captureModeLabel->setStyleSheet(styleSheet);
  this->ui->throttlingLabel->setStyleSheet(styleSheet);

  WATERFALL_CALL(setFftPlotColor(cfg.spectrumForeground));
  WATERFALL_CALL(setFftAxesColor(cfg.spectrumAxes));
  WATERFALL_CALL(setFftBgColor(cfg.spectrumBackground));
  WATERFALL_CALL(setFftTextColor(cfg.spectrumText));
  WATERFALL_CALL(setFilterBoxColor(cfg.filterBox));

  this->lastColorConfig = cfg;
}

void
MainSpectrum::setGuiConfig(GuiConfig const &cfg)
{
  if (this->noLimits != cfg.noLimits) {
    this->noLimits = cfg.noLimits;
    this->updateLimits();
  }

  if (this->wf == nullptr && this->glWf == nullptr) {
    if (cfg.useGLWaterfall) {
      // OpenGL waterfall
      this->glWf = new GLWaterfall(this);
      this->glWf->setObjectName(QStringLiteral("mainSpectrum"));
      this->ui->gridLayout->addWidget(this->glWf, 2, 0, 1, 4);
      this->connectGLWf();
    } else {
      // Classic waterfall
      this->wf = new Waterfall(this);
      this->wf->setObjectName(QStringLiteral("mainSpectrum"));
      this->ui->gridLayout->addWidget(this->wf, 2, 0, 1, 4);
      this->connectWf();
    }

    WATERFALL_CALL(setBookmarkSource(this->bookmarkSource));
    WATERFALL_CALL(setClickResolution(1));
    WATERFALL_CALL(setFilterClickResolution(1));

    this->setShowFATs(true);
    this->setColorConfig(this->lastColorConfig);
  }

  if (this->glWf != nullptr)
    this->glWf->setMaxBlending(cfg.useMaxBlending);

  WATERFALL_CALL(setUseLBMdrag(cfg.useLMBdrag));
}

void
MainSpectrum::updateOverlay(void)
{
  WATERFALL_CALL(updateOverlay());
}

void
MainSpectrum::setGain(float gain)
{
  WATERFALL_CALL(setGain(gain));
}

void
MainSpectrum::setZeroPoint(float zeroPoint)
{
  WATERFALL_CALL(setZeroPoint(zeroPoint));
}

void
MainSpectrum::setUnits(QString const &name, float dBPerUnit, float zeroPoint)
{
  WATERFALL_CALL(setUnitName(name));
  WATERFALL_CALL(setdBPerUnit(dBPerUnit));
  WATERFALL_CALL(setZeroPoint(zeroPoint));
}

void
MainSpectrum::setFilterBandwidth(unsigned int bw)
{
  if (this->bandwidth != bw) {
    int freq = static_cast<int>(bw);

   WATERFALL_CALL(setHiLowCutFrequencies(
          computeLowCutFreq(freq),
          computeHighCutFreq(freq)));
    this->bandwidth = bw;
  }
}

void
MainSpectrum::setFilterSkewness(Skewness skw)
{
  if (skw != this->filterSkewness) {
    this->filterSkewness = skw;
    unsigned int bw = this->bandwidth;
    int freq = static_cast<int>(this->cachedRate);
    bool lowerSideBand =
        this->filterSkewness == SYMMETRIC || this->filterSkewness == LOWER;
    bool upperSideBand =
        this->filterSkewness == SYMMETRIC || this->filterSkewness == UPPER;

    WATERFALL_CALL(setDemodRanges(
          lowerSideBand ? -freq / 2 : 1,
          1,
          1,
          upperSideBand ? +freq / 2 : 1,
          skw == SYMMETRIC));

    this->bandwidth = 0;
    this->setFilterBandwidth(bw);
    emit bandwidthChanged();
  }
}

void
MainSpectrum::setZoom(unsigned int zoom)
{
  if (zoom > 0) {
    this->zoom = zoom;
    WATERFALL_CALL(setSpanFreq(this->cachedRate / zoom));
  }
}

void
MainSpectrum::setSampleRate(unsigned int rate)
{
  if (this->cachedRate != rate) {
    int freq = static_cast<int>(rate);
    bool lowerSideBand =
        this->filterSkewness == SYMMETRIC || this->filterSkewness == LOWER;
    bool upperSideBand =
        this->filterSkewness == SYMMETRIC || this->filterSkewness == UPPER;

    WATERFALL_CALL(setDemodRanges(
          lowerSideBand ? -freq / 2 : 1,
          1,
          1,
          upperSideBand ? +freq / 2 : 1,
          this->filterSkewness == SYMMETRIC));

    WATERFALL_CALL(setSampleRate(rate));

    WATERFALL_CALL(setSpanFreq(rate / this->zoom));
    this->ui->loLcd->setMin(-freq / 2 + this->getCenterFreq());
    this->ui->loLcd->setMax(freq / 2 + this->getCenterFreq());

    this->cachedRate = rate;
    this->resAdjusted = false;
  }
}

qint32 MainSpectrum::computeLowCutFreq(int bw) const
{
    bool lowerSideBand =
        this->filterSkewness == SYMMETRIC || this->filterSkewness == LOWER;
    return lowerSideBand ? -bw / 2 : 0;
}
qint32 MainSpectrum::computeHighCutFreq(int bw) const
{
    bool upperSideBand =
        this->filterSkewness == SYMMETRIC || this->filterSkewness == UPPER;

    return upperSideBand ? +bw / 2 : 0;
}

void
MainSpectrum::setShowFATs(bool show)
{
  WATERFALL_CALL(setFATsVisible(show));
}

void
MainSpectrum::pushFAT(FrequencyAllocationTable *fat)
{
  WATERFALL_CALL(pushFAT(fat));
}

void
MainSpectrum::removeFAT(QString const &name)
{
  WATERFALL_CALL(removeFAT(name.toStdString()));
}

FrequencyBand
MainSpectrum::deserializeFrequencyBand(Suscan::Object const &obj)
{
  FrequencyBand band;

  band.min = static_cast<qint64>(obj.get("min", 0.f));
  band.max = static_cast<qint64>(obj.get("max", 0.f));
  band.primary = obj.get("primary", std::string());
  band.secondary = obj.get("secondary", std::string());
  band.footnotes = obj.get("footnotes", std::string());

  band.color.setNamedColor(
        QString::fromStdString(obj.get("color", std::string("#1f1f1f"))));

  return band;
}

FrequencyAllocationTable *
MainSpectrum::getFAT(QString const &name) const
{
  std::string asStdString = name.toStdString();

  for (auto p : this->FATs)
    if (p->getName() == asStdString)
      return p;

  return nullptr;
}

void
MainSpectrum::adjustSizes(void)
{
  this->setSidePanelWidth(qBound(0, this->maxToolWidth, 220));
}

qreal
MainSpectrum::sidePanelRatio(void) const
{
  int fullWidth = this->ui->splitter->width();

  return static_cast<qreal>(this->ui->splitter->sizes()[1])
      / static_cast<qreal>(fullWidth);
}

void
MainSpectrum::setSidePanelWidth(int width)
{
  QList<int> sizes;
  int fullWidth = this->ui->splitter->width();

  if (fullWidth > 0) {
    sizes.append(fullWidth - width);
    sizes.append(width);

    this->ui->splitter->setSizes(sizes);
  }
}

int
MainSpectrum::sidePanelWidth(void) const
{
  return this->ui->splitter->sizes()[1];
}

void
MainSpectrum::setSidePanelRatio(qreal ratio)
{
  int fullWidth = this->ui->splitter->width();

  int width = static_cast<int>(ratio * fullWidth);

  this->setSidePanelWidth(width);
}

void
MainSpectrum::deserializeFATs(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();
  Suscan::Object bands;
  unsigned int i, count, ndx = 0;

  for (auto p = sus->getFirstFAT();
       p != sus->getLastFAT();
       p++) {
    this->FATs.resize(ndx + 1);
    this->FATs[ndx] = new FrequencyAllocationTable(p->getField("name").value());
    bands = p->getField("bands");

    SU_ATTEMPT(bands.getType() == SUSCAN_OBJECT_TYPE_SET);

    count = bands.length();

    for (i = 0; i < count; ++i) {
      try {
        this->FATs[ndx]->pushBand(deserializeFrequencyBand(bands[i]));
      } catch (Suscan::Exception &) {
      }
    }

    emit newBandPlan(QString::fromStdString(this->FATs[ndx]->getName()));
    ++ndx;
  }
}

////////////////////////////////// Getters ////////////////////////////////////
bool
MainSpectrum::getThrottling(void) const
{
  return this->throttling;
}

MainSpectrum::CaptureMode
MainSpectrum::getCaptureMode(void) const
{
  return this->mode;
}

qint64
MainSpectrum::getCenterFreq(void) const
{
  return this->ui->fcLcd->getValue();
}

qint64
MainSpectrum::getLoFreq(void) const
{
  return this->ui->loLcd->getValue() - this->getCenterFreq();
}

qint64
MainSpectrum::getLnbFreq(void) const
{
  return this->ui->lnbLcd->getValue();
}

unsigned int
MainSpectrum::getBandwidth(void) const
{
  return this->bandwidth;
}

//////////////////////////////// Slots /////////////////////////////////////////
void
MainSpectrum::onWfBandwidthChanged(int min, int max)
{
  this->setFilterBandwidth(
        (this->filterSkewness == SYMMETRIC ? 1 : 2)
        * static_cast<unsigned int>(max - min));
  emit bandwidthChanged();
}

void
MainSpectrum::onFrequencyChanged(void)
{
  qint64 freq = this->ui->fcLcd->getValue();
  this->setCenterFreq(freq);
  emit frequencyChanged(freq);
  this->onLoChanged();
}

void
MainSpectrum::onNewCenterFreq(qint64 freq)
{
  this->ui->fcLcd->setValue(freq);
  this->updateLimits();
}

void
MainSpectrum::onLnbFrequencyChanged(void)
{
  qint64 freq = this->ui->lnbLcd->getValue();
  this->setLnbFreq(freq);
  emit lnbFrequencyChanged(freq);
}

void
MainSpectrum::onWfLoChanged(void)
{
  if (this->wf != nullptr)
    this->ui->loLcd->setValue(this->wf->getFilterOffset() + this->getCenterFreq());
  else if (this->glWf != nullptr)
    this->ui->loLcd->setValue(this->glWf->getFilterOffset() + this->getCenterFreq());

  emit loChanged(this->getLoFreq());
}

void
MainSpectrum::onLoChanged(void)
{
  WATERFALL_CALL(setFilterOffset(this->getLoFreq()));
  emit loChanged(this->getLoFreq());
}

void
MainSpectrum::onRangeChanged(float min, float max)
{
  emit rangeChanged(min, max);
}

void
MainSpectrum::onNewZoomLevel(float level)
{
  emit zoomChanged(level);
}

void
MainSpectrum::onNewModulation(QString modulation)
{
  emit modulationChanged(modulation);
}

