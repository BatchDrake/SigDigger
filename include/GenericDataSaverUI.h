//
//    GenericDataSaverUI.h: Base class for data saver widgets
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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

#ifndef GENERICDATASAVERUI_H
#define GENERICDATASAVERUI_H

#include <PersistentWidget.h>

//
// TODO: How about doing the same with the network forwarder?
//
namespace SigDigger {
  class GenericDataSaverUI : public PersistentWidget
  {
    Q_OBJECT

  protected:
    virtual void setDiskUsage(qreal) = 0;

  public:
    explicit GenericDataSaverUI(QWidget *parent = nullptr);
    virtual ~GenericDataSaverUI();

    void refreshDiskUsage(void);

    // Setters
    virtual void setRecordSavePath(std::string const &) = 0;
    virtual void setSaveEnabled(bool enabled) = 0;
    virtual void setCaptureSize(quint64) = 0;
    virtual void setIORate(qreal) = 0;
    virtual void setRecordState(bool state) = 0;

    // Getters
    virtual bool getRecordState(void) const = 0;
    virtual std::string getRecordSavePath(void) const = 0;

  signals:
    void recordSavePathChanged(QString);
    void recordStateChanged(bool state);

  };
}

#endif // GENERICDATASAVERUI_H
