//
//    FloatingTabWindow.h: description
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
#ifndef FLOATINGTABWINDOW_H
#define FLOATINGTABWINDOW_H

#include <QMainWindow>
#include <TabWidgetFactory.h>
#include <QCloseEvent>
#include <QVBoxLayout>

class QMenu;

namespace Ui {
  class FloatingTabWindow;
}

namespace SigDigger {
  class FloatingTabWindow : public QMainWindow
  {
    Q_OBJECT

    TabWidget *m_tabWidget = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QMenu *m_customMenu = nullptr;

    void connectAll();

  public:
    void closeEvent(QCloseEvent *) override;

    TabWidget *getTabWidget() const;
    TabWidget *takeTabWidget();

    explicit FloatingTabWindow(TabWidget *widget, QWidget *parent = nullptr);

    ~FloatingTabWindow();

  signals:
    void finished();
    void reattach();

  public slots:
    void onRename(QString);
    void onClose();
    void onChildDestroyed();

  private:
    Ui::FloatingTabWindow *ui;
  };

}

#endif // FLOATINGTABWINDOW_H
