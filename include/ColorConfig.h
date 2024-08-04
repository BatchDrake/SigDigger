//
//    ColorConfig.h: Color configuration object
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

#ifndef COLORCONFIG_H
#define COLORCONFIG_H

#include <Suscan/Serializable.h>

#include <QObject>
#include <QColor>

#define SIGDIGGER_DEFAULT_FOREGROUND QColor("#3ed180")
#define SIGDIGGER_DEFAULT_BACKGROUND QColor("#070d0c")
#define SIGDIGGER_DEFAULT_AXES       QColor("#152d2b")
#define SIGDIGGER_DEFAULT_TEXT       QColor("#ffffff")
#define SIGDIGGER_DEFAULT_SELECTION  QColor("#152d2b")
#define SIGDIGGER_DEFAULT_FILTER_BOX QColor("#327d54")

#define SIGDIGGER_DEFAULT_SV_FG_HI   QColor("#ffffff")
#define SIGDIGGER_DEFAULT_SV_FG_LO   QColor("#000000")
#define SIGDIGGER_DEFAULT_SV_BG      QColor("#134529")

namespace SigDigger {
  class ColorConfig : public Suscan::Serializable
  {
    public:
      QColor lcdForeground;
      QColor lcdBackground;
      QColor spectrumForeground;
      QColor spectrumBackground;
      QColor spectrumAxes;
      QColor spectrumText;
      QColor spectrumTimeStamps;
      QColor constellationForeground;
      QColor constellationBackground;
      QColor constellationAxes;
      QColor transitionForeground;
      QColor transitionBackground;
      QColor transitionAxes;
      QColor histogramForeground;
      QColor histogramBackground;
      QColor histogramAxes;
      QColor histogramModel;
      QColor symViewLow;
      QColor symViewHigh;
      QColor symViewBackground;
      QColor filterBox;
      QColor selection;

      ColorConfig();
      ColorConfig(Suscan::Object const &conf);

      // Overriden methods
      void loadDefaults(void);
      void deserialize(Suscan::Object const &conf) override;
      Suscan::Object &&serialize(void) override;

    signals:

    public slots:
  };
}

#endif // COLORCONFIG_H
