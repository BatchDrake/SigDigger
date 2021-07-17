//
//    SymViewTab.h: Inspector's symbol view tab
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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

#ifndef SYMVIEWTAB_H
#define SYMVIEWTAB_H

#include <QWidget>
#include <Decider.h>
#include <ColorConfig.h>

namespace Ui {
  class SymViewTab;
}

class ThrottleControl;

namespace SigDigger {
  class SymViewTab : public QWidget
  {
    Q_OBJECT

    bool demodulating = false;
    bool scrolling = false;

    unsigned int bps = 1;

    void refreshSizes(void);
    void refreshVScrollBar(void) const;
    void refreshHScrollBar(void) const;

    unsigned int getHScrollOffset(void) const;
    unsigned int getVScrollPageSize(void) const;

    void connectAll(void);

  public:
    void setBitsPerSymbol(unsigned int);
    void feed(const Symbol *data, unsigned int length);
    void setThrottleControl(ThrottleControl *);
    void setEnabled(bool);
    void setColorConfig(const ColorConfig &);

    inline void
    feed(std::vector<Symbol> const &symVec)
    {
      this->feed(symVec.data(), symVec.size());
    }

    inline bool
    isRecording(void) const
    {
      return this->demodulating;
    }

    explicit SymViewTab(QWidget *parent = 0);
    ~SymViewTab();

  public slots:
    void onSymViewZoomChanged(unsigned int zoom);
    void onZoomReset(void);
    void onZoomChanged(void);
    void onClearSymView(void);
    void onSaveSymView(void);
    void onSymViewControlsChanged(void);
    void onStrideChanged(unsigned int stride);
    void onHOffsetChanged(int offset);
    void onOffsetChanged(unsigned int offset);
    void onHScrollBarChanged(int offset);
    void onScrollBarChanged(int offset);

  private:
    Ui::SymViewTab *ui;
  };
}

#endif // SYMVIEWTAB_H
