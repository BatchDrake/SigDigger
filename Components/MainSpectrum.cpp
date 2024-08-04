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
#include <QDateTime>
#include "Waterfall.h"
#include "GLWaterfall.h"
#include <WFHelpers.h>
#include <SuWidgetsHelpers.h>
#include <GlobalProperty.h>
#include <SigDiggerHelpers.h>

using namespace SigDigger;

#define WATERFALL_CALL(call)        \
  do {                              \
    if (m_wf != nullptr)            \
      m_wf->call;                   \
  } while (false);                  \

#define WATERFALL_CALL_LHT(t, call) \
  do {                              \
    if (m_wf != nullptr)            \
      t m_wf->call;                 \
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
  m_ui(new Ui::MainSpectrum)
{
  m_ui->setupUi(this);

  connectAll();

  setFreqs(0, 0);
  m_lastFreqUpdate.start();
  m_bookmarkSource = new SuscanBookmarkSource();

  gettimeofday(&m_lastTimeStamp, nullptr);
}

MainSpectrum::~MainSpectrum()
{
  for (auto p : m_FATs)
    delete p;

  delete m_bookmarkSource;

  delete m_ui;
}

void
MainSpectrum::connectWf(void)
{
  connect(
        m_wf,
        SIGNAL(newFilterFreq(int, int)),
        this,
        SLOT(onWfBandwidthChanged(int, int)));

  connect(
        m_wf,
        SIGNAL(newDemodFreq(qint64, qint64)),
        this,
        SLOT(onWfLoChanged(void)));

  connect(
        m_wf,
        SIGNAL(pandapterRangeChanged(float, float)),
        this,
        SLOT(onRangeChanged(float, float)));

  connect(
        m_wf,
        SIGNAL(newCenterFreq(qint64)),
        this,
        SLOT(onNewCenterFreq(qint64)));

  connect(
        m_wf,
        SIGNAL(newZoomLevel(float)),
        this,
        SLOT(onNewZoomLevel(float)));

  connect(
        m_wf,
        SIGNAL(newModulation(QString)),
        this,
        SLOT(onNewModulation(QString)));
}

void
MainSpectrum::connectAll(void)
{
  connect(
        m_ui->fcLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onFrequencyChanged(void)));

  connect(
        m_ui->fcLcd,
        SIGNAL(lockStateChanged(void)),
        this,
        SLOT(onLockStateChanged(void)));

  connect(
        m_ui->lnbLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onLnbFrequencyChanged(void)));

  connect(
        m_ui->loLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onLoChanged(void)));
}

void
MainSpectrum::addToolWidget(QWidget *widget, QString const &title)
{
  int widthHint = widget->sizeHint().width();
  bool collapsed = widget->property("collapsed").value<bool>();

  m_ui->multiToolBox->addItem(
        new MultiToolBoxItem(
          title,
          widget,
          !collapsed));

  if (m_maxToolWidth < widthHint)
    m_maxToolWidth = widthHint;
}

void
MainSpectrum::refreshFFTProperties()
{
  GlobalProperty::lookupProperty("fft_size")->setValue(m_cachedFftSize);
  QString rbwStr = m_cachedFftSize > 0 && m_cachedRate > 0
      ? SuWidgetsHelpers::formatQuantity(
          SCAST(qreal, m_cachedRate) / m_cachedFftSize,
          3,
          "Hz")
      : "N/A";
  GlobalProperty::lookupProperty("rbw")->setValue(rbwStr);
}

void
MainSpectrum::feed(float *data, int size, struct timeval const &tv, bool looped)
{
  QDateTime dateTime;

  if (size != SCAST(int, m_cachedFftSize)) {
    m_cachedFftSize = SCAST(unsigned, size);
    refreshFFTProperties();
    refreshInfoText();
  }

  dateTime.setMSecsSinceEpoch(tv.tv_sec * 1000 + tv.tv_usec / 1000);

  WATERFALL_CALL(setNewFftData(data, size, dateTime, looped));
}

