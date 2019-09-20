//
//    DecoderTab.h: Stream decoder tab UI
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

#ifndef DECODERTAB_H
#define DECODERTAB_H

#include <QWidget>
#include <DecoderDialog.h>
#include <Suscan/Compat.h>
#include <DecoderStack.h>

namespace Ui {
  class DecoderTab;
}

namespace Suscan {
  class Decoder;
}

namespace SigDigger {
  class DecoderTab : public QWidget
  {
    Q_OBJECT

    std::vector<Suscan::Decoder *> decoderList;
    DecoderDialog decoderDialog;
    DecoderStack stack;
    Decoder *termination = nullptr;
    bool ready = false;
    bool enabled = true;
    void connectAll(void);
    void rebuildStack(void);

  public:
    explicit DecoderTab(QWidget *parent = nullptr);
    ~DecoderTab();

    void setTerminationDecoder(Decoder *decoder);

    void setInputBps(uint8_t bps);
    uint8_t getOutputBps(void) const;

    void feed(const Symbol *samples, size_t len);
    bool isReady(void) const;
    bool isEnabled(void) const;

  public slots:
    void onAddDecoder(void);
    void onMoveDecoder(int, int);
    void onRemoveDecoder(int);
    void onToggleDecoder(void);

  signals:
    void toggled(void);
    void changed(void);

  private:
    Ui::DecoderTab *ui;
  };
}

#endif // DECODERTAB_H
