//
//    FACFrameSyncUI.h: UI for the FAC frame synchronizer
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

#ifndef FACFRAMESYNCUI_H
#define FACFRAMESYNCUI_H

#include <QWidget>
#include <Suscan/DecoderFactory.h>
#include <Decoder.h>

namespace Ui {
  class FACFrameSyncUI;
}

namespace SigDigger {
  class FACFrameSyncUI : public QWidget, public Suscan::DecoderUI
  {
      Q_OBJECT

    public:
      explicit FACFrameSyncUI(QWidget *parent = nullptr);
      ~FACFrameSyncUI();

      QWidget *asWidget(void);

      void setThrottleControl(ThrottleControl *);

      void setLength(int len);
      void setShift(int shift);
      void setProgress(qreal);
      void setBufferLength(int len);
      void setGuessingEnabled(bool);
      void setEnabled(bool);

      void setSequence(std::vector<Symbol> const &);
      void setStateString(QString const &);
      void setCandidateString(QString const &);
      void setApplyEnabled(bool enabled);

      int getLength(void) const;
      int getShift(void) const;
      int getBufferLength(void) const;
      bool isGuessingEnabled(void) const;
      bool isEnabled(void) const;

    public slots:
      void onControlsChanged(void);
      void onApplyConfig(void);
      void onToggleEnable(void);
      void onToggleGuess(void);

    signals:
      void controlsChanged(void);
      void applyConfig(void);
      void toggleEnable(bool);
      void toggleGuess(bool);

    private:
      Ui::FACFrameSyncUI *ui;
  };
}

#endif // FACFRAMESYNCUI_H
