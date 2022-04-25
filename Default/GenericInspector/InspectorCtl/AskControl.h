#ifndef ASKCONTROL_H
#define ASKCONTROL_H

#include <QWidget>
#include "InspectorCtl.h"

namespace Ui {
  class AskControl;
}

namespace SigDigger {
  class AskControl : public InspectorCtl
  {
      Q_OBJECT

    public:
      explicit AskControl(
          QWidget *parent,
          Suscan::Config *config);
      ~AskControl() override;

      bool applicable(QString const &key) override;
      void refreshUi(void) override;
      void parseConfig(void) override;

    private:
      Ui::AskControl *ui;
  };
}

#endif // ASKCONTROL_H
