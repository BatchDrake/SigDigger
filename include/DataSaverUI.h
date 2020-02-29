//
//    DataSaverUI.h: Data saver user interface
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

#ifndef DATASAVERUI_H
#define DATASAVERUI_H

#include <GenericDataSaverUI.h>

namespace Ui {
  class DataSaverUI;
}

namespace SigDigger {
  class DataSaverConfig : public Suscan::Serializable {
  public:
    std::string path;

    // Overriden methods
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize(void) override;
  };

  class DataSaverUI : public GenericDataSaverUI
  {
      Q_OBJECT
    DataSaverConfig *config = nullptr;
      void connectAll(void);

  protected:
      void setDiskUsage(qreal) override;

  public:
      // Setters
      void setRecordSavePath(std::string const &) override;
      void setSaveEnabled(bool enabled) override;
      void setCaptureSize(quint64) override;
      void setIORate(qreal) override;
      void setRecordState(bool state) override;

      // Getters
      bool getRecordState(void) const override;
      std::string getRecordSavePath(void) const override;

      // Other overriden methods
      Suscan::Serializable *allocConfig(void) override;
      void applyConfig(void) override;

      explicit DataSaverUI(QWidget *parent = nullptr);
      ~DataSaverUI() override;

  public slots:
      void onChangeSavePath(void);
      void onRecordStartStop(void);

  private:
      Ui::DataSaverUI *ui;
  };
}

#endif // DATASAVERUI_H