void
MainSpectrum::updateLimits(void)
{
  qint64 minFreq = m_noLimits ? 0         : m_minFreq;
  qint64 maxFreq = m_noLimits ? 300000000000 : m_maxFreq;

  qint64 minLcd = minFreq + getLnbFreq();
  qint64 maxLcd = maxFreq + getLnbFreq();

  // Center frequency LCD limits
  m_ui->fcLcd->setMinSilent(minLcd);
  m_ui->fcLcd->setMaxSilent(maxLcd);

  WATERFALL_CALL(setFrequencyLimitsEnabled(!m_noLimits));
  WATERFALL_CALL(
        setFrequencyLimits(
          minLcd - m_cachedRate / 2,
          maxLcd + m_cachedRate / 2));

  // Demod frequency LCD limits
  minLcd = m_ui->fcLcd->getValue() - m_cachedRate / 2;
  maxLcd = m_ui->fcLcd->getValue() + m_cachedRate / 2;

  m_ui->loLcd->setMinSilent(minLcd);
  m_ui->loLcd->setMaxSilent(maxLcd);
}

void
MainSpectrum::setTimeStamp(const struct timeval &tv)
{
  m_lastTimeStamp = tv;

  refreshInfoText();
}

void
MainSpectrum::refreshInfoText()
{
  QString newText;

  newText = SigDiggerHelpers::expandGlobalProperties(m_infoTextTemplate);

  if (newText != m_infoText) {
    m_infoText = newText;
    WATERFALL_CALL(setInfoText(m_infoText));
  }
}

void
MainSpectrum::setFrequencyLimits(qint64 min, qint64 max)
{
  m_minFreq = min;
  m_maxFreq = max;

  updateLimits();
}

void
MainSpectrum::refreshUi(void)
{
  QString modeText = "  Capture mode: ";

  switch (m_mode) {
    case UNAVAILABLE:
      modeText += "N/A";
      break;

    case CAPTURE:
      modeText += "LIVE";
      break;

    case HISTORY:
      modeText += "HISTORY";
      break;

    case REPLAY:
      modeText += "REPLAY";
      break;
  }

  m_ui->captureModeLabel->setText(modeText);

  if (m_throttling)
    m_ui->throttlingLabel->setText("  Throttling: ON");
  else
    m_ui->throttlingLabel->setText("  Throttling: OFF");
}

void
MainSpectrum::setThrottling(bool value)
{
  m_throttling = value;
  refreshUi();
}

void
MainSpectrum::setTimeSpan(quint64 span)
{
  WATERFALL_CALL(setWaterfallSpan(span * 1000));
}

void
MainSpectrum::setCaptureMode(CaptureMode mode)
{
  m_mode = mode;
  refreshUi();
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
}

void
MainSpectrum::setCenterFreq(qint64 freq)
{
  setFreqs(freq, getLnbFreq());
}

void
MainSpectrum::setLoFreq(qint64 loFreq)
{
  m_ui->loLcd->setValue(loFreq + getCenterFreq());
  WATERFALL_CALL(setFilterOffset(loFreq));

  if (loFreq != getLoFreq())
    emit loChanged(loFreq);
}

void
MainSpectrum::setLnbFreq(qint64 lnbFreq)
{
  setFreqs(getCenterFreq(), lnbFreq);
}

void
MainSpectrum::setLocked(bool locked)
{
  m_ui->fcLcd->setLocked(locked);
  m_ui->lnbLcd->setLocked(locked);

  m_ui->fcLcd->setEnabled(!locked);
  m_ui->lnbLcd->setEnabled(!locked);

  onLockStateChanged();
}

void
MainSpectrum::setTimeStampsUTC(bool utc)
{
  WATERFALL_CALL(setTimeStampsUTC(utc));
}

void
MainSpectrum::setClickResolution(unsigned int res)
{
  WATERFALL_CALL(setClickResolution((int)res));

  // filter click resolution of 1/5th the tuning resolution is reasonable
  int filter_res = res > 5 ? (int)(res / 5) : 1;
  WATERFALL_CALL(setFilterClickResolution(filter_res));
}

void
MainSpectrum::setGracePeriod(qint64 period)
{
  m_freqGracePeriod = period;
}

void
MainSpectrum::setDisplayFreqs(qint64 freq, qint64 lnbFreq)
{
  setFreqs(freq, lnbFreq, true);
}

