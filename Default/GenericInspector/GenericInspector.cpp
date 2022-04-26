//
//    Inspector.h: Inspector object
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

#include "InspectorUI.h"
#include "GenericInspector.h"
#include <SuWidgetsHelpers.h>
#include <UIMediator.h>

using namespace SigDigger;

void
GenericInspectorConfig::deserialize(Suscan::Object const &)
{

}

Suscan::Object &&
GenericInspectorConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("GenericInspectorConfig");

  return this->persist(obj);
}

GenericInspector::GenericInspector(
    InspectionWidgetFactory *factory,
    Suscan::AnalyzerRequest const &request,
    UIMediator *mediator,
    QWidget *parent) :
  InspectionWidget(factory, request, mediator, parent)
{
  QString name = getInspectorTabTitle();

  this->ui = new InspectorUI(
        this,
        name,
        &m_config,
        *mediator->getAppConfig());

  this->ui->setAppConfig(*mediator->getAppConfig());
  this->ui->setBasebandRate(request.basebandRate);
  this->ui->setSampleRate(request.equivRate);
  this->ui->setBandwidth(static_cast<unsigned int>(request.bandwidth));
  this->ui->setLo(static_cast<int>(request.lo));

  this->connect(
        this->ui,
        SIGNAL(configChanged()),
        this,
        SLOT(onConfigChanged()));

  this->connect(
        this->ui,
        SIGNAL(setSpectrumSource(unsigned int)),
        this,
        SLOT(onSetSpectrumSource(unsigned int)));

  this->connect(
        this->ui,
        SIGNAL(loChanged(void)),
        this,
        SLOT(onLoChanged(void)));

  this->connect(
        this->ui,
        SIGNAL(bandwidthChanged(void)),
        this,
        SLOT(onBandwidthChanged(void)));

  this->connect(
        this->ui,
        SIGNAL(toggleEstimator(Suscan::EstimatorId, bool)),
        this,
        SLOT(onToggleEstimator(Suscan::EstimatorId, bool)));

  this->connect(
        this->ui,
        SIGNAL(applyEstimation(QString, float)),
        this,
        SLOT(onApplyEstimation(QString, float)));

  connect(
        this->ui,
        SIGNAL(setCorrection(Suscan::Orbit)),
        this,
        SLOT(onDopplerCorrection(Suscan::Orbit)));

  connect(
        this->ui,
        SIGNAL(disableCorrection(void)),
        this,
        SLOT(onDisableCorrection(void)));

  connect(
        this->ui,
        SIGNAL(openInspector(QString, qint64, qreal, bool)),
        this,
        SLOT(onOpenInspector(QString, qint64, qreal, bool)));

  for (auto p = request.spectSources.begin();
       p != request.spectSources.end();
       ++p)
    this->ui->addSpectrumSource(*p);

  for (auto p = request.estimators.begin();
       p != request.estimators.end();
       ++p)
    this->ui->addEstimator(*p);
}

void
GenericInspector::attachAnalyzer(Suscan::Analyzer *analyzer)
{
  this->ui->setState(InspectorUI::ATTACHED);

  connect(
        analyzer,
        SIGNAL(source_info_message(Suscan::SourceInfoMessage const &)),
        this,
        SLOT(onSourceInfoMessage(Suscan::SourceInfoMessage const &)));
}

void
GenericInspector::detachAnalyzer()
{
  this->ui->setState(InspectorUI::DETACHED);
}


void
GenericInspector::setProfile(Suscan::Source::Config &profile)
{
  bool isRealTime = false;
  struct timeval tv, start, end;

  isRealTime = !profile.isRemote()
      && profile.getType() == SUSCAN_SOURCE_TYPE_SDR;


  if (isRealTime) {
    gettimeofday(&tv, nullptr);

    start = tv;
    start.tv_sec -= 1;
    start.tv_usec = 0;

    end = tv;
    end.tv_sec += 1;
    end.tv_usec = 0;
  } else {
    if (profile.fileIsValid()) {
      start = profile.getStartTime();
      end   = profile.getEndTime();
    } else {
      start = profile.getStartTime();
      end   = start;
      end.tv_sec += 1;
    }
    tv = start;
  }

  this->setRealTime(isRealTime);
  this->setTimeLimits(start, end);
  this->setTunerFrequency(profile.getFreq());

  this->setTimeStamp(start);
}

void
GenericInspector::setTimeStamp(struct timeval const &tv)
{
  this->ui->setTimeStamp(tv);
}

void
GenericInspector::setQth(Suscan::Location const &location)
{
  this->ui->setQth(location.site);
}

void
GenericInspector::inspectorMessage(Suscan::InspectorMessage const &msg)
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

      this->feedSpectrum(
            data,
            len,
            msg.getSpectrumRate(),
            msg.getSpectrumSourceId());
      break;

    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_ORBIT_REPORT:
      this->notifyOrbitReport(msg.getOrbitReport());
      break;

    case SUSCAN_ANALYZER_INSPECTOR_MSGKIND_SET_TLE:
      if (!msg.isTLEEnabled())
        this->disableCorrection();
      break;

    default:
      break;
  }
}

void
GenericInspector::samplesMessage(Suscan::SamplesMessage const &msg)
{
  this->feed(msg.getSamples(), msg.getCount());
}

std::string
GenericInspector::getLabel() const
{
  return this->getInspectorTabTitle().toStdString();
}

