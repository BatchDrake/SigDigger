//
//    ColorConfigTab.cpp: Color configuration tab
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
#include "ColorConfigTab.h"
#include "ui_ColorConfigTab.h"

using namespace SigDigger;

#define CCREFRESH(widget, field) this->ui->widget->setColor(this->colors.field)
#define CCSAVE(widget, field) this->ui->widget->getColor(this->colors.field)
#define CCCONNECT(widget)         \
  connect(                        \
      this->ui->widget,           \
      SIGNAL(colorChanged(QColor)), \
      this,                       \
      SLOT(onColorChanged(void)))
void
ColorConfigTab::save(void)
{
  CCSAVE(lcdFgColor, lcdForeground);
  CCSAVE(lcdBgColor, lcdBackground);
  CCSAVE(spectrumFgColor, spectrumForeground);
  CCSAVE(spectrumBgColor, spectrumBackground);
  CCSAVE(spectrumAxesColor, spectrumAxes);
  CCSAVE(spectrumTextColor, spectrumText);
  CCSAVE(timeStampColor, spectrumTimeStamps);
  CCSAVE(constellationFgColor, constellationForeground);
  CCSAVE(constellationBgColor, constellationBackground);
  CCSAVE(constellationAxesColor, constellationAxes);
  CCSAVE(transitionFgColor, transitionForeground);
  CCSAVE(transitionBgColor, transitionBackground);
  CCSAVE(transitionAxesColor, transitionAxes);
  CCSAVE(histogramFgColor, histogramForeground);
  CCSAVE(histogramBgColor, histogramBackground);
  CCSAVE(histogramAxesColor, histogramAxes);
  CCSAVE(histogramModelColor, histogramModel);
  CCSAVE(symViewLoColor, symViewLow);
  CCSAVE(symViewHiColor, symViewHigh);
  CCSAVE(symViewBgColor, symViewBackground);
  CCSAVE(selectionColor, selection);
  CCSAVE(filterBoxColor, filterBox);
}

void
ColorConfigTab::refreshUi(void)
{
  CCREFRESH(lcdFgColor, lcdForeground);
  CCREFRESH(lcdBgColor, lcdBackground);
  CCREFRESH(spectrumFgColor, spectrumForeground);
  CCREFRESH(spectrumBgColor, spectrumBackground);
  CCREFRESH(spectrumAxesColor, spectrumAxes);
  CCREFRESH(spectrumTextColor, spectrumText);
  CCREFRESH(timeStampColor, spectrumTimeStamps);
  CCREFRESH(constellationFgColor, constellationForeground);
  CCREFRESH(constellationBgColor, constellationBackground);
  CCREFRESH(constellationAxesColor, constellationAxes);
  CCREFRESH(transitionFgColor, transitionForeground);
  CCREFRESH(transitionBgColor, transitionBackground);
  CCREFRESH(transitionAxesColor, transitionAxes);
  CCREFRESH(histogramFgColor, histogramForeground);
  CCREFRESH(histogramBgColor, histogramBackground);
  CCREFRESH(histogramAxesColor, histogramAxes);
  CCREFRESH(histogramModelColor, histogramModel);
  CCREFRESH(symViewLoColor, symViewLow);
  CCREFRESH(symViewHiColor, symViewHigh);
  CCREFRESH(symViewBgColor, symViewBackground);
  CCREFRESH(selectionColor, selection);
  CCREFRESH(filterBoxColor, filterBox);
}

void
ColorConfigTab::connectAll(void)
{
  CCCONNECT(lcdFgColor);
  CCCONNECT(lcdBgColor);
  CCCONNECT(spectrumFgColor);
  CCCONNECT(spectrumBgColor);
  CCCONNECT(spectrumAxesColor);
  CCCONNECT(spectrumTextColor);
  CCCONNECT(timeStampColor);
  CCCONNECT(constellationFgColor);
  CCCONNECT(constellationBgColor);
  CCCONNECT(constellationAxesColor);
  CCCONNECT(transitionFgColor);
  CCCONNECT(transitionBgColor);
  CCCONNECT(transitionAxesColor);
  CCCONNECT(histogramFgColor);
  CCCONNECT(histogramBgColor);
  CCCONNECT(histogramAxesColor);
  CCCONNECT(histogramModelColor);
  CCCONNECT(symViewLoColor);
  CCCONNECT(symViewHiColor);
  CCCONNECT(symViewBgColor);
  CCCONNECT(selectionColor);
  CCCONNECT(filterBoxColor);
}

void
ColorConfigTab::setColorConfig(ColorConfig const &config)
{
  this->colors = config;
  this->refreshUi();
  this->modified = false;
}

ColorConfig
ColorConfigTab::getColorConfig(void) const
{
  return this->colors;
}

bool
ColorConfigTab::hasChanged(void) const
{
  return this->modified;
}

ColorConfigTab::ColorConfigTab(QWidget *parent) :
  ConfigTab(parent, "Colors"),
  ui(new Ui::ColorConfigTab)
{
  ui->setupUi(this);

  this->connectAll();
}

ColorConfigTab::~ColorConfigTab()
{
  delete ui;
}

////////////////////////////// Slots //////////////////////////////////////////
void
ColorConfigTab::onColorChanged(void)
{
  this->modified = true;
  emit changed();
}