void
MainSpectrum::setFreqs(qint64 freq, qint64 lnbFreq, bool silent)
{
  qint64 newLo = m_ui->loLcd->getValue() - freq;

  if (silent
      && !m_lastFreqUpdate.hasExpired(m_freqGracePeriod))
    return;

  if (silent) {
    m_ui->lnbLcd->setValueSilent(lnbFreq);
    m_ui->fcLcd->setValueSilent(freq);
  } else {
    m_lastFreqUpdate.start();
    m_ui->lnbLcd->setValue(lnbFreq);
    m_ui->fcLcd->setValue(freq);
  }

  WATERFALL_CALL(setFreqUnits(getFrequencyUnits(freq)));
  WATERFALL_CALL(setCenterFreq(freq));

  updateLimits();

  if (silent)
    BLOCKSIG(this, setLoFreq(newLo));
  else
    setLoFreq(newLo);
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
MainSpectrum::setFilled(bool filled)
{
  WATERFALL_CALL(setFftFill(filled));
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

  m_ui->fcLcd->setForegroundColor(cfg.lcdForeground);
  m_ui->fcLcd->setBackgroundColor(cfg.lcdBackground);
  m_ui->loLcd->setForegroundColor(cfg.lcdForeground);
  m_ui->loLcd->setBackgroundColor(cfg.lcdBackground);
  m_ui->lnbLcd->setForegroundColor(cfg.lcdForeground);
  m_ui->lnbLcd->setBackgroundColor(cfg.lcdBackground);

  m_ui->loLabel->setStyleSheet(styleSheet);
  m_ui->lnbLabel->setStyleSheet(styleSheet);
  m_ui->captureModeLabel->setStyleSheet(styleSheet);
  m_ui->throttlingLabel->setStyleSheet(styleSheet);

  WATERFALL_CALL(setFftPlotColor(cfg.spectrumForeground));
  WATERFALL_CALL(setFftAxesColor(cfg.spectrumAxes));
  WATERFALL_CALL(setFftBgColor(cfg.spectrumBackground));
  WATERFALL_CALL(setFftTextColor(cfg.spectrumText));
  WATERFALL_CALL(setFilterBoxColor(cfg.filterBox));
  WATERFALL_CALL(setTimeStampColor(cfg.spectrumTimeStamps));

  m_lastColorConfig = cfg;
}

void
MainSpectrum::setGuiConfig(GuiConfig const &cfg)
{
  if (m_noLimits != cfg.noLimits) {
    m_noLimits = cfg.noLimits;
    updateLimits();
  }

  if (m_wf == nullptr) {
    if (cfg.useGLWaterfall) {
      // OpenGL waterfall
      m_wf = new GLWaterfall(this);
    } else {
      // Classic waterfall
      m_wf = new Waterfall(this);
    }

    m_wf->setObjectName(QStringLiteral("mainSpectrum"));
    m_ui->gridLayout->addWidget(m_wf, 2, 0, 1, 4);
    connectWf();
    m_wf->setBookmarkSource(m_bookmarkSource);

    setShowFATs(true);
    setColorConfig(m_lastColorConfig);
  }

  m_wf->setMaxBlending(cfg.useMaxBlending);
  m_wf->setUseLBMdrag(cfg.useLMBdrag);

  m_infoTextTemplate = QString::fromStdString(cfg.infoText).trimmed();
  m_infoText = m_infoTextTemplate;

  m_wf->setInfoText(m_infoTextTemplate);
  m_wf->setInfoTextColor(cfg.infoTextColor);

  refreshInfoText();
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
  if (m_bandwidth != bw) {
    int freq = static_cast<int>(bw);

   WATERFALL_CALL(setHiLowCutFrequencies(
          computeLowCutFreq(freq),
          computeHighCutFreq(freq)));
    m_bandwidth = bw;
   emit bandwidthChanged();
  }
}

void
MainSpectrum::setFilterSkewness(Skewness skw)
{
  if (skw != m_filterSkewness) {
    m_filterSkewness = skw;
    unsigned int bw = m_bandwidth;
    int freq = static_cast<int>(m_cachedRate);
    bool lowerSideBand =
        m_filterSkewness == SYMMETRIC || m_filterSkewness == LOWER;
    bool upperSideBand =
        m_filterSkewness == SYMMETRIC || m_filterSkewness == UPPER;

    WATERFALL_CALL(setDemodRanges(
          lowerSideBand ? -freq / 2 : 1,
          1,
          1,
          upperSideBand ? +freq / 2 : 1,
          skw == SYMMETRIC));

    m_bandwidth = 0;
    setFilterBandwidth(bw);
    emit bandwidthChanged();
  }
}

void
MainSpectrum::setZoom(float zoom)
{
  if (zoom > 0) {
    m_zoom = zoom;
    WATERFALL_CALL(setSpanFreq(m_cachedRate / zoom));
  }
}

void
MainSpectrum::setSampleRate(unsigned int rate)
{
  if (m_cachedRate != rate) {
    int freq = static_cast<int>(rate);
    bool lowerSideBand =
        m_filterSkewness == SYMMETRIC || m_filterSkewness == LOWER;
    bool upperSideBand =
        m_filterSkewness == SYMMETRIC || m_filterSkewness == UPPER;

    WATERFALL_CALL(setDemodRanges(
          lowerSideBand ? -freq / 2 : 1,
          1,
          1,
          upperSideBand ? +freq / 2 : 1,
          m_filterSkewness == SYMMETRIC));

    WATERFALL_CALL(setSampleRate(rate));

    WATERFALL_CALL(setSpanFreq(rate / m_zoom));
    m_ui->loLcd->setMin(-freq / 2 + getCenterFreq());
    m_ui->loLcd->setMax(freq / 2 + getCenterFreq());

    m_cachedRate = rate;
    refreshFFTProperties();
    refreshInfoText();
  }
}

qint32 MainSpectrum::computeLowCutFreq(int bw) const
{
    bool lowerSideBand =
        m_filterSkewness == SYMMETRIC || m_filterSkewness == LOWER;
    return lowerSideBand ? -bw / 2 : 0;
}
qint32 MainSpectrum::computeHighCutFreq(int bw) const
{
    bool upperSideBand =
        m_filterSkewness == SYMMETRIC || m_filterSkewness == UPPER;

    return upperSideBand ? +bw / 2 : 0;
}

void
MainSpectrum::setShowFATs(bool show)
{
  WATERFALL_CALL(setFATsVisible(show));
}

void
MainSpectrum::setShowChannels(bool show)
{
  WATERFALL_CALL(setChannelsEnabled(show));
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

  for (auto p : m_FATs)
    if (p->getName() == asStdString)
      return p;

  return nullptr;
}

MainSpectrum::Skewness
MainSpectrum::getFilterSkewness() const
{
  return m_filterSkewness;
}

void
MainSpectrum::adjustSizes(void)
{
  setSidePanelWidth(qBound(0, m_maxToolWidth, 220));
}

qreal
MainSpectrum::sidePanelRatio(void) const
{
  int fullWidth = m_ui->splitter->width();

  return static_cast<qreal>(m_ui->splitter->sizes()[1])
      / static_cast<qreal>(fullWidth);
}

void
MainSpectrum::setSidePanelWidth(int width)
{
  QList<int> sizes;
  int fullWidth = m_ui->splitter->width();

  if (fullWidth > 0) {
    sizes.append(fullWidth - width);
    sizes.append(width);

    m_ui->splitter->setSizes(sizes);
  }
}

int
MainSpectrum::sidePanelWidth(void) const
{
  return m_ui->splitter->sizes()[1];
}

void
MainSpectrum::setSidePanelRatio(qreal ratio)
{
  int fullWidth = m_ui->splitter->width();

  int width = static_cast<int>(ratio * fullWidth);

  setSidePanelWidth(width);
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
    m_FATs.resize(ndx + 1);
    m_FATs[ndx] = new FrequencyAllocationTable(p->getField("name").value());
    bands = p->getField("bands");

    SU_ATTEMPT(bands.getType() == SUSCAN_OBJECT_TYPE_SET);

    count = bands.length();

    for (i = 0; i < count; ++i) {
      try {
        m_FATs[ndx]->pushBand(deserializeFrequencyBand(bands[i]));
      } catch (Suscan::Exception &) {
      }
    }

    emit newBandPlan(QString::fromStdString(m_FATs[ndx]->getName()));
    ++ndx;
  }
}

