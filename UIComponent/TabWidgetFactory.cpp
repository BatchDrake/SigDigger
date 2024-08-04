//
//    TabWidgetFactory.cpp: Factory for tab widgets
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
#include "TabWidgetFactory.h"
#include <Suscan/Library.h>
#include <UIMediator.h>

#include <QInputDialog>
#include <QMenu>
#include <QAction>

using namespace SigDigger;

TabWidget::TabWidget(
    TabWidgetFactory *factory, UIMediator *mediator, QWidget *parent) :
  QWidget(parent), UIComponent(factory, mediator)
{
  m_menu = new QMenu(this);
  m_renameTab = new QAction("&Rename...", this);
  m_floatTab = new QAction("&Detach to window", this);
  m_closeTab = new QAction("&Close", this);

  m_renameTab->setShortcut(QString("F2"));
  m_renameTab->setShortcutContext(Qt::WidgetWithChildrenShortcut);

  m_closeTab->setShortcut(QString("CTRL+W"));
  m_closeTab->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  QWidget::addAction(m_closeTab);

  m_floatTab->setShortcut(QString("CTRL+SHIFT+W"));
  m_floatTab->setShortcutContext(Qt::WidgetWithChildrenShortcut);

  m_menu->addAction(m_renameTab);
  QWidget::addAction(m_renameTab);

  m_menu->addAction(m_floatTab);
  QWidget::addAction(m_floatTab);

  m_menu->addSeparator();
  m_menu->addAction(m_closeTab);

  connect(
        m_renameTab,
        SIGNAL(triggered()),
        this,
        SLOT(onRename()));

  connect(
        m_closeTab,
        SIGNAL(triggered()),
        this,
        SLOT(onPopupMenuClose()));

  connect(
        m_floatTab,
        SIGNAL(triggered()),
        this,
        SLOT(onFloat()));
}

QString
TabWidget::getCurrentLabel() const
{
  if (!m_labelChanged)
    return QString::fromStdString(getLabel());

  return m_cachedLabel;
}

void
TabWidget::addAction(QAction *action)
{
  m_customActions.push_back(action);
  m_menu->insertAction(m_renameTab, action);

  QWidget::addAction(action);
}

void
TabWidget::addSeparator()
{
  m_customActions.push_back(nullptr);
  m_menu->insertSeparator(m_renameTab);
}

bool
TabWidget::hasCustomActions() const
{
  return m_customActions.size() > 0;
}

void
TabWidget::addCustomActionsToMenu(QMenu *menu)
{
  for (auto action : m_customActions) {
    if (action == nullptr)
      menu->addSeparator();
    else
      menu->addAction(action);
  }
}

void
TabWidget::addCommonActionsToMenu(QMenu *menu)
{
  menu->addAction(m_renameTab);
  menu->addSeparator();
  menu->addAction(m_closeTab);
}

void
TabWidget::addActionsToParent(QWidget *widget)
{
  m_renameTab->setShortcutContext(Qt::ApplicationShortcut);
  m_floatTab->setShortcutContext(Qt::ApplicationShortcut);

  widget->addAction(m_renameTab);
  widget->addAction(m_closeTab);

  for (auto action : m_customActions) {
    if (action != nullptr) {
      widget->addAction(action);
            action->setShortcutContext(Qt::ApplicationShortcut);
    }
  }
}


TabWidget::~TabWidget()
{
  if (m_mediator != nullptr)
    (void) m_mediator->closeTabWidget(this);
}

void
TabWidget::popupMenu()
{
  m_menu->popup(QCursor::pos());
}

void
TabWidget::floatStart()
{
  // NO-OP
}

void
TabWidget::floatEnd()
{
  // NO-OP
}

void
TabWidget::closeRequested()
{
  // Mark it for removal. Its destruction will triger a bunch of things.
  this->deleteLater();
}

void
TabWidget::onPopupMenuClose()
{
  this->closeRequested();
}

void
TabWidget::onRename()
{
  bool ok;

  if (!m_labelChanged) {
    m_cachedLabel  = QString::fromStdString(this->getLabel());
    m_labelChanged = true;
  }

  QString text = QInputDialog::getText(
        this,
        "Change tab name",
        "New label: ",
        QLineEdit::Normal,
        m_cachedLabel,
        &ok);

  if (ok && !text.isEmpty()) {
    m_cachedLabel = text;
    emit nameChanged(m_cachedLabel);
  }
}

void
TabWidget::onFloat()
{
  m_mediator->floatTabWidget(this);
}

bool
TabWidgetFactory::registerGlobally(void)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->registerTabWidgetFactory(this);
}

bool
TabWidgetFactory::unregisterGlobally(void)
{
  Suscan::Singleton *s = Suscan::Singleton::get_instance();

  return s->unregisterTabWidgetFactory(this);
}

TabWidgetFactory::TabWidgetFactory(Suscan::Plugin *plugin)
  : UIComponentFactory(plugin)
{

}
