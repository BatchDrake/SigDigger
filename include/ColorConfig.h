#ifndef COLORCONFIG_H
#define COLORCONFIG_H

#include <Suscan/Serializable.h>

#include <QObject>
#include <QColor>

#define SIGDIGGER_DEFAULT_FOREGROUND QColor("#3ed281")
#define SIGDIGGER_DEFAULT_BACKGROUND QColor("#090a18")
#define SIGDIGGER_DEFAULT_AXES       QColor("#152d2b")
#define SIGDIGGER_DEFAULT_TEXT       QColor("#9effcb")
#define SIGDIGGER_DEFAULT_SELECTION  QColor("#152d2b")
#define SIGDIGGER_DEFAULT_FILTER_BOX QColor("#327d54")

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
