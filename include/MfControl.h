//
//    MfControl.h: Matched filter control
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

#ifndef MFCONTROL_H
#define MFCONTROL_H

#include <QWidget>
#include <InspectorCtl.h>

namespace Ui {
  class MfControl;
}

namespace SigDigger {
  class MfControl : public InspectorCtl
  {
      Q_OBJECT

    public:
      explicit MfControl(
          QWidget *parent,
          Suscan::Config *config);
      ~MfControl() override;

      bool applicable(QString const &key) override;
      void refreshUi(void) override;
      void parseConfig(void) override;

    private:
      Ui::MfControl *ui;
  };
}

#endif // MFCONTROL_H
