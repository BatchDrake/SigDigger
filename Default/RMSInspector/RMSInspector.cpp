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
        ui->integrationTimeSpin,
        SIGNAL(changed(qreal, qreal)),
        this,
        SLOT(configChanged()));
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

  ui->integrationTimeSpin->setSampleRate(m_sampleRate);
  ui->integrationTimeSpin->setTimeMax(4);
  ui->integrationTimeSpin->setTimeValue(intTime);

  intTime = ui->integrationTimeSpin->timeValue();

  m_rmsTab->setSampleRate(1 / intTime);

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
      qreal rms  = sqrt(mean);

      if (m_analyzer != nullptr) {
        struct timeval tv = m_analyzer->getSourceTimeStamp();
        m_rmsTab->feed(
              SCAST(qreal, tv.tv_sec) + 1e-6 * SCAST(qreal, tv.tv_usec),
              rms);
      }

    }
    m_kahanC = m_kahanAcc = 0;
    m_count = 0;
  }
}

void
RMSInspector::updateMaxSamples()
{
  qreal maxTime = ui->integrationTimeSpin->timeValue();
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
}

void
RMSInspector::detachAnalyzer()
{
  m_analyzer = nullptr;
}

void
RMSInspector::setProfile(Suscan::Source::Config &)
{

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
  m_rmsTab->setColorConfig(cfg);

#define WATERFALL_CALL(x) ui->passBandSpectrum->x
  WATERFALL_CALL(setFftPlotColor(cfg.spectrumForeground));
  WATERFALL_CALL(setFftBgColor(cfg.spectrumBackground));
  WATERFALL_CALL(setFftAxesColor(cfg.spectrumAxes));
  WATERFALL_CALL(setFftTextColor(cfg.spectrumText));
  WATERFALL_CALL(setFilterBoxColor(cfg.filterBox));
#undef WATERFALL_CALL
}

void
RMSInspector::inspectorMessage(Suscan::InspectorMessage const &)
{

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
  this->ui->integrationTimeSpin->setTimeValue(m_uiConfig->integrationTime);

  // TODO
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
RMSInspector::configChanged()
{
  updateMaxSamples();
}
