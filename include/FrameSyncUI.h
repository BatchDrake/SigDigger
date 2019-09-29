//
//    FACFrameSyncUI.h: UI for the frame synchronizer
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
