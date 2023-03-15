//
//    FloatingTabWindow.cpp: description
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
#include "FloatingTabWindow.h"
#include "ui_FloatingTabWindow.h"

using namespace SigDigger;

FloatingTabWindow::FloatingTabWindow(TabWidget *widget, QWidget *parent) :
  QMainWindow(parent),
  m_tabWidget(widget),
  ui(new Ui::FloatingTabWindow)
{
  ui->setupUi(this);

  //m_layout = new QVBoxLayout;

  //setLayout(m_layout);

  //m_layout->addWidget(m_tabWidget);
  //m_tabWidget->setParent(this);

  setCentralWidget(m_tabWidget);

  if (m_tabWidget->hasCustomActions()) {
    m_customMenu = new QMenu("&Actions", this);
    ui->menubar->addMenu(m_customMenu);
    m_tabWidget->addCustomActionsToMenu(m_customMenu);
  }

  setWindowTitle("SigDigger - " + m_tabWidget->getCurrentLabel());

  connectAll();
}

TabWidget *
FloatingTabWindow::getTabWidget() const
{
  return m_tabWidget;
}

TabWidget *
FloatingTabWindow::takeTabWidget()
{
  TabWidget *widget = m_tabWidget;

  m_tabWidget->setParent(nullptr);

  m_tabWidget = nullptr;

  return widget;
}

void
FloatingTabWindow::closeEvent(QCloseEvent *event)
{
  event->ignore();

  hide();

  emit finished();
}

void
FloatingTabWindow::connectAll()
{
  connect(
        ui->action_Rename,
        SIGNAL(triggered(bool)),
        m_tabWidget,
        SLOT(onRename()));

  connect(
        ui->action_Close,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onClose()));

  connect(
        m_tabWidget,
        SIGNAL(nameChanged(QString)),
        this,
        SLOT(onRename(QString)));

  connect(
        ui->actionRe_attach,
        SIGNAL(triggered(bool)),
        this,
        SIGNAL(reattach()));
}

FloatingTabWindow::~FloatingTabWindow()
{
  delete ui;
}

void
FloatingTabWindow::onRename(QString name)
{
  setWindowTitle(
        "SigDigger - "
        + name);
}

void
FloatingTabWindow::onClose()
{
  hide();

  emit finished();
}
