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

using namespace SigDigger;

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
      if (p->frequency <= end) {
        BookmarkInfo info;
        info.name = QString::fromStdString(p->name);
        info.frequency = p->frequency;
        info.color = QColor(QString::fromStdString(p->color)).rgb();

        list.push_back(info);
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
  this->setShowFATs(true);

  this->bookmarkSource = new SuscanBookmarkSource();

  this->ui->mainSpectrum->setBookmarkSource(this->bookmarkSource);
}

MainSpectrum::~MainSpectrum()
{
  for (auto p : this->FATs)
    delete p;

  delete this->bookmarkSource;

  delete ui;
}

void
MainSpectrum::connectAll(void)
{
  connect(
        this->ui->mainSpectrum,
        SIGNAL(newFilterFreq(int, int)),
        this,
        SLOT(onWfBandwidthChanged(int, int)));

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

  connect(
        this->ui->mainSpectrum,
        SIGNAL(newDemodFreq(qint64, qint64)),
        this,
        SLOT(onWfLoChanged(void)));

  connect(
        this->ui->mainSpectrum,
        SIGNAL(pandapterRangeChanged(float, float)),
        this,
        SLOT(onRangeChanged(float, float)));

  connect(
        this->ui->mainSpectrum,
        SIGNAL(newCenterFreq(qint64)),
        this,
        SLOT(onNewCenterFreq(qint64)));

  connect(
        this->ui->mainSpectrum,
        SIGNAL(newZoomLevel(float)),
        this,
        SLOT(onNewZoomLevel(float)));
}

void
MainSpectrum::feed(float *data, int size)
{
  this->ui->mainSpectrum->setNewFftData(data, size);
}


void
MainSpectrum::updateLimits(void)
{
  qint64 minLcd = this->minFreq + this->getLnbFreq();
  qint64 maxLcd = this->maxFreq + this->getLnbFreq();

  // Center frequency LCD limits
  this->ui->fcLcd->setMin(minLcd);
  this->ui->fcLcd->setMax(maxLcd);

  // Demod frequency LCD limits
  minLcd = this->ui->fcLcd->getValue() - this->cachedRate / 2;
  maxLcd = this->ui->fcLcd->getValue() + this->cachedRate / 2;

  this->ui->loLcd->setMin(minLcd);
  this->ui->loLcd->setMax(maxLcd);
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
  this->ui->mainSpectrum->setWaterfallSpan(span * 1000);
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
  this->ui->mainSpectrum->setRunningState(false);
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
    this->ui->mainSpectrum->setFilterOffset(loFreq);
    emit loChanged(loFreq);
  }
}

void
MainSpectrum::setLnbFreq(qint64 lnbFreq)
{
  this->setFreqs(this->getCenterFreq(), lnbFreq);
}

void
MainSpectrum::setFreqs(qint64 freq, qint64 lnbFreq)
{
  qint64 loFreq = this->getLoFreq();

  this->ui->lnbLcd->setValue(lnbFreq);
  this->ui->fcLcd->setValue(freq);

  this->ui->mainSpectrum->setCenterFreq(freq);
  this->ui->mainSpectrum->setFreqUnits(getFrequencyUnits(freq));

  this->updateLimits();
  this->setLoFreq(loFreq);
}

void
MainSpectrum::setPaletteGradient(const QColor *table)
{
  this->ui->mainSpectrum->setPalette(table);
}

void
MainSpectrum::setPandapterRange(float min, float max)
{
  this->ui->mainSpectrum->setPandapterRange(min, max);
}

void
MainSpectrum::setWfRange(float min, float max)
{
  this->ui->mainSpectrum->setWaterfallRange(min, max);
}

void
MainSpectrum::setPanWfRatio(float ratio)
{
  this->ui->mainSpectrum->setPercent2DScreen(static_cast<int>(ratio * 100));
}

void
MainSpectrum::setPeakHold(bool hold)
{
  this->ui->mainSpectrum->setPeakHold(hold);
}

void
MainSpectrum::setPeakDetect(bool det)
{
  this->ui->mainSpectrum->setPeakDetection(det, 5);
}

void
MainSpectrum::setExpectedRate(int rate)
{
  this->ui->mainSpectrum->setExpectedRate(rate);
}

void
MainSpectrum::setTimeStamps(bool enabled)
{
  this->ui->mainSpectrum->setTimeStampsEnabled(enabled);
  this->ui->mainSpectrum->updateOverlay();
}

void
MainSpectrum::setBookmarks(bool enabled)
{
  this->ui->mainSpectrum->setBookmarksEnabled(enabled);
  this->ui->mainSpectrum->updateOverlay();
}

void
MainSpectrum::setUseLBMdrag(bool enabled)
{
  this->ui->mainSpectrum->setUseLBMdrag(enabled);
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

  this->ui->mainSpectrum->setFftPlotColor(cfg.spectrumForeground);
  this->ui->mainSpectrum->setFftAxesColor(cfg.spectrumAxes);
  this->ui->mainSpectrum->setFftBgColor(cfg.spectrumBackground);
  this->ui->mainSpectrum->setFftTextColor(cfg.spectrumText);
  this->ui->mainSpectrum->setFilterBoxColor(cfg.filterBox);
}

void
MainSpectrum::updateOverlay(void)
{
  this->ui->mainSpectrum->updateOverlay();
}

void
MainSpectrum::setFilterBandwidth(unsigned int bw)
{
  if (this->bandwidth != bw) {
    int freq = static_cast<int>(bw);
    bool lowerSideBand =
        this->filterSkewness == SYMMETRIC || this->filterSkewness == LOWER;
    bool upperSideBand =
        this->filterSkewness == SYMMETRIC || this->filterSkewness == UPPER;

    this->ui->mainSpectrum->setHiLowCutFrequencies(
          lowerSideBand ? -freq / 2 : 0,
          upperSideBand ? +freq / 2 : 0);
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

    this->ui->mainSpectrum->setDemodRanges(
          lowerSideBand ? -freq / 2 : 1,
          1,
          1,
          upperSideBand ? +freq / 2 : 1,
          skw == SYMMETRIC);

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
    this->ui->mainSpectrum->setSpanFreq(
          this->cachedRate / zoom);
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

    this->ui->mainSpectrum->setDemodRanges(
          lowerSideBand ? -freq / 2 : 1,
          1,
          1,
          upperSideBand ? +freq / 2 : 1,
          this->filterSkewness == SYMMETRIC);

    this->ui->mainSpectrum->setSampleRate(rate);

    this->ui->mainSpectrum->setSpanFreq(rate / this->zoom);
    this->ui->loLcd->setMin(-freq / 2 + this->getCenterFreq());
    this->ui->loLcd->setMax(freq / 2 + this->getCenterFreq());

    this->cachedRate = rate;
  }
}

void
MainSpectrum::setShowFATs(bool show)
{
  this->ui->mainSpectrum->setFATsVisible(show);
}

void
MainSpectrum::pushFAT(FrequencyAllocationTable *fat)
{
  this->ui->mainSpectrum->pushFAT(fat);
}

void
MainSpectrum::removeFAT(QString const &name)
{
  this->ui->mainSpectrum->removeFAT(name.toStdString());
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
  this->ui->loLcd->setValue(this->ui->mainSpectrum->getFilterOffset() + this->getCenterFreq());
  emit loChanged(this->getLoFreq());
}

void
MainSpectrum::onLoChanged(void)
{
  this->ui->mainSpectrum->setFilterOffset(this->getLoFreq());
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
