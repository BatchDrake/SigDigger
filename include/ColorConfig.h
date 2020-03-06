#ifndef COLORCONFIG_H
#define COLORCONFIG_H

#include <Suscan/Serializable.h>

#include <QObject>
#include <QColor>

#define SIGDIGGER_DEFAULT_FOREGROUND QColor(0x3e, 0xd2, 0x81, 0xff)
#define SIGDIGGER_DEFAULT_BACKGROUND QColor(0x0f, 0x11, 0x28, 0xff)
#define SIGDIGGER_DEFAULT_AXES       QColor(128, 128, 128, 255)
#define SIGDIGGER_DEFAULT_TEXT       QColor(0x9e, 0xff, 0xcb, 255)
#define SIGDIGGER_DEFAULT_SELECTION  QColor(0x1f, 0x23, 0x53, 255)

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
