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
#include "Inspector.h"
#include <SuWidgetsHelpers.h>

using namespace SigDigger;

Inspector::Inspector(
    QWidget *parent,
    const Suscan::InspectorMessage &msg,
    AppConfig const &config) :
  QWidget(parent),
  config(msg.getCConfig())
{
  QString name = getInspectorTabTitle(msg);
  this->handle = msg.getHandle();
  this->analyzer = nullptr;

  this->ui = std::make_unique<InspectorUI>(this, name, &this->config, config);
  this->ui->setAppConfig(config);
  this->ui->setBasebandRate(msg.getBasebandRate());
  this->ui->setSampleRate(msg.getEquivSampleRate());
  this->ui->setBandwidth(static_cast<unsigned int>(msg.getBandwidth()));
  this->ui->setLo(static_cast<int>(msg.getLo()));

  this->connect(
        this->ui.get(),
        SIGNAL(configChanged()),
        this,
        SLOT(onConfigChanged()));

  this->connect(
        this->ui.get(),
        SIGNAL(setSpectrumSource(unsigned int)),
        this,
        SLOT(onSetSpectrumSource(unsigned int)));

  this->connect(
        this->ui.get(),
        SIGNAL(loChanged(void)),
        this,
        SLOT(onLoChanged(void)));

  this->connect(
        this->ui.get(),
        SIGNAL(bandwidthChanged(void)),
        this,
        SLOT(onBandwidthChanged(void)));

  this->connect(
        this->ui.get(),
        SIGNAL(toggleEstimator(Suscan::EstimatorId, bool)),
        this,
        SLOT(onToggleEstimator(Suscan::EstimatorId, bool)));

  this->connect(
        this->ui.get(),
        SIGNAL(applyEstimation(QString, float)),
        this,
        SLOT(onApplyEstimation(QString, float)));

  connect(
        this->ui.get(),
        SIGNAL(setCorrection(Suscan::Orbit)),
        this,
        SLOT(onDopplerCorrection(Suscan::Orbit)));

  connect(
        this->ui.get(),
        SIGNAL(disableCorrection(void)),
        this,
        SLOT(onDisableCorrection(void)));

  connect(
        this->ui.get(),
        SIGNAL(openInspector(QString, qint64, qreal, bool)),
        this,
        SLOT(onOpenInspector(QString, qint64, qreal, bool)));

  connect(
        this->ui.get(),
        SIGNAL(nameChanged()),
        this,
        SIGNAL(nameChanged()));

  connect(
        this->ui.get(),
        SIGNAL(closeRequested()),
        this,
        SIGNAL(closeRequested()));

  for (auto p = msg.getSpectrumSources().begin();
       p != msg.getSpectrumSources().end();
       ++p)
    this->ui->addSpectrumSource(*p);

  for (auto p = msg.getEstimators().begin();
       p != msg.getEstimators().end();
       ++p)
    this->ui->addEstimator(*p);
}

QString
Inspector::getInspectorTabTitle(Suscan::InspectorMessage const &msg)
{
  QString result = " in "
      + SuWidgetsHelpers::formatQuantity(
        msg.getChannel().fc + msg.getChannel().ft,
        "Hz");

  if (msg.getClass() == "psk")
    return "PSK inspector" + result;
  else if (msg.getClass() == "fsk")
    return "FSK inspector" + result;
  else if (msg.getClass() == "ask")
    return "ASK inspector" + result;

  return "Generic inspector" + result;
}

void
Inspector::setAnalyzer(Suscan::Analyzer *analyzer)
{
  this->analyzer = analyzer;
  this->ui->setState(
        this->analyzer == nullptr
        ? InspectorUI::DETACHED
        : InspectorUI::ATTACHED);
}

void
Inspector::feed(const SUCOMPLEX *data, unsigned int size)
{
  this->ui->feed(data, size);
}

void
Inspector::feedSpectrum(
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
Inspector::showEvent(QShowEvent *)
{
  if (!this->adjusted) {
    this->ui->adjustSizes();
    this->adjusted = true;
  }
}

void
Inspector::updateEstimator(Suscan::EstimatorId id, float val)
{
  this->ui->updateEstimator(id, val);
}

void
Inspector::notifyOrbitReport(Suscan::OrbitReport const &report)
{
  this->ui->setOrbitReport(report);
}

void
Inspector::disableCorrection(void)
{
  this->ui->notifyDisableCorrection();
}

void
Inspector::setTunerFrequency(SUFREQ freq)
{
  this->ui->setTunerFrequency(freq);
}

void
Inspector::setRealTime(bool realTime)
{
  this->ui->setRealTime(realTime);
}

void
Inspector::setTimeStamp(struct timeval const &tv)
{
  this->ui->setTimeStamp(tv);
}

void
Inspector::setTimeLimits(
    struct timeval const &start,
    struct timeval const &end)
{
  this->ui->setTimeLimits(start, end);
}

Inspector::~Inspector()
{

}

/////////////////////////////////// Slots /////////////////////////////////////
void
Inspector::onConfigChanged(void)
{
  // TODO: Send config
  if (this->analyzer != nullptr) {
    this->analyzer->setInspectorConfig(
          this->handle,
          this->config,
          static_cast<Suscan::RequestId>(rand()));
  }
}

void
Inspector::onSetSpectrumSource(unsigned int index)
{
  if (this->analyzer != nullptr)
    this->analyzer->setSpectrumSource(
        this->handle,
        index,
        static_cast<Suscan::RequestId>(rand()));
}

void
Inspector::onLoChanged(void)
{
  if (this->analyzer != nullptr)
    this->analyzer->setInspectorFreq(
        this->handle,
        this->ui->getLo(),
        0);
}

void
Inspector::onBandwidthChanged(void)
{
  if (this->analyzer != nullptr)
    this->analyzer->setInspectorBandwidth(
        this->handle,
        this->ui->getBandwidth(),
        0);
}

void
Inspector::onToggleEstimator(Suscan::EstimatorId id, bool enabled)
{
  if (this->analyzer != nullptr)
    this->analyzer->setInspectorEnabled(this->handle, id, enabled, 0);
}

void
Inspector::onApplyEstimation(QString key, float value)
{
  this->config.set(key.toStdString(), value);
  if (this->analyzer != nullptr) {
    this->ui->refreshInspectorCtls();
    this->analyzer->setInspectorConfig(
          this->handle,
          this->config,
          static_cast<Suscan::RequestId>(rand()));
  }
}

void
Inspector::onDopplerCorrection(Suscan::Orbit orbit)
{
  if (this->analyzer != nullptr) {
    this->analyzer->setInspectorDopplerCorrection(
        this->handle,
        orbit,
        static_cast<Suscan::RequestId>(rand()));
  }
}

void
Inspector::onDisableCorrection(void)
{
  if (this->analyzer != nullptr)
    this->analyzer->disableDopplerCorrection(this->handle, 0);
}
void
Inspector::onOpenInspector(
    QString inspClass,
    qint64 freq,
    qreal bw,
    bool precise)
{
  Suscan::Channel ch;

  ch.bw    = bw;
  ch.ft    = 0;
  ch.fc    = static_cast<SUFREQ>(freq);
  ch.fLow  = - .5 * ch.bw;
  ch.fHigh = + .5 * ch.bw;

  this->analyzer->openEx(
        inspClass.toStdString(),
        ch,
        precise,
        this->handle,
        0);
}

