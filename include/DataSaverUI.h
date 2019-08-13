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

#include <QWidget>

namespace Ui {
  class DataSaverUI;
}

namespace SigDigger {
  class DataSaverUI : public QWidget
  {
      Q_OBJECT

      void connectAll(void);
      void setDiskUsage(qreal);

    public:
      void refreshDiskUsage(void);

      // Setters
      void setRecordSavePath(std::string const &);
      void setSaveEnabled(bool enabled);
      void setCaptureSize(quint64);
      void setIORate(qreal);
      void setRecordState(bool state);

      // Getters
      bool getRecordState(void) const;
      std::string getRecordSavePath(void) const;

      explicit DataSaverUI(QWidget *parent = nullptr);
      ~DataSaverUI();

    public slots:
      void onChangeSavePath(void);
      void onRecordStartStop(void);

    signals:
      void recordSavePathChanged(QString);
      void recordStateChanged(bool state);

    private:
      Ui::DataSaverUI *ui;
  };
}

#endif // DATASAVERUI_H
