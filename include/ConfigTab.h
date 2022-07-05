//
//    ConfigTab.h: Base class for configuration tabs
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
#ifndef SETTINGS_CONFIGTAB_H
#define SETTINGS_CONFIGTAB_H

#include <QWidget>

namespace SigDigger {
  class ConfigTab : public QWidget {
    Q_OBJECT

    QString tabName;

  public:
    explicit ConfigTab(QWidget *parent = nullptr, QString name = "");
    ~ConfigTab();

    virtual QString getName(void) const;
    virtual bool hasChanged(void) const;
    virtual void save(void);

  signals:
    void changed(void);
  };
}

#endif // SETTINGS_CONFIGTAB_H
