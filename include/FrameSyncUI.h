#ifndef FRAMESYNCUI_H
#define FRAMESYNCUI_H

#include <QWidget>
#include <Suscan/DecoderFactory.h>
#include <Decoder.h>

namespace Ui {
  class FrameSyncUI;
}

namespace SigDigger {
  class FrameSyncUI : public QWidget, public Suscan::DecoderUI
  {
      Q_OBJECT

      std::vector<Symbol> lsb2msb;
      std::vector<Symbol> sequence;
      unsigned int bps;
      unsigned int mask;

    public:
      explicit FrameSyncUI(QWidget *parent = nullptr);
      ~FrameSyncUI();
      QWidget *asWidget(void);

      void setThrottleControl(ThrottleControl *);

      void setBps(unsigned int bps);
      std::vector<Symbol> &getSequence(void);
      int getTimes(void) const;
      bool isRepetition(void) const;
      bool lsb(void) const;

    public slots:
      void onControlsChanged(void);

    signals:
      void controlsChanged(void);

    private:
      Ui::FrameSyncUI *ui;
  };
}

#endif // FRAMESYNCUI_H
