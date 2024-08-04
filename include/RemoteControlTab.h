//
//    RemoteControlTab.h: Remote Control tab
//    Copyright (C) 2023 Gonzalo José Carracedo Carballal
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
#ifndef REMOTECONTROLTAB_H
#define REMOTECONTROLTAB_H

#include <ConfigTab.h>
#include <RemoteControlConfig.h>

namespace Ui {
  class RemoteControlTab;
}

namespace SigDigger {
  class RemoteControlTab : public ConfigTab
  {
    Q_OBJECT

    RemoteControlConfig rcConfig;
    bool modified = false;
    void refreshUi();
    void connectAll(void);

  public:
    void save(void) override;
    bool hasChanged(void) const override;
    void setRemoteControlConfig(const RemoteControlConfig &config);
    RemoteControlConfig getRemoteControlConfig() const;

    explicit RemoteControlTab(QWidget *parent = nullptr);
    ~RemoteControlTab();

  private:
    Ui::RemoteControlTab *ui;

  public slots:
    void onConfigChanged(void);
  };
}

#endif // REMOTECONTROLTAB_H
