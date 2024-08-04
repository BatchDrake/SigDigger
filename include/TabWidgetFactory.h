//
//    TabWidgetFactory.h: Make tab widget
//    Copyright (C) 2022 Gonzalo José Carracedo Carballal
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
#ifndef TABWIDGETFACTORY_H
#define TABWIDGETFACTORY_H

#include <UIComponentFactory.h>
#include <QList>

class QMenu;

namespace SigDigger {
  class UIMediator;
  class TabWidgetFactory;
  class UIListener;

  class TabWidget : public QWidget, public UIComponent {
    Q_OBJECT

    QMenu   *m_menu = nullptr;
    QAction *m_closeTab = nullptr;
    QAction *m_renameTab = nullptr;
    QAction *m_floatTab = nullptr;

    QList<QAction *> m_customActions;

    bool     m_labelChanged = false;

    QString  m_cachedLabel;

  protected:
    void addAction(QAction *);
    void addSeparator();

    TabWidget(TabWidgetFactory *, UIMediator *, QWidget *parent = nullptr);

  public:
    virtual std::string getLabel() const = 0;
    virtual void floatStart();
    virtual void floatEnd();
    virtual void closeRequested();

    QString getCurrentLabel() const;
    bool hasCustomActions() const;
    void addCustomActionsToMenu(QMenu *);
    void addCommonActionsToMenu(QMenu *);
    void addActionsToParent(QWidget *);

    void popupMenu();

    ~TabWidget() override;

  signals:
    void nameChanged(QString);

  public slots:
    void onPopupMenuClose();
    void onRename();
    void onFloat();
  };

  class TabWidgetFactory : public UIComponentFactory {
  public:
    virtual TabWidget *make(UIMediator *) = 0;

    // Overriden methods
    bool registerGlobally(void) override;
    bool unregisterGlobally(void) override;

    TabWidgetFactory(Suscan::Plugin *);
  };
}

#endif // TABWIDGETFACTORY_H
