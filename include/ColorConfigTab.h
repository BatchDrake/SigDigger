//
//    ColorConfigTab.h: configure SigDigger colors
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
#ifndef COLORCONFIGTAB_H
#define COLORCONFIGTAB_H

#include <ConfigTab.h>
#include <ColorConfig.h>

namespace Ui {
  class ColorConfigTab;
}

namespace SigDigger {
  class ColorConfigTab : public ConfigTab
  {
    Q_OBJECT

    ColorConfig colors;
    bool modified = false;

    void refreshUi();
    void connectAll();

  public:
    bool hasChanged() const override;
    void save() override;

    void setColorConfig(const ColorConfig &config);
    ColorConfig getColorConfig() const;

    explicit ColorConfigTab(QWidget *parent = nullptr);
    ~ColorConfigTab();

  public slots:
    void onColorChanged();

  private:
    Ui::ColorConfigTab *ui;
  };
}

#endif // COLORCONFIGTAB_H
