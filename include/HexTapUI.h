#ifndef HEXTAP_H
#define HEXTAP_H

#include <QWidget>
#include <Suscan/DecoderFactory.h>

namespace Ui {
  class HexTap;
}

namespace SigDigger {
  class HexTapUI : public QWidget, Suscan::DecoderUI
  {
      Q_OBJECT

    public:
      explicit HexTapUI(QWidget *parent = nullptr);
      virtual ~HexTapUI();

    private:
      Ui::HexTap *ui;
  };
}

#endif // HEXTAP_H
