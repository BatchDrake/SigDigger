//
//    RMSInspector.cpp: description
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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
#include "RMSInspector.h"
#include "RMSViewTab.h"
#include "ui_RMSInspector.h"
#include "SuWidgetsHelpers.h"
#include "UIMediator.h"
#include "Default/FFT/FFTWidget.h"
#include "SigDiggerHelpers.h"

using namespace SigDigger;

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

void
RMSInspectorConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(integrate);
  LOAD(integrationTime);
  LOAD(dBscale);
  LOAD(autoFit);
  LOAD(autoScroll);
}

Suscan::Object &&
RMSInspectorConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("RMSInspectorConfig");

  STORE(integrate);
  STORE(integrationTime);
  STORE(dBscale);
  STORE(autoFit);
  STORE(autoScroll);

  return this->persist(obj);
}

QString
RMSInspector::getInspectorTabTitle() const
{

  QString result = " for "
      + SuWidgetsHelpers::formatQuantity(
        this->request().channel.fc + this->mediator()->getCurrentCenterFreq(),
        "Hz");

  return "RMS plot" + result;
}

void
RMSInspector::connectAll()
{
  connect(
        m_rmsTab,
        SIGNAL(integrationTimeChanged(qreal)),
        this,
        SLOT(onConfigChanged()));

  connect(
        ui->tabWidget,
        SIGNAL(currentChanged(int)),
        this,
        SLOT(onTabChanged()));

  connect(
        ui->passBandSpectrum,
        SIGNAL(pandapterRangeChanged(float, float)),
        this,
        SLOT(onRangeChanged(float, float)));

  connect(
        ui->freqLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onChangeLo(void)));

  connect(
        ui->bandwidthLcd,
        SIGNAL(valueChanged(void)),
        this,
        SLOT(onChangeBandwidth(void)));
}

RMSInspector::RMSInspector(
    InspectionWidgetFactory *factory,
    Suscan::AnalyzerRequest const &request,
    UIMediator *mediator,
    QWidget *parent) :
  InspectionWidget(factory, request, mediator, parent),
  ui(new Ui::RMSInspector())
{
  qreal intTime = RMS_INSPECTOR_DEFAULT_INTEGRATION_TIME_MS * 1e-3;
  ui->setupUi(this);

  m_rmsTab = new RMSViewTab(nullptr, nullptr);
  m_sampleRate = SCAST(qreal, request.equivRate);

  ui->freqLcd->setValue(SCAST(qint64, request.lo));
  ui->bandwidthLcd->setValue(SCAST(qint64, request.bandwidth));

  ui->freqLcd->setMin(-SCAST(int, request.basebandRate) / 2);
  ui->freqLcd->setMax(SCAST(int, request.basebandRate) / 2);

  ui->bandwidthLcd->setMin(0);
  ui->bandwidthLcd->setMax(SCAST(int, request.equivRate));

  m_rmsTab->setIntegrationTimeMode(1 / m_sampleRate, 24 * 3600);

  m_rmsTab->setSampleRate(1 / intTime);
  m_rmsTab->setIntegrationTimeHint(intTime);

  updateMaxSamples();

  ui->tabWidget->insertTab(0, m_rmsTab, "Power plot");
  ui->tabWidget->setCurrentIndex(0);


  connectAll();
}

void
RMSInspector::checkMaxSamples()
{
  if (m_count >= m_maxSamples) {
    if (m_count > 0) {
      qreal mean = m_kahanAcc / SCAST(qreal, m_count);

      if (m_analyzer != nullptr) {
        struct timeval tv = m_analyzer->getSourceTimeStamp();

        if (m_rmsTab->running())
          m_rmsTab->feed(
                SCAST(qreal, tv.tv_sec) + 1e-6 * SCAST(qreal, tv.tv_usec),
                mean);
      }

    }
    m_kahanC = m_kahanAcc = 0;
    m_count = 0;
  }
}

