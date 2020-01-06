//
//    PanoramicDialog.cpp: Description
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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

#include <PanoramicDialog.h>
#include <Suscan/Library.h>
#include "ui_PanoramicDialog.h"

using namespace SigDigger;

PanoramicDialog::PanoramicDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::PanoramicDialog)
{
  ui->setupUi(static_cast<QDialog *>(this));
  this->connectAll();
}

PanoramicDialog::~PanoramicDialog()
{
  delete ui;
}

void
PanoramicDialog::connectAll(void)
{
  connect(
        this->ui->deviceCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onDeviceChanged(void)));

  connect(
        this->ui->fullRangeCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onFullRangeChanged(void)));

  connect(
        this->ui->rangeStartSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onFreqRangeChanged(void)));

  connect(
        this->ui->rangeEndSpin,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onFreqRangeChanged(void)));

  connect(
        this->ui->scanButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleScan(void)));

  connect(
        this->ui->waterfall,
        SIGNAL(newFilterFreq(int, int)),
        this,
        SLOT(onNewBandwidth(int, int)));

  connect(
        this->ui->waterfall,
        SIGNAL(newDemodFreq(qint64, qint64)),
        this,
        SLOT(onNewOffset()));

  connect(
        this->ui->waterfall,
        SIGNAL(newZoomLevel(float)),
        this,
        SLOT(onNewZoomLevel(void)));

  connect(
        this->ui->waterfall,
        SIGNAL(newCenterFreq(qint64)),
        this,
        SLOT(onNewZoomLevel(void)));

  connect(
        this->ui->rttSpin,
        SIGNAL(valueChanged(int)),
        this,
        SIGNAL(frameSkipChanged(void)));

  connect(
        this->ui->relBwSlider,
        SIGNAL(valueChanged(int)),
        this,
        SIGNAL(relBandwidthChanged(void)));

  connect(
        this->ui->waterfall,
        SIGNAL(pandapterRangeChanged(float, float)),
        this,
        SLOT(onRangeChanged(float, float)));
}

void
PanoramicDialog::refreshUi(void)
{
  bool empty = this->deviceMap.size() == 0;
  bool fullRange = this->ui->fullRangeCheck->isChecked();

  this->ui->deviceCombo->setEnabled(!this->running && !empty);
  this->ui->fullRangeCheck->setEnabled(!this->running && !empty);
  this->ui->rangeEndSpin->setEnabled(!this->running && !empty && !fullRange);
  this->ui->rangeStartSpin->setEnabled(!this->running && !empty && !fullRange);

  this->ui->scanButton->setChecked(this->running);
}

SUFREQ
PanoramicDialog::getMinFreq(void) const
{
  return this->ui->rangeStartSpin->value();
}

void
PanoramicDialog::setRunning(bool running)
{
  if (running && !this->running) {
    this->frames = 0;
    this->ui->framesLabel->setText("0");
  }

  this->running = running;
  this->refreshUi();
}

SUFREQ
PanoramicDialog::getMaxFreq(void) const
{
  return this->ui->rangeEndSpin->value();
}

void
PanoramicDialog::setWfRange(quint64 freqStart, quint64 freqEnd)
{
  qint64 bw = static_cast<qint64>(freqEnd - freqStart);
  quint64 fc = static_cast<quint64>(.5 * (freqEnd + freqStart));

  this->ui->waterfall->setLocked(false);
  this->ui->waterfall->setSampleRate(bw);
  this->ui->waterfall->setCenterFreq(fc);
  this->ui->waterfall->setDemodRanges(
        -bw / 2,
        0,
        0,
        bw / 2,
        true);

  this->ui->waterfall->setHiLowCutFrequencies(
        -bw / 20,
        bw / 20);

  this->ui->waterfall->resetHorizontalZoom();
}

void
PanoramicDialog::feed(
    quint64 freqStart,
    quint64 freqEnd,
    float *data,
    size_t size)
{
  if (this->freqStart != freqStart || this->freqEnd != freqEnd) {
    this->adjustingRange = true;
    this->freqStart = freqStart;
    this->freqEnd   = freqEnd;

    this->setWfRange(freqStart, freqEnd);
    this->adjustingRange = false;
  }

  this->ui->waterfall->setNewFftData(data, static_cast<int>(size));

  ++this->frames;
  this->redrawMeasures();
}

void
PanoramicDialog::setColors(ColorConfig const &cfg)
{
  this->ui->waterfall->setFftPlotColor(cfg.spectrumForeground);
  this->ui->waterfall->setFftAxesColor(cfg.spectrumAxes);
  this->ui->waterfall->setFftBgColor(cfg.spectrumBackground);
  this->ui->waterfall->setFftTextColor(cfg.spectrumText);
}

void
PanoramicDialog::setPaletteGradient(const QColor *table)
{
  this->ui->waterfall->setPalette(table);
}

void
PanoramicDialog::populateDeviceCombo(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  this->ui->deviceCombo->clear();
  this->deviceMap.clear();

  for (auto i = sus->getFirstDevice(); i != sus->getLastDevice(); ++i) {
    if (i->getMaxFreq() > 0 && i->isAvailable()) {
      std::string name = i->getDesc();
      this->deviceMap[name] = *i;
      this->ui->deviceCombo->addItem(QString::fromStdString(name));
    }
  }

  if (this->deviceMap.size() > 0)
    this->onDeviceChanged();

  this->refreshUi();
}

