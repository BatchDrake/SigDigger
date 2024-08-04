//
//    GuiConfigTab.h: GuiConfigTab.h
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

#include <ConfigTab.h>
#include <GuiConfig.h>

namespace Ui {
  class GuiConfigTab;
}

namespace SigDigger {
  class GuiConfigTab : public ConfigTab
  {
    Q_OBJECT

    GuiConfig guiConfig;
    bool modified = false;
    void refreshUi();
    void connectAll(void);

  public:
    void save(void) override;
    bool hasChanged(void) const override;
    void setGuiConfig(const GuiConfig &config);
    GuiConfig getGuiConfig() const;

    explicit GuiConfigTab(QWidget *parent = nullptr);
    ~GuiConfigTab() override;

  public slots:
    void onConfigChanged(void);

  private:
    Ui::GuiConfigTab *ui;
  };
}

#endif // GUICONFIGTAB_H