void
RMSInspector::feedSpectrum(
    const SUFLOAT *data,
    SUSCOUNT len,
    SUSCOUNT rate,
    uint32_t)
{
#define WATERFALL_CALL(x) ui->passBandSpectrum->x
  if (m_lastRate != rate) {
    WATERFALL_CALL(setSampleRate(SCAST(float, rate)));
    m_lastRate = rate;
  }

  m_fftData.resize(len);
  m_fftData.assign(data, data + len);

  WATERFALL_CALL(setNewFftData(
        static_cast<float *>(m_fftData.data()),
        SCAST(int, len)));

  if (false) {
    SUFLOAT min = +INFINITY;
    SUFLOAT max = -INFINITY;

    for (SUSCOUNT i = 0; i < len; ++i) {
      if (min > data[i])
        min = data[i];
      if (max < data[i])
        max = data[i];
    }

    if (!isinf(min) && !isinf(max)) {
      unsigned int updates;
      SUFLOAT range = max - min;
      SUFLOAT spacing = .1f * range;
      WATERFALL_CALL(setPandapterRange(min - max - spacing, spacing));
      WATERFALL_CALL(setWaterfallRange(min - max - spacing, spacing));

      // The inspector spectrum size is len, with 100 ms update period.
      // If the sample rate is fs, len / (fs * .1) is the number of intermediate
      // updates we need to wait for the spectrum to stabilize.

      updates = static_cast<unsigned>(len / (m_sampleRate * .1f));

      if (m_spectrumAdjustCounter++ > updates)
        m_haveSpectrumLimits = true;
    }
  }

  if (m_lastLen != len) {
    int res = SCAST(int,
          round(SCAST(qreal, rate) / SCAST(qreal, len)));
    if (res < 1)
      res = 1;

    m_lastLen = len;
    WATERFALL_CALL(resetHorizontalZoom());
    WATERFALL_CALL(setClickResolution(res));
    WATERFALL_CALL(setFilterClickResolution(res));
  }
#undef WATERFALL_CALL
}

void
RMSInspector::updateMaxSamples()
{
  qreal maxTime = m_rmsTab->getIntegrationTimeHint();

  if (m_uiConfig != nullptr)
    m_uiConfig->integrationTime = maxTime;
  m_maxSamples = SCAST(quint64, maxTime * m_sampleRate);

  checkMaxSamples();
}

RMSInspector::~RMSInspector()
{
  delete ui;
}


/////////////////////////// Overriden methods /////////////////////////////////
void
RMSInspector::attachAnalyzer(Suscan::Analyzer *analyzer)
{
  m_analyzer = analyzer;

  connect(
        analyzer,
        SIGNAL(source_info_message(Suscan::SourceInfoMessage const &)),
        this,
        SLOT(onSourceInfoMessage(Suscan::SourceInfoMessage const &)));
}

void
RMSInspector::detachAnalyzer()
{
  m_analyzer = nullptr;
}

void
RMSInspector::setProfile(Suscan::Source::Config &profile)
{
  m_tunerFreq = profile.getFreq();
}

void
RMSInspector::setTimeStamp(struct timeval const &)
{
}

void
RMSInspector::setQth(Suscan::Location const &)
{

}

void
RMSInspector::setColorConfig(ColorConfig const &cfg)
{
  QString styleSheet =
      "background-color: " + cfg.lcdBackground.name() + "; \
      color: " + cfg.lcdForeground.name() + "; \
      font-size: 12px; \
      font-family: Monospace; \
      font-weight: bold;";

  m_rmsTab->setColorConfig(cfg);

  ui->freqLcd->setBackgroundColor(cfg.lcdBackground);
  ui->freqLcd->setForegroundColor(cfg.lcdForeground);

  ui->bandwidthLcd->setBackgroundColor(cfg.lcdBackground);
  ui->bandwidthLcd->setForegroundColor(cfg.lcdForeground);

  ui->freqLabel->setStyleSheet(styleSheet);
  ui->bandwidthLabel->setStyleSheet(styleSheet);
  ui->padLabel->setStyleSheet(styleSheet);

#define WATERFALL_CALL(x) ui->passBandSpectrum->x
  WATERFALL_CALL(setFftPlotColor(cfg.spectrumForeground));
  WATERFALL_CALL(setFftBgColor(cfg.spectrumBackground));
  WATERFALL_CALL(setFftAxesColor(cfg.spectrumAxes));
  WATERFALL_CALL(setFftTextColor(cfg.spectrumText));
  WATERFALL_CALL(setFilterBoxColor(cfg.filterBox));
#undef WATERFALL_CALL
}

