//
//    FACTab.cpp: Fast Autocorrelation
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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

#include "FACTab.h"
#include "ui_FACTab.h"

using namespace SigDigger;

void
FACTab::resizeFAC(int size)
{
  this->buffer.resize(static_cast<size_t>(size));
  this->buffer.assign(this->buffer.size(), 0);

  this->fac.resize(static_cast<size_t>(size / 2));
  this->fac.assign(this->fac.size(), 0);

  this->ui->facWaveform->setData(&this->fac, true);

  this->ui->facWaveform->invalidate();
  this->adjustZoom = true;

  this->min = INFINITY;
  this->max = -INFINITY;

  this->p = 0;

  if (this->direct != nullptr)
    SU_FFTW(_destroy_plan)(this->direct);

  if (this->reverse != nullptr)
    SU_FFTW(_destroy_plan)(this->reverse);

  SU_ATTEMPT(this->direct = SU_FFTW(_plan_dft_1d)(
        static_cast<int>(this->buffer.size()),
        reinterpret_cast<SU_FFTW(_complex) *>(this->buffer.data()),
        reinterpret_cast<SU_FFTW(_complex) *>(this->buffer.data()),
        FFTW_FORWARD,
        FFTW_ESTIMATE));

  SU_ATTEMPT(this->reverse = SU_FFTW(_plan_dft_1d)(
        static_cast<int>(this->buffer.size()),
        reinterpret_cast<SU_FFTW(_complex) *>(this->buffer.data()),
        reinterpret_cast<SU_FFTW(_complex) *>(this->buffer.data()),
        FFTW_BACKWARD,
        FFTW_ESTIMATE));

  this->ui->facWaveform->zoomHorizontal(
        static_cast<qint64>(0),
        static_cast<qint64>(size / 2));
}

FACTab::FACTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FACTab)
{
  unsigned int i;

  ui->setupUi(this);

  this->connectAll();

  for (i = 9; i < 20; ++i)
    this->ui->facSizeCombo->addItem(
          QString::number(1 << i),
          QVariant::fromValue<int>(1 << i));

  this->ui->facSizeCombo->setCurrentIndex(16 - 9);

  this->ui->facWaveform->setRealComponent(true);
  this->onUnitsChanged();

  this->onChangeFACSize();
  this->onAdjustAveraging();
  this->onChangePeakDetect();

  this->ui->facWaveform->zoomVertical(
        static_cast<qreal>(0),
        static_cast<qreal>(1));

  gettimeofday(&this->lastRefresh, nullptr);
}

FACTab::~FACTab()
{
  delete ui;

  if (this->direct != nullptr)
    SU_FFTW(_destroy_plan)(this->direct);

  if (this->reverse != nullptr)
    SU_FFTW(_destroy_plan)(this->reverse);
}

void
FACTab::refreshUi(void)
{

}

void
FACTab::connectAll(void)
{
  connect(
        this->ui->recordButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onRecord(void)));

  connect(
        this->ui->unitCheck,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onUnitsChanged(void)));

  connect(
        this->ui->detectPeaksCheck,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onChangePeakDetect(void)));

  connect(
        this->ui->facSizeCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onChangeFACSize(void)));

  connect(
        this->ui->averagingSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onAdjustAveraging(void)));
}

void
FACTab::setThrottleControl(ThrottleControl *ctl)
{
  this->ui->facWaveform->setThrottleControl(ctl);
}

void
FACTab::setSampleRate(qreal fs)
{
  this->fs = fs;

  this->onUnitsChanged();

  this->buffer.assign(this->buffer.size(), 0);
  this->fac.assign(this->fac.size(), 0);
}

void
FACTab::setColorConfig(ColorConfig const &cfg)
{
  this->ui->facWaveform->setBackgroundColor(cfg.spectrumBackground);
  this->ui->facWaveform->setForegroundColor(cfg.spectrumForeground);
  this->ui->facWaveform->setAxesColor(cfg.spectrumAxes);
  this->ui->facWaveform->setTextColor(cfg.spectrumText);
  this->ui->facWaveform->setSelectionColor(cfg.selection);
}

