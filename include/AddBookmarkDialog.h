//
//    AddBookmarkDialog.h: Description
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
#ifndef ADDBOOKMARKDIALOG_H
#define ADDBOOKMARKDIALOG_H

#include <QDialog>

namespace Ui {
  class AddBookmarkDialog;
}

namespace SigDigger {
  class AddBookmarkDialog : public QDialog
  {
      Q_OBJECT

    public:
      explicit AddBookmarkDialog(QWidget *parent = nullptr);
      virtual ~AddBookmarkDialog() override;

      void setFrequencyHint(qint64 val);
      void setNameHint(QString const &name);
      void setColorHint(QColor const &);
      void setBandwidthHint(qint32 val);
      void setModulationHint(QString const &name);

      qint64 frequency(void) const;
      QString name(void) const;
      QColor color(void) const;
      qint32 bandwidth(void) const;
      QString modulation(void) const;

      virtual void showEvent(QShowEvent *event) override;

    private:
      Ui::AddBookmarkDialog *ui;
  };
}

#endif // ADDBOOKMARKDIALOG_H