///////////////////////////// Named Channel API ///////////////////////////////
NamedChannelSetIterator MainSpectrum::addChannel(
    QString name,
    qint64 frequency,
    qint32 fMin,
    qint32 fMax,
    QColor boxColor,
    QColor markerColor,
    QColor cutOffColor)
{
  WATERFALL_CALL_LHT(
      return,
      addChannel(
          name, frequency, fMin,
          fMax, boxColor, markerColor,
          cutOffColor));

  return NamedChannelSetIterator();
}

void
MainSpectrum::removeChannel(NamedChannelSetIterator it)
{
  WATERFALL_CALL(removeChannel(it));
}

void
MainSpectrum::refreshChannel(NamedChannelSetIterator &it)
{
  WATERFALL_CALL(refreshChannel(it));
}

NamedChannelSetIterator
MainSpectrum::findChannel(qint64 freq)
{
  WATERFALL_CALL_LHT(return, findChannel(freq));

  return NamedChannelSetIterator();
}

NamedChannelSetIterator
MainSpectrum::channelCBegin() const
{
  WATERFALL_CALL_LHT(return, channelCBegin());

  return NamedChannelSetIterator();
}

NamedChannelSetIterator
MainSpectrum::channelCEnd() const
{
  WATERFALL_CALL_LHT(return, channelCEnd());

  return NamedChannelSetIterator();
}