Suscan::Serializable *
GenericInspector::allocConfig(void)
{
  return new GenericInspectorConfig();
}

void
GenericInspector::applyConfig(void)
{

}

///////////////////////////// Private methods /////////////////////////////////
QString
GenericInspector::getInspectorTabTitle() const
{
  QString result = " in "
      + SuWidgetsHelpers::formatQuantity(
        this->request().channel.fc + this->request().channel.ft,
        "Hz");

  if (this->request().inspClass == "psk")
    return "PSK inspector" + result;
  else if (this->request().inspClass == "fsk")
    return "FSK inspector" + result;
  else if (this->request().inspClass == "ask")
    return "ASK inspector" + result;

  return "Generic inspector" + result;
}


void
GenericInspector::feed(const SUCOMPLEX *data, unsigned int size)
{
  this->ui->feed(data, size);
}


void
GenericInspector::feedSpectrum(
    const SUFLOAT *data,
    SUSCOUNT len,
    SUSCOUNT rate,
    uint32_t id)
{
  if (id != this->lastSpectrumId) {
    this->lastSpectrumId = id;
    this->ui->resetSpectrumLimits();
  }

  if (len > 0)
    this->ui->feedSpectrum(data, len, rate);
}

void
GenericInspector::showEvent(QShowEvent *)
{
  if (!this->adjusted) {
    this->ui->adjustSizes();
    this->adjusted = true;
  }
}

void
GenericInspector::updateEstimator(Suscan::EstimatorId id, float val)
{
  this->ui->updateEstimator(id, val);
}

void
GenericInspector::notifyOrbitReport(Suscan::OrbitReport const &report)
{
  this->ui->setOrbitReport(report);
}

void
GenericInspector::disableCorrection(void)
{
  this->ui->notifyDisableCorrection();
}

void
GenericInspector::setTunerFrequency(SUFREQ freq)
{
  this->ui->setTunerFrequency(freq);
}

void
GenericInspector::setRealTime(bool realTime)
{
  this->ui->setRealTime(realTime);
}

void
GenericInspector::floatStart()
{
  this->ui->beginReparenting();
}

void
GenericInspector::floatEnd()
{
  this->ui->doneReparenting();
}


void
GenericInspector::setTimeLimits(
    struct timeval const &start,
    struct timeval const &end)
{
  this->ui->setTimeLimits(start, end);
}

GenericInspector::~GenericInspector()
{

}

/////////////////////////////////// Slots /////////////////////////////////////
void
GenericInspector::onConfigChanged(void)
{
  // TODO: Send config
  if (this->analyzer() != nullptr) {
    this->analyzer()->setInspectorConfig(
          this->request().handle,
          this->config(),
          static_cast<Suscan::RequestId>(rand()));
  }
}

void
GenericInspector::onSetSpectrumSource(unsigned int index)
{
  if (this->analyzer() != nullptr)
    this->analyzer()->setSpectrumSource(
        this->request().handle,
        index,
        static_cast<Suscan::RequestId>(rand()));
}

void
GenericInspector::onLoChanged(void)
{
  if (this->analyzer() != nullptr)
    this->analyzer()->setInspectorFreq(
        this->request().handle,
        this->ui->getLo(),
        0);
}

void
GenericInspector::onBandwidthChanged(void)
{
  if (this->analyzer() != nullptr)
    this->analyzer()->setInspectorBandwidth(
        this->request().handle,
        this->ui->getBandwidth(),
        0);
}

void
GenericInspector::onToggleEstimator(Suscan::EstimatorId id, bool enabled)
{
  if (this->analyzer() != nullptr)
    this->analyzer()->setInspectorEnabled(this->request().handle, id, enabled, 0);
}

void
GenericInspector::onApplyEstimation(QString key, float value)
{
  m_config.set(key.toStdString(), value);

  if (this->analyzer() != nullptr) {
    this->ui->refreshInspectorCtls();
    this->analyzer()->setInspectorConfig(this->request().handle, m_config);
  }
}

void
GenericInspector::onDopplerCorrection(Suscan::Orbit orbit)
{
  if (this->analyzer() != nullptr) {
    this->analyzer()->setInspectorDopplerCorrection(
        this->request().handle,
        orbit,
        static_cast<Suscan::RequestId>(rand()));
  }
}

void
GenericInspector::onDisableCorrection(void)
{
  if (this->analyzer() != nullptr)
    this->analyzer()->disableDopplerCorrection(this->request().handle);
}
void
GenericInspector::onOpenInspector(
    QString inspClass,
    qint64 freq,
    qreal bw,
    bool precise)
{
  Suscan::Channel ch;
  auto className = inspClass.toStdString();

  ch.bw    = bw;
  ch.ft    = 0;
  ch.fc    = static_cast<SUFREQ>(freq);
  ch.fLow  = - .5 * ch.bw;
  ch.fHigh = + .5 * ch.bw;

  // TODO: request open subcarrier inspector on this handle

  this->mediator()->openInspectorTab(
        "GenericInspectorFactory",
        inspClass.toStdString().c_str(),
        ch,
        precise,
        this->request().handle);
}

void
GenericInspector::onSourceInfoMessage(Suscan::SourceInfoMessage const &msg)
{
  this->ui->setTunerFrequency(msg.info()->getFrequency());
}
