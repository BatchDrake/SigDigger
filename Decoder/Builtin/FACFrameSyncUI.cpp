//
//    FACFrameSyncUI.cpp: UI for the FAC frame synchronizer
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

#include "include/FACFrameSyncUI.h"
#include "ui_FACFrameSyncUI.h"

using namespace SigDigger;

FACFrameSyncUI::FACFrameSyncUI(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FACFrameSyncUI)
{
  ui->setupUi(this);

  connect(
        this->ui->shiftSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->lengthSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->bufferSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->enableButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleEnable(void)));

  connect(
        this->ui->startButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onToggleGuess(void)));

  connect(
        this->ui->applyButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onApplyConfig(void)));
}

FACFrameSyncUI::~FACFrameSyncUI()
{
  delete ui;
}

void
FACFrameSyncUI::setThrottleControl(ThrottleControl *)
{

}

QWidget *
FACFrameSyncUI::asWidget(void)
{
  return this;
}

void
FACFrameSyncUI::setBufferLength(int len)
{
  this->ui->bufferSpin->setValue(len);
}

void
FACFrameSyncUI::setCandidateString(QString const &text)
{
  this->ui->candidateSyncLabel->setText(text);
}

void
FACFrameSyncUI::setStateString(QString const &text)
{
  this->ui->statusLabel->setText(text);
}

void
FACFrameSyncUI::setEnabled(bool enabled)
{
  this->ui->enableButton->setChecked(enabled);

  if (!enabled)
    this->ui->progressBar->setValue(0);
}

void
FACFrameSyncUI::setGuessingEnabled(bool enabled)
{
  this->ui->startButton->setChecked(enabled);
}

void
FACFrameSyncUI::setLength(int length)
{
  this->ui->lengthSpin->setValue(length);
  this->ui->shiftSpin->setMaximum(length - 1);
}

void
FACFrameSyncUI::setSequence(std::vector<Symbol> const &seq)
{
  this->ui->candidateSyncText->clear();

  for (auto p : seq) {
    char sym = static_cast<char>(p + '0');
    this->ui->candidateSyncText->insertPlainText(QString::fromUtf8(&sym, 1));
  }
}

void
FACFrameSyncUI::setShift(int shift)
{
  this->ui->shiftSpin->setValue(shift);
}

void
FACFrameSyncUI::setProgress(qreal value)
{
  this->ui->progressBar->setValue(static_cast<int>(value * 100.));
}

void
FACFrameSyncUI::setApplyEnabled(bool enabled)
{
  this->ui->applyButton->setEnabled(enabled);
}

int
FACFrameSyncUI::getLength(void) const
{
  return this->ui->lengthSpin->value();
}

int
FACFrameSyncUI::getShift(void) const
{
  return this->ui->shiftSpin->value();
}

int
FACFrameSyncUI::getBufferLength(void) const
{
  return this->ui->bufferSpin->value();
}

bool
FACFrameSyncUI::isGuessingEnabled(void) const
{
  return this->ui->startButton->isChecked();
}

bool
FACFrameSyncUI::isEnabled(void) const
{
  return this->ui->enableButton->isChecked();
}


////////////////////////////////// Slots //////////////////////////////////////
void
FACFrameSyncUI::onControlsChanged(void)
{
  emit controlsChanged();
}

void
FACFrameSyncUI::onApplyConfig(void)
{
  emit applyConfig();
}

void
FACFrameSyncUI::onToggleEnable(void)
{
  emit toggleEnable(this->ui->enableButton->isChecked());
}

void
FACFrameSyncUI::onToggleGuess(void)
{
  emit toggleGuess(this->ui->startButton->isChecked());
}