////////////////////////////////// Getters ////////////////////////////////////
bool
MainSpectrum::getThrottling(void) const
{
  return m_throttling;
}

MainSpectrum::CaptureMode
MainSpectrum::getCaptureMode(void) const
{
  return m_mode;
}

qint64
MainSpectrum::getCenterFreq(void) const
{
  return m_ui->fcLcd->getValue();
}

qint64
MainSpectrum::getLoFreq(void) const
{
  return m_ui->loLcd->getValue() - getCenterFreq();
}

qint64
MainSpectrum::getLnbFreq(void) const
{
  return m_ui->lnbLcd->getValue();
}

unsigned int
MainSpectrum::getBandwidth(void) const
{
  return m_bandwidth;
}

bool
MainSpectrum::canChangeFrequency(qint64 freq) const
{
  return canChangeFrequency(freq, getLnbFreq());
}

bool
MainSpectrum::canChangeFrequency(qint64 freq, qint64 lnb) const
{
  qint64 minFreq = m_noLimits ? 0            : m_minFreq;
  qint64 maxFreq = m_noLimits ? 300000000000 : m_maxFreq;

  qint64 minLcd = minFreq + lnb;
  qint64 maxLcd = maxFreq + lnb;

  return (minLcd <= freq) && (freq <= maxLcd);
}


//////////////////////////////// Slots /////////////////////////////////////////
void
MainSpectrum::onWfBandwidthChanged(int min, int max)
{
  setFilterBandwidth(
        (m_filterSkewness == SYMMETRIC ? 1 : 2)
        * static_cast<unsigned int>(max - min));
  emit bandwidthChanged();
}

void
MainSpectrum::onFrequencyChanged(void)
{
  qint64 freq = m_ui->fcLcd->getValue();
  setCenterFreq(freq);
  emit frequencyChanged(freq);
  onLoChanged();
}

void
MainSpectrum::onNewCenterFreq(qint64 freq)
{
  m_ui->fcLcd->setValue(freq);
  updateLimits();
}

void
MainSpectrum::onLnbFrequencyChanged(void)
{
  qint64 freq = m_ui->lnbLcd->getValue();
  setLnbFreq(freq);
  emit lnbFrequencyChanged(freq);
}

void
MainSpectrum::onWfLoChanged(void)
{
  if (m_wf != nullptr)
    m_ui->loLcd->setValue(m_wf->getFilterOffset() + getCenterFreq());

  emit loChanged(getLoFreq());
}

void
MainSpectrum::onLoChanged(void)
{
  WATERFALL_CALL(setFilterOffset(getLoFreq()));
  emit loChanged(getLoFreq());
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

void
MainSpectrum::onLockStateChanged(void)
{
  WATERFALL_CALL(setFreqDragLocked(m_ui->fcLcd->isLocked()));
}
