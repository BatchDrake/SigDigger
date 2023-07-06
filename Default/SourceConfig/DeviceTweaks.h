//
//    DeviceTweaks.h: Tweak device-specific settings
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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
#ifndef DEVICETWEAKS_H
#define DEVICETWEAKS_H

#include <QDialog>
#include <Suscan/Source.h>

class QItemSelection;

namespace Ui {
  class DeviceTweaks;
}

namespace SigDigger {
  class DeviceTweaks : public QDialog
  {
    Q_OBJECT
    Suscan::Source::Config *profile = nullptr; // borrowed

    bool changed = false;
    void connectAll(void);
    void refreshUi(void);
    void setChanged(bool changed);

  public:
    explicit DeviceTweaks(QWidget *parent = nullptr);
    void setProfile(Suscan::Source::Config *profile);
    bool hasChanged(void) const;
    void commitConfig(void);
    ~DeviceTweaks();

  private:
    Ui::DeviceTweaks *ui;

  public slots:
    void onAddEntry(void);
    void onRemoveEntry(void);
    void onReset(void);
    void onSelectionChanged(const QItemSelection &, const QItemSelection &);
    void onChanged(void);
  };
}

#endif // DEVICETWEAKS_H
