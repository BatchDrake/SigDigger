#ifndef GUICONFIG_H
#define GUICONFIG_H

#include <Suscan/Serializable.h>

#include <QObject>

namespace SigDigger {
  class GuiConfig : public Suscan::Serializable
  {
    public:
        /**
         * @brief use left mouse button to drag
         * drag / change center frequency on FFT spectrum area
         */
        bool useLMBdrag;

      GuiConfig();
      GuiConfig(Suscan::Object const &conf);

      // Overriden methods
      void loadDefaults(void);
      void deserialize(Suscan::Object const &conf) override;
      Suscan::Object &&serialize(void) override;

    signals:

    public slots:
  };
}

#endif // GUICONFIG_H
