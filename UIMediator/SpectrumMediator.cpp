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
#include "MainSpectrum.h"
#include <InspectionWidgetFactory.h>
#include <SuWidgetsHelpers.h>

using namespace SigDigger;

void
UIMediator::feedPSD(const Suscan::PSDMessage &msg)
{
  bool expired = false;

  if (this->appConfig->guiConfig.enableMsgTTL) {
    qreal delta;
    qreal psdDelta;
    qreal prevDelta;
    qreal interval = this->appConfig->analyzerParams.psdUpdateInterval;
    qreal selRate = 1. / interval;
    struct timeval now, rttime, diff;
    qreal max_delta;
    qreal adj;
    max_delta = this->appConfig->guiConfig.msgTTL * 1e-3;

    gettimeofday(&now, nullptr);

    rttime = msg.getRealTimeStamp();

    /* Update current rtDelta */
    timersub(&now, &rttime, &diff);
    delta = diff.tv_sec + diff.tv_usec * 1e-6;

    timersub(&now, &this->lastPsd, &diff);
    psdDelta = diff.tv_sec + diff.tv_usec * 1e-6;

    this->lastPsd = now;

    if (this->rtCalibrations++ == 0) {
      this->rtDeltaReal = delta;
      this->psdDelta    = 1. / selRate;
      prevDelta         = this->psdDelta;
      adj               = prevDelta;
    } else {
      prevDelta         = this->psdDelta;
      SU_SPLPF_FEED(
            this->rtDeltaReal,
            delta,
            SU_SPLPF_ALPHA(SIGDIGGER_UI_MEDIATOR_PSD_CAL_LEN));
      SU_SPLPF_FEED(this->psdDelta, psdDelta, SU_SPLPF_ALPHA(selRate));
      adj               = this->psdDelta - prevDelta;
    }

    SU_SPLPF_FEED(this->psdAdj, adj, SU_SPLPF_ALPHA(selRate));

    if (!this->haveRtDelta) {
      if (++this->rtCalibrations > SIGDIGGER_UI_MEDIATOR_PSD_CAL_LEN)
        this->haveRtDelta = true;
    } else {
      /* Subtract the intrinsic time delta */
      delta -= this->rtDeltaReal;
      expired = delta > max_delta;

      if (this->appConfig->profile.isRemote()
          && fabs(this->psdAdj / interval)
          < SIGDIGGER_UI_MEDIATOR_PSD_LAG_THRESHOLD) {
        if ((this->psdDelta - interval) / interval
            > SIGDIGGER_UI_MEDIATOR_PSD_MAX_LAG) {
          if (this->laggedMsgBox == nullptr) {
            this->laggedMsgBox = new QMessageBox(this->owner);
            this->laggedMsgBox->setWindowTitle("Connection quality warning");
            this->laggedMsgBox->setWindowModality(Qt::NonModal);
            this->laggedMsgBox->setIcon(QMessageBox::Icon::Warning);
          }

          if (this->laggedMsgBox->isHidden()) {
            this->laggedMsgBox->setText(
                  QString::asprintf(
                    "The rate at which spectrum data is arriving is slower than "
                    "expected (requested %g fps, but it is arriving at %g fps). "
                    "This is most likely a bandwidth issue.\n\nIn order to prevent "
                    "server synchronization issues, please reduce either the "
                    "spectrum rate or the FFT size.",
                    selRate,
                    1. / this->psdDelta));
            this->laggedMsgBox->show();
          }
        }
      }
    }
  }

  this->setSampleRate(msg.getSampleRate());

  if (!expired) {
    this->averager.feed(msg);
    this->ui->spectrum->feed(
          this->averager.get(),
          static_cast<int>(this->averager.size()),
          msg.getTimeStamp(),
          msg.hasLooped());
  }
}

void
UIMediator::connectSpectrum(void)
{
  connect(
        this->ui->spectrum,
        SIGNAL(bandwidthChanged(void)),
        this,
        SLOT(onSpectrumBandwidthChanged(void)));

  connect(
        this->ui->spectrum,
        SIGNAL(frequencyChanged(qint64)),
        this,
        SLOT(onFrequencyChanged(qint64)));

  connect(
        this->ui->spectrum,
        SIGNAL(lnbFrequencyChanged(qint64)),
        this,
        SLOT(onFrequencyChanged(qint64)));

  connect(
        this->ui->spectrum,
        SIGNAL(loChanged(qint64)),
        this,
        SLOT(onLoChanged(qint64)));

  connect(
        this->ui->spectrum,
        SIGNAL(newBandPlan(QString)),
        this,
        SLOT(onNewBandPlan(QString)));

  connect(
        this->ui->spectrum,
        SIGNAL(seek(struct timeval)),
        this,
        SIGNAL(seek(struct timeval)));
}

void
UIMediator::onSpectrumBandwidthChanged(void)
{
  this->appConfig->bandwidth =
      static_cast<unsigned int>(this->ui->spectrum->getBandwidth());
}

void
UIMediator::onFrequencyChanged(qint64)
{
  qint64 freq = this->ui->spectrum->getCenterFreq();

  if (this->isLive())
    this->appConfig->profile.setFreq(static_cast<SUFREQ>(freq));

  emit frequencyChanged(
        this->ui->spectrum->getCenterFreq(),
        this->ui->spectrum->getLnbFreq());
}

void
UIMediator::onLoChanged(qint64)
{
  this->appConfig->loFreq = static_cast<int>(this->ui->spectrum->getLoFreq());
}

void
UIMediator::onNewBandPlan(QString plan)
{
  this->addBandPlan(plan.toStdString());
}

void
UIMediator::onBookmarkChanged(void)
{
  this->ui->spectrum->updateOverlay();
}

