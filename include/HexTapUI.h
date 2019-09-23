#ifndef HEXTAPUI_H
#define HEXTAPUI_H

#include <QWidget>
#include <Suscan/DecoderFactory.h>

class ThrottleControl;

namespace Ui {
  class HexTap;
}

namespace SigDigger {
  typedef std::vector<uint8_t> FrameBytes;

  class HexTapUI : public QWidget, public Suscan::DecoderUI
  {
      Q_OBJECT

      FrameBytes *frameBytes = nullptr;
      size_t lastLen = 0;

      void repaint(void);

    public:
      explicit HexTapUI(QWidget *parent = nullptr);
      virtual QWidget *asWidget(void);
      virtual ~HexTapUI();

      void setFrameCount(int count);
      void setFrameBytes(FrameBytes *);
      void refreshFrom(size_t ptr);

      unsigned int frameId(void) const;
      int shift(void) const;
      bool pack(void) const;
      bool lsb(void) const;
      bool reverse(void) const;
      bool autoScroll(void) const;
      void setThrottleControl(ThrottleControl *);
      void resizeEvent(QResizeEvent *ev);

    signals:
      void controlsChanged(void);
      void clear(void);

    public slots:
      void onControlsChanged(void);
      void onScroll(int);
      void onClear(void);

    private:
      Ui::HexTap *ui;
  };
}

#endif // HEXTAPUI_H
