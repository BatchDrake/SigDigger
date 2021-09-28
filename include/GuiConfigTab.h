//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
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
#ifndef GUICONFIGTAB_H
#define GUICONFIGTAB_H

#include <QWidget>
#include <GuiConfig.h>

namespace Ui {
  class GuiConfigTab;
}

namespace SigDigger {
  class GuiConfigTab : public QWidget
  {
    Q_OBJECT

    GuiConfig guiConfig;

    void refreshGuiConfigUi();
    void connectAll(void);
    void saveGuiConfigUi(void);

  public:
    void setGuiConfig(const GuiConfig &config);
    GuiConfig getGuiConfig();

    explicit GuiConfigTab(QWidget *parent = nullptr);
    ~GuiConfigTab();

  private:
    Ui::GuiConfigTab *ui;
  };
}

#endif // GUICONFIGTAB_H