bool
PanoramicDialog::getSelectedDevice(Suscan::Source::Device &dev) const
{
  std::string name = this->ui->deviceCombo->currentText().toStdString();
  auto p = this->deviceMap.find(name);

  if (p != this->deviceMap.cend()) {
    dev = p->second;
    return true;
  }

  return false;
}

void
PanoramicDialog::adjustRanges(void)
{
  if (this->ui->rangeStartSpin->value() >
      this->ui->rangeEndSpin->value()) {
    auto val = this->ui->rangeStartSpin->value();
    this->ui->rangeStartSpin->setValue(
          this->ui->rangeEndSpin->value());
    this->ui->rangeEndSpin->setValue(val);
  }
}

bool
PanoramicDialog::invalidRange(void) const
{
  return fabs(
        this->ui->rangeEndSpin->value() - this->ui->rangeStartSpin->value()) < 1;
}
void
PanoramicDialog::setRanges(Suscan::Source::Device const &dev)
{
  SUFREQ minFreq = dev.getMinFreq();
  SUFREQ maxFreq = dev.getMaxFreq();

  // Prevents Waterfall frequencies from overflowing.

  if (minFreq > 2e9)
    minFreq = 2e9;

  if (maxFreq > 2e9)
    maxFreq = 2e9;

  this->ui->rangeStartSpin->setMinimum(minFreq);
  this->ui->rangeStartSpin->setMaximum(maxFreq);
  this->ui->rangeEndSpin->setMinimum(minFreq);
  this->ui->rangeEndSpin->setMaximum(maxFreq);

  if (this->invalidRange()) {
    this->ui->rangeStartSpin->setValue(minFreq);
    this->ui->rangeEndSpin->setValue(maxFreq);
  }

  this->adjustRanges();
}

void
PanoramicDialog::run(void)
{
  this->populateDeviceCombo();
  this->exec();
  emit stop();
}

void
PanoramicDialog::redrawMeasures(void)
{
  this->demodFreq = static_cast<qint64>(
        this->ui->waterfall->getFilterOffset() +
        .5 * (this->freqStart + this->freqEnd));

  this->ui->centerLabel->setText(
        QString::number(
          static_cast<qint64>(
            this->ui->waterfall->getFilterOffset() +
            .5 * (this->freqStart + this->freqEnd))) + " Hz");

  this->ui->bwLabel->setText(
        QString::number(this->ui->waterfall->getFilterBw()) + " Hz");

  this->ui->framesLabel->setText(QString::number(this->frames));
}

unsigned int
PanoramicDialog::getRttMs(void) const
{
  return static_cast<unsigned int>(this->ui->rttSpin->value());
}

float
PanoramicDialog::getRelBw(void) const
{
  return this->ui->relBwSlider->value() / 100.f;
}

////////////////////////////// Slots //////////////////////////////////////
void
PanoramicDialog::onDeviceChanged(void)
{
  Suscan::Source::Device dev;

  if (this->getSelectedDevice(dev)) {
    this->setRanges(dev);
    if (this->ui->fullRangeCheck->isChecked()) {
      this->ui->rangeStartSpin->setValue(dev.getMinFreq());
      this->ui->rangeEndSpin->setValue(dev.getMaxFreq());
    }
  }
}

void
PanoramicDialog::onFullRangeChanged(void)
{
  Suscan::Source::Device dev;
  bool checked = this->ui->fullRangeCheck->isChecked();

  if (this->getSelectedDevice(dev)) {
    if (checked) {
      this->ui->rangeStartSpin->setValue(dev.getMinFreq());
      this->ui->rangeEndSpin->setValue(dev.getMaxFreq());
    }
  }

  this->refreshUi();
}

void
PanoramicDialog::onFreqRangeChanged(void)
{
  this->adjustRanges();
}

void
PanoramicDialog::onToggleScan(void)
{
  if (this->ui->scanButton->isChecked())
    emit start();
  else
    emit stop();
}

void
PanoramicDialog::onNewZoomLevel(void)
{
  qint64 min, max;
  bool adj = false;

  if (!this->adjustingRange) {
    this->adjustingRange = true;

    min = abs(this->ui->waterfall->getCenterFreq() + this->ui->waterfall->getFftCenterFreq())
        - static_cast<qint64>(this->ui->waterfall->getSpanFreq() / 2);

    max = abs(this->ui->waterfall->getCenterFreq() + this->ui->waterfall->getFftCenterFreq())
        + static_cast<qint64>(this->ui->waterfall->getSpanFreq() / 2);

    if (min < this->getMinFreq()) {
      min = static_cast<qint64>(this->getMinFreq());
      adj = true;
    }

    if (max > this->getMaxFreq()) {
      max = static_cast<qint64>(this->getMaxFreq());
      adj = true;
    }

    if (adj)
      this->setWfRange(min, max);

    emit detailChanged(
          static_cast<quint64>(min),
          static_cast<quint64>(max));
    this->adjustingRange = false;
  }
}

void
PanoramicDialog::onRangeChanged(float min, float max)
{
  this->ui->waterfall->setWaterfallRange(min, max);
}

void
PanoramicDialog::onNewOffset(void)
{
  this->redrawMeasures();
}

void
PanoramicDialog::onNewBandwidth(int, int)
{
  this->redrawMeasures();
}

