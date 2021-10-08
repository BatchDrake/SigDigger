//
//    TLESourceTab.h: TLE source tab
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
#ifndef TLESOURCETAB_H
#define TLESOURCETAB_H

#include <QWidget>
#include <ConfigTab.h>
#include <TLESourceConfig.h>
#include <AddTLESourceDialog.h>

#if HAVE_CURL
#include <TLEDownloaderTask.h>
#endif // HAVE_CURL

namespace Ui {
  class TLESourceTab;
}

namespace SigDigger {
  class TLESourceTab : public ConfigTab
  {
    Q_OBJECT

    AddTLESourceDialog *addDialog = nullptr;
    TLESourceConfig tleSourceConfig;
    bool modified = false;

    bool downloading = false;

#if HAVE_CURL
#endif // HAVE_CURL

    void populateTLESourceTable(void);
    void refreshUi(void);
    void connectAll(void);

  public:
    void save(void) override;
    bool hasChanged(void) const override;
    explicit TLESourceTab(QWidget *parent = nullptr);
    ~TLESourceTab() override;
    void setTleSourceConfig(const TLESourceConfig &config);
    TLESourceConfig getTleSourceConfig() const;

  public slots:
    void onConfigChanged(void);
    void onAddTLESource(void);
    void onRemoveTLESource(void);
    void onTLESelectionChanged(void);

  private:
    Ui::TLESourceTab *ui;
  };

};

#endif // TLESOURCETAB_H
