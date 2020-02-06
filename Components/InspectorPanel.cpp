//
//    InspectorPanel.cpp: Dockable inspector panel
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
#include "InspectorPanel.h"
#include "ui_InspectorPanel.h"

// FIXME!! THESE INCLUDES ARE AWFUL. CREATE SIGDIGGER UTILITY CLASS
#include "DataSaverUI.h"
#include <Waveform.h>
#include "SourcePanel.h"

using namespace SigDigger;

///////////////////////////// Inspector panel cnfig ////////////////////////////
#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

void
InspectorPanelConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(inspectorClass);
  LOAD(precise);
}

Suscan::Object &&
InspectorPanelConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("InspectorPanelConfig");

  STORE(inspectorClass);
  STORE(precise);

  return this->persist(obj);
}

////////////////////////// Inspector panel widget //////////////////////////////
Suscan::Serializable *
InspectorPanel::allocConfig(void)
{
  return this->panelConfig = new InspectorPanelConfig();
}

void
InspectorPanel::applyConfig(void)
{
  this->setInspectorClass(this->panelConfig->inspectorClass);
  this->setPrecise(this->panelConfig->precise);
}

void
InspectorPanel::connectAll(void)
{
  connect(
        this->ui->bandwidthSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onBandwidthChanged(int)));

  connect(
        this->ui->openInspectorButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onOpenInspector(void)));

  connect(
        this->ui->preciseCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onPreciseChanged(void)));

  connect(
        this->ui->captureButton,
        SIGNAL(pressed(void)),
        this,
        SLOT(onPressHold(void)));

  connect(
        this->ui->captureButton,
        SIGNAL(released(void)),
        this,
        SLOT(onReleaseHold(void)));

}

void
InspectorPanel::refreshUi(void)
{
  switch (this->state) {
    case DETACHED:
      this->ui->openInspectorButton->setEnabled(false);
      this->ui->bandwidthSpin->setEnabled(false);
      break;

    case ATTACHED:
      this->ui->openInspectorButton->setEnabled(true);
      this->ui->bandwidthSpin->setEnabled(true);
      break;
  }
}

void
InspectorPanel::postLoadInit(void)
{
  this->timeWindow = new TimeWindow(this);
}

void
InspectorPanel::setDemodFrequency(qint64 freq)
{
  this->ui->inspectorChannelLabel->setText(QString::number(freq) + " Hz");
}

void
InspectorPanel::setBandwidthLimits(unsigned int min, unsigned int max)
{
  this->ui->bandwidthSpin->setMinimum(static_cast<int>(min));
  this->ui->bandwidthSpin->setMaximum(static_cast<int>(max));
}

void
InspectorPanel::setBandwidth(unsigned int freq)
{
  this->ui->bandwidthSpin->setValue(static_cast<int>(freq));
}

void
InspectorPanel::setPrecise(bool precise)
{
  this->ui->preciseCheck->setChecked(precise);
}

void
InspectorPanel::setState(enum State state)
{
  if (this->state != state) {
    this->state = state;
    this->refreshUi();
  }
}

enum InspectorPanel::State
InspectorPanel::getState(void) const
{
  return this->state;
}

bool
InspectorPanel::getPrecise(void) const
{
  return this->ui->preciseCheck->isChecked();
}

void
InspectorPanel::setInspectorClass(std::string const &cls)
{
  if (cls == "psk")
    this->ui->pskRadio->setChecked(true);
  else if (cls == "fsk")
    this->ui->pskRadio->setChecked(true);
  else if (cls == "ask")
    this->ui->pskRadio->setChecked(true);
}

std::string
InspectorPanel::getInspectorClass(void) const
{
  if (this->ui->pskRadio->isChecked())
    return "psk";
  else if (this->ui->fskRadio->isChecked())
    return "fsk";
  else if (this->ui->askRadio->isChecked())
    return "ask";

  return "";
}

unsigned int
InspectorPanel::getBandwidth(void) const
{
  return static_cast<unsigned int>(this->ui->bandwidthSpin->value());
}

void
InspectorPanel::resetRawInspector(qreal fs)
{
  this->timeWindowFs = fs;
  this->data.resize(0);
  this->ui->sampleRateLabel->setText(
        SourcePanel::formatSampleRate(static_cast<unsigned>(fs)));
  this->ui->durationLabel->setText(Waveform::formatLabel(0, "s"));
  this->ui->memoryLabel->setText(DataSaverUI::formatCaptureSize(0));
}

void
InspectorPanel::feedRawInspector(const SUCOMPLEX *data, size_t size)
{
  if (this->ui->captureButton->isDown()) {
    this->data.insert(this->data.end(), data, data + size);
    this->ui->durationLabel->setText(
          Waveform::formatLabel(this->data.size() / this->timeWindowFs, "s"));
    this->ui->memoryLabel->setText(
          DataSaverUI::formatCaptureSize(this->data.size() * sizeof(SUCOMPLEX)));
  }
}

InspectorPanel::InspectorPanel(QWidget *parent) :
  PersistentWidget(parent),
  ui(new Ui::InspectorPanel)
{
  ui->setupUi(this);

  this->assertConfig();

  this->connectAll();
}

InspectorPanel::~InspectorPanel()
{
  delete ui;
}

/////////////////////////////////// Slots /////////////////////////////////////
void
InspectorPanel::onOpenInspector(void)
{
  this->panelConfig->inspectorClass = this->getInspectorClass();
  emit requestOpenInspector(QString::fromStdString(this->getInspectorClass()));
}

void
InspectorPanel::onBandwidthChanged(int bw)
{
  /* this->mainWindow->mainSpectrum->setHiLowCutFrequencies(-bw / 2, bw / 2); */
  emit bandwidthChanged(bw);
}

void
InspectorPanel::onPreciseChanged(void)
{
  this->panelConfig->precise = this->ui->preciseCheck->isChecked();
}

void
InspectorPanel::onPressHold(void)
{
  emit startRawCapture();
}

void
InspectorPanel::onReleaseHold(void)
{
  emit stopRawCapture();

  if (this->data.size() > 0) {
    this->timeWindow->setData(this->data, this->timeWindowFs);
    this->timeWindow->show();
    this->timeWindow->setFocus();
    this->timeWindow->onFit();

    this->ui->sampleRateLabel->setText(
          SourcePanel::formatSampleRate(static_cast<unsigned>(0)));
    this->ui->durationLabel->setText(Waveform::formatLabel(0, "s"));
    this->ui->memoryLabel->setText(DataSaverUI::formatCaptureSize(0));
  }
}
