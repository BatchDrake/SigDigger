//
//    ColorConfig.cpp: Color configuration object
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

#include "ColorConfig.h"

using namespace SigDigger;

ColorConfig::ColorConfig()
{
  this->loadDefaults();
}

ColorConfig::ColorConfig(Suscan::Object const &conf) : ColorConfig()
{
  this->deserialize(conf);
}

void
ColorConfig::loadDefaults(void)
{
  this->lcdForeground = SIGDIGGER_DEFAULT_FOREGROUND;
  this->lcdBackground = SIGDIGGER_DEFAULT_BACKGROUND;
  this->spectrumForeground = SIGDIGGER_DEFAULT_FOREGROUND;

  this->spectrumBackground = SIGDIGGER_DEFAULT_BACKGROUND;
  this->constellationForeground = SIGDIGGER_DEFAULT_FOREGROUND;
  this->transitionForeground = SIGDIGGER_DEFAULT_FOREGROUND;
  this->histogramForeground = SIGDIGGER_DEFAULT_FOREGROUND;

  this->constellationBackground = SIGDIGGER_DEFAULT_BACKGROUND;
  this->lcdBackground = SIGDIGGER_DEFAULT_BACKGROUND;
  this->histogramBackground = SIGDIGGER_DEFAULT_BACKGROUND;
  this->transitionBackground = SIGDIGGER_DEFAULT_BACKGROUND;

  this->spectrumAxes = SIGDIGGER_DEFAULT_AXES;
  this->constellationAxes = SIGDIGGER_DEFAULT_AXES;
  this->transitionAxes = SIGDIGGER_DEFAULT_AXES;
  this->histogramAxes = SIGDIGGER_DEFAULT_AXES;

  this->symViewBackground = SIGDIGGER_DEFAULT_SV_BG;
  this->symViewHigh = SIGDIGGER_DEFAULT_SV_FG_HI;
  this->symViewLow = SIGDIGGER_DEFAULT_SV_FG_LO;

  this->spectrumText = SIGDIGGER_DEFAULT_TEXT;
  this->spectrumTimeStamps = SIGDIGGER_DEFAULT_TEXT;
  this->histogramModel = QColor(255, 255, 0, 255);

  this->selection = SIGDIGGER_DEFAULT_SELECTION;
  this->filterBox = SIGDIGGER_DEFAULT_FILTER_BOX;
}

#define STRINGFY(x) #x
#define CCSTORE(field) \
  obj.set(STRINGFY(field), this->field.name().toStdString())
#define CCLOAD(field)           \
  this->field = QColor(         \
      QString::fromStdString(   \
        conf.get(STRINGFY(field), this->field.name().toStdString())))

Suscan::Object &&
ColorConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("colorcfg");

  CCSTORE(lcdBackground);
  CCSTORE(lcdForeground);
  CCSTORE(spectrumBackground);
  CCSTORE(spectrumForeground);
  CCSTORE(spectrumAxes);
  CCSTORE(spectrumText);
  CCSTORE(constellationForeground);
  CCSTORE(constellationBackground);
  CCSTORE(constellationAxes);
  CCSTORE(transitionForeground);
  CCSTORE(transitionBackground);
  CCSTORE(transitionAxes);
  CCSTORE(histogramForeground);
  CCSTORE(histogramBackground);
  CCSTORE(histogramAxes);
  CCSTORE(histogramModel);
  CCSTORE(symViewLow);
  CCSTORE(symViewHigh);
  CCSTORE(symViewBackground);
  CCSTORE(selection);
  CCSTORE(filterBox);

  return this->persist(obj);
}

void
ColorConfig::deserialize(Suscan::Object const &conf)
{
  CCLOAD(lcdBackground);
  CCLOAD(lcdForeground);
  CCLOAD(spectrumBackground);
  CCLOAD(spectrumForeground);
  CCLOAD(spectrumAxes);
  CCLOAD(spectrumText);
  CCLOAD(constellationForeground);
  CCLOAD(constellationBackground);
  CCLOAD(constellationAxes);
  CCLOAD(transitionForeground);
  CCLOAD(transitionBackground);
  CCLOAD(transitionAxes);
  CCLOAD(histogramForeground);
  CCLOAD(histogramBackground);
  CCLOAD(histogramAxes);
  CCLOAD(histogramModel);
  CCLOAD(symViewLow);
  CCLOAD(symViewHigh);
  CCLOAD(symViewBackground);
  CCLOAD(selection);
  CCLOAD(filterBox);
}
