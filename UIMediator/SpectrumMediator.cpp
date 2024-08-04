//
//    Spectrum.cpp: Coordinate spectrum signals
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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

#include "UIMediator.h"
#include "MainWindow.h"
#include "GlobalProperty.h"
#include "MainSpectrum.h"
#include <InspectionWidgetFactory.h>
#include <SuWidgetsHelpers.h>

using namespace SigDigger;

void
UIMediator::feedPSD(const Suscan::PSDMessage &msg)
{
  bool expired = false;

  if (m_appConfig->guiConfig.enableMsgTTL) {
    qreal delta;
    qreal psdDelta;
    qreal prevDelta;
    qreal interval = m_appConfig->analyzerParams.psdUpdateInterval;
    qreal selRate = 1. / interval;
    struct timeval now, rttime, diff;
    qreal max_delta;
    qreal adj;
    max_delta = m_appConfig->guiConfig.msgTTL * 1e-3;

    gettimeofday(&now, nullptr);

    rttime = msg.getRealTimeStamp();

    /* Update current rtDelta */
    timersub(&now, &rttime, &diff);
    delta = diff.tv_sec + diff.tv_usec * 1e-6;

    timersub(&now, &m_lastPsd, &diff);
    psdDelta = diff.tv_sec + diff.tv_usec * 1e-6;

    m_lastPsd = now;

    if (m_rtCalibrations++ == 0) {
      m_rtDeltaReal = delta;
      m_psdDelta    = 1. / selRate;
      prevDelta         = m_psdDelta;
      adj               = prevDelta;
    } else {
      prevDelta         = m_psdDelta;
      SU_SPLPF_FEED(
            m_rtDeltaReal,
            delta,
            SU_SPLPF_ALPHA(SIGDIGGER_UI_MEDIATOR_PSD_CAL_LEN));
      SU_SPLPF_FEED(m_psdDelta, psdDelta, SU_SPLPF_ALPHA(selRate));
      adj               = m_psdDelta - prevDelta;
    }

    SU_SPLPF_FEED(m_psdAdj, adj, SU_SPLPF_ALPHA(selRate));

    if (!m_haveRtDelta) {
      if (++m_rtCalibrations > SIGDIGGER_UI_MEDIATOR_PSD_CAL_LEN)
        m_haveRtDelta = true;
    } else {
      /* Subtract the intrinsic time delta */
      delta -= m_rtDeltaReal;
      expired = delta > max_delta;

      if (m_appConfig->profile.isRemote()
          && fabs(m_psdAdj / interval)
          < SIGDIGGER_UI_MEDIATOR_PSD_LAG_THRESHOLD) {
        if ((m_psdDelta - interval) / interval
            > SIGDIGGER_UI_MEDIATOR_PSD_MAX_LAG) {
          if (m_laggedMsgBox == nullptr) {
            m_laggedMsgBox = new QMessageBox(m_owner);
            m_laggedMsgBox->setWindowTitle("Connection quality warning");
            m_laggedMsgBox->setWindowModality(Qt::NonModal);
            m_laggedMsgBox->setIcon(QMessageBox::Icon::Warning);
          }

          if (m_laggedMsgBox->isHidden()) {
            m_laggedMsgBox->setText(
                  QString::asprintf(
                    "The rate at which spectrum data is arriving is slower than "
                    "expected (requested %g fps, but it is arriving at %g fps). "
                    "This is most likely a bandwidth issue.\n\nIn order to prevent "
                    "server synchronization issues, please reduce either the "
                    "spectrum rate or the FFT size.",
                    selRate,
                    1. / m_psdDelta));
            m_laggedMsgBox->show();
          }
        }
      }
    }
  }

  setSampleRate(msg.getSampleRate());

  if (!expired || msg.hasLooped()) {
    m_averager.feed(msg);
    m_ui->spectrum->feed(
          m_averager.get(),
          static_cast<int>(m_averager.size()),
          msg.getTimeStamp(),
          msg.hasLooped());
  }
}

void
UIMediator::connectSpectrum(void)
{
  connect(
        m_ui->spectrum,
        SIGNAL(bandwidthChanged(void)),
        this,
        SLOT(onSpectrumBandwidthChanged(void)));

  connect(
        m_ui->spectrum,
        SIGNAL(frequencyChanged(qint64)),
        this,
        SLOT(onFrequencyChanged(qint64)));

  connect(
        m_ui->spectrum,
        SIGNAL(lnbFrequencyChanged(qint64)),
        this,
        SLOT(onFrequencyChanged(qint64)));

  connect(
        m_ui->spectrum,
        SIGNAL(loChanged(qint64)),
        this,
        SLOT(onLoChanged(qint64)));

  connect(
        m_ui->spectrum,
        SIGNAL(newBandPlan(QString)),
        this,
        SLOT(onNewBandPlan(QString)));

  connect(
        m_ui->spectrum,
        SIGNAL(seek(struct timeval)),
        this,
        SIGNAL(seek(struct timeval)));
}

void
UIMediator::onSpectrumBandwidthChanged(void)
{
  m_appConfig->bandwidth =
      static_cast<unsigned int>(m_ui->spectrum->getBandwidth());
}

void
UIMediator::onFrequencyChanged(qint64)
{
  qint64 freq = m_ui->spectrum->getCenterFreq();
  qint64 lnb  = m_ui->spectrum->getLnbFreq();

  if (isLive())
    m_appConfig->profile.setFreq(static_cast<SUFREQ>(freq));

  m_propFrequency->setValue(freq);
  m_propLNB->setValue(lnb);

  emit frequencyChanged(
        m_ui->spectrum->getCenterFreq(),
        m_ui->spectrum->getLnbFreq());
}

void
UIMediator::onLoChanged(qint64)
{
  m_appConfig->loFreq = static_cast<int>(m_ui->spectrum->getLoFreq());
}

void
UIMediator::onNewBandPlan(QString plan)
{
  addBandPlan(plan.toStdString());
}

void
UIMediator::onBookmarkChanged(void)
{
  m_ui->spectrum->updateOverlay();
  emit triggerSaveConfig();
}

void
UIMediator::onPropFrequencyChanged()
{
  int64_t newFreq = SCAST(qint64, m_propFrequency->toDouble());

  if (m_ui->spectrum->canChangeFrequency(
        newFreq,
        m_ui->spectrum->getLnbFreq())) {
    m_ui->spectrum->setFreqs(
        newFreq,
        m_ui->spectrum->getLnbFreq());
  } else {
    int64_t currFreq = m_ui->spectrum->getCenterFreq();
    m_propFrequency->setValueSilent(currFreq);
  }
}

void
UIMediator::onPropLNBChanged()
{
  m_ui->spectrum->setFreqs(
        m_ui->spectrum->getCenterFreq(),
        SCAST(qint64, m_propLNB->toDouble()));
}
