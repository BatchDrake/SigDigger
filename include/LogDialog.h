//
//    LogDialog.h: Display log messages
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
#ifndef LOGDIALOG_H
#define LOGDIALOG_H

#include <QDialog>
#include <Suscan/Logger.h>
#include <QVector>

namespace Ui {
  class LogDialog;
}

class QTableWidgetItem;

namespace SigDigger {
  class LogDialog : public QDialog
  {
      Q_OBJECT

      QVector<Suscan::LoggerMessage> m_msgVec;
      bool                           m_errorFound = false;
      Suscan::Logger                *m_logger;
      unsigned int                   m_repeat = 1;

      bool isRepeated(std::string const &msg) const;

      void connectAll(void);
      void saveLog(QString path);
      static QTableWidgetItem *makeSeverityItem(
          enum sigutils_log_severity);

    public:
      bool haveErrorMessages(void) const
      {
        return this->m_errorFound;
      }

      QString getErrorHtml(void) const;

      explicit LogDialog(QWidget *parent = nullptr);
      ~LogDialog();

    public slots:
      void onMessage(Suscan::LoggerMessage);
      void onClear(void);
      void onSave(void);

    private:
      Ui::LogDialog *ui;
  };
}

#endif // LOGDIALOG_H