void
FACTab::feed(const SUCOMPLEX *data, unsigned int size)
{
  size_t got, i;
  SUCOMPLEX *bufData = this->buffer.data();
  SUCOMPLEX *facData = this->fac.data();
  QList<WaveMarker> markers;
  WaveMarker marker;
  size_t bufLen = this->buffer.size();
  bool redraw = false;
  SUFLOAT localMax = -INFINITY;
  SUFLOAT power = 0;
  unsigned int powerCount = 0;
  size_t localMaxPos = 0;
  struct timeval tv, diff;

  while (size > 0) {
    got = size;
    if (got > (bufLen - this->p))
      got = bufLen - this->p;

    memcpy(bufData + this->p, data, got * sizeof(SUCOMPLEX));
    p    += got;

    if (p == bufLen) {
      p = 0;

      gettimeofday(&this->lastRefresh, nullptr);

      SU_FFTW(_execute(this->direct));

      for (i = 0; i < bufLen; ++i)
        bufData[i] *= SU_C_CONJ(bufData[i]);

      SU_FFTW(_execute(this->reverse));

      for (i = 0; i < bufLen / 2; ++i) {
        bufData[i] = SU_C_ABS(bufData[i]);

        if (i > 4) {
          if (SU_C_REAL(bufData[i]) > localMax) {
            localMax = SU_C_REAL(bufData[i]);
            localMaxPos = i;
          } else {
            power += SU_C_REAL(bufData[i]) * SU_C_REAL(bufData[i]);
            ++powerCount;
          }

          if (SU_C_REAL(bufData[i]) > this->max)
            this->max = SU_C_REAL(bufData[i]);

          if (SU_C_REAL(bufData[i]) < this->min)
            this->min = SU_C_REAL(bufData[i]);
        }
      }

      for (i = 0; i < bufLen / 2; ++i)
        SU_SPLPF_FEED(facData[i], SU_C_REAL(bufData[i]) / this->max, this->alpha);

      redraw = true;
    }

    size -= got;
    data += got;
  }

  if (redraw) {
    this->ui->progressBar->setValue(100);
    this->ui->facWaveform->refreshData();
    this->ui->facWaveform->invalidate();

    if (this->adjustZoom) {
      this->ui->facWaveform->zoomVertical(
            static_cast<qreal>(0),
            static_cast<qreal>(1));

      this->adjustZoom = false;
    }

    if (this->ui->detectPeaksCheck->isChecked()) {
      if (!isinf(localMax) && powerCount > 0) {
        SUFLOAT meanPower = SU_SQRT(power / powerCount);
        SUFLOAT sigmas = localMax / meanPower;

        if (sigmas > this->ui->sigmaSpin->value()) {
          marker.below = false;
          marker.x = localMaxPos;
          marker.string =
              "Max: " + QString::number(localMaxPos) +
              " (" + QString::number(sigmas, 'g', 2) + "σ)";
          markers.append(marker);
        }
      }
    }
    this->ui->facWaveform->setMarkerList(markers);
  } else {
    gettimeofday(&tv, nullptr);
    timersub(&tv, &this->lastRefresh, &diff);

    if (diff.tv_usec > 100000) {
      this->ui->progressBar->setValue(100. * this->p / bufLen);
      this->lastRefresh = tv;
    }
  }
}

/////////////////////////////////////// Slots //////////////////////////////////
void
FACTab::onRecord(void)
{
  this->recording = this->ui->recordButton->isChecked();
}

void
FACTab::onAdjustAveraging(void)
{
  this->alpha =
      1 - static_cast<SUFLOAT>(this->ui->averagingSlider->value() / 100.);
}

void
FACTab::onChangeFACSize(void)
{
  this->resizeFAC(this->ui->facSizeCombo->currentData().value<int>());
}

void
FACTab::onUnitsChanged(void)
{
  if (this->ui->unitCheck->isChecked()) {
    this->ui->facWaveform->setSampleRate(1);
    this->ui->facWaveform->setHorizontalUnits("sp");
  } else {
    this->ui->facWaveform->setSampleRate(this->fs);
    this->ui->facWaveform->setHorizontalUnits("s");
  }
}

void
FACTab::onChangePeakDetect()
{
  this->ui->sigmaSpin->setEnabled(this->ui->detectPeaksCheck->isChecked());
}