void
RMSInspector::inspectorMessage(Suscan::InspectorMessage const &msg)
{
  SUFLOAT *data;
  SUSCOUNT len, p;
  float x;

  switch (msg.getKind()) {
    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_SPECTRUM:
      data = msg.getSpectrumData();
      len = msg.getSpectrumLength();
      p = len / 2;

      for (auto i = 0u; i < len; ++i)
        data[i] = SU_POWER_DB(data[i]);

      for (auto i = 0u; i < len / 2; ++i) {
        x = data[i];
        data[i] = data[p];
        data[p] = x;

        if (++p == len)
          p = 0;
      }

      feedSpectrum(
            data,
            len,
            msg.getSpectrumRate(),
            msg.getSpectrumSourceId());
      break;
    default:
      break;
  }
}

void
RMSInspector::samplesMessage(Suscan::SamplesMessage const &samplesMsg)
{
  unsigned int count, i;
  qreal input, y, t;
  const SUCOMPLEX *samples = samplesMsg.getSamples();
  count = samplesMsg.getCount();

  // TODO: We can make this actually a little better. Look for the max
  // number of samples we can process in a row before calling
  // checkMaxSamples and then deliver to the rmstab

  for (i = 0; i < count; ++i) {
    input = SCAST(qreal, SU_C_REAL(samples[i] * SU_C_CONJ(samples[i])));
    y = input - m_kahanC;
    t = m_kahanAcc + y;

    m_kahanC = (t - m_kahanAcc) - y;
    m_kahanAcc = t;

    ++m_count;
    checkMaxSamples();
  }
}

Suscan::Serializable *
RMSInspector::allocConfig()
{
  m_uiConfig = new RMSInspectorConfig();

  return m_uiConfig;
}

void
RMSInspector::applyConfig()
{
  FFTWidgetConfig config;

  m_rmsTab->setIntegrationTimeHint(SCAST(qreal, m_uiConfig->integrationTime));

  updateMaxSamples();

  config.deserialize(
        mediator()->getAppConfig()->getComponentConfig("FFTWidget"));

  auto palette = SigDiggerHelpers::instance()->getPalette(config.palette);

  if (palette != nullptr)
    ui->passBandSpectrum->setPalette(palette->getGradient());
}


void
RMSInspector::showEvent(QShowEvent *)
{

}

void
RMSInspector::floatStart()
{

}

void
RMSInspector::floatEnd()
{

}


std::string
RMSInspector::getLabel() const
{
  return this->getInspectorTabTitle().toStdString();
}

//////////////////////////////// Slots /////////////////////////////////////////
void
RMSInspector::onConfigChanged()
{
  updateMaxSamples();
}

void
RMSInspector::onTabChanged()
{
  if (m_analyzer != nullptr) {
    if (ui->tabWidget->currentIndex() == 1) {
      m_analyzer->setSpectrumSource(
              this->request().handle,
              1,
              static_cast<Suscan::RequestId>(rand()));
    } else {
      m_analyzer->setSpectrumSource(
              this->request().handle,
              0,
              static_cast<Suscan::RequestId>(rand()));
    }
  }
}

void
RMSInspector::onRangeChanged(float min, float max)
{
  bool blocked = ui->passBandSpectrum->blockSignals(true);

  ui->passBandSpectrum->setWaterfallRange(min, max);
  ui->passBandSpectrum->blockSignals(blocked);
}

void
RMSInspector::onChangeLo()
{
  if (m_analyzer != nullptr) {
    int lo = SCAST(int, this->ui->freqLcd->getValue());
    m_analyzer->setInspectorFreq(
        this->request().handle,
        lo,
        0);

    if (m_haveNamedChannel) {
      m_namedChannel.value()->frequency = m_tunerFreq + lo;
      refreshNamedChannel();
    }
  }
}

void
RMSInspector::onChangeBandwidth()
{
  if (m_analyzer != nullptr) {
    int bw = SCAST(int, this->ui->bandwidthLcd->getValue());
    m_analyzer->setInspectorBandwidth(
        this->request().handle,
        bw,
        0);
    if (m_haveNamedChannel) {
      auto halfBw = bw / 2;
      m_namedChannel.value()->lowFreqCut  = -halfBw;
      m_namedChannel.value()->highFreqCut = +halfBw;
      refreshNamedChannel();
    }
  }
}

void
RMSInspector::onSourceInfoMessage(Suscan::SourceInfoMessage const &msg)
{
  m_tunerFreq = msg.info()->getFrequency();
}
