//
//    RMSInspector.cpp: description
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
#include "RMSInspector.h"
#include "RMSViewTab.h"
#include "ui_RMSInspector.h"
#include "SuWidgetsHelpers.h"
#include "UIMediator.h"

using namespace SigDigger;

#define STRINGFY(x) #x
#define STORE(field) obj.set(STRINGFY(field), this->field)
#define LOAD(field) this->field = conf.get(STRINGFY(field), this->field)

void
RMSInspectorConfig::deserialize(Suscan::Object const &conf)
{
  LOAD(integrate);
  LOAD(dBscale);
  LOAD(autoFit);
  LOAD(autoScroll);
}

Suscan::Object &&
RMSInspectorConfig::serialize(void)
{
  Suscan::Object obj(SUSCAN_OBJECT_TYPE_OBJECT);

  obj.setClass("RMSInspectorConfig");

  STORE(integrate);
  STORE(dBscale);
  STORE(autoFit);
  STORE(autoScroll);

  return this->persist(obj);
}

QString
RMSInspector::getInspectorTabTitle() const
{

  QString result = " for "
      + SuWidgetsHelpers::formatQuantity(
        this->request().channel.fc + this->mediator()->getCurrentCenterFreq(),
        "Hz");

  return "RMS plot" + result;
}

void
RMSInspector::connectAll()
{

}

RMSInspector::RMSInspector(
    InspectionWidgetFactory *factory,
    Suscan::AnalyzerRequest const &request,
    UIMediator *mediator,
    QWidget *parent) :
  InspectionWidget(factory, request, mediator, parent),
  ui(new Ui::RMSInspector())
{
  ui->setupUi(this);

  m_rmsTab = new RMSViewTab(nullptr, nullptr);

  ui->tabWidget->insertTab(0, m_rmsTab, "Power plot");
  ui->tabWidget->setCurrentIndex(0);
  connectAll();
}


RMSInspector::~RMSInspector()
{
  delete ui;
}


/////////////////////////// Overriden methods /////////////////////////////////
void
RMSInspector::attachAnalyzer(Suscan::Analyzer *)
{

}

void
RMSInspector::detachAnalyzer()
{

}

void
RMSInspector::setProfile(Suscan::Source::Config &)
{

}

void
RMSInspector::setTimeStamp(struct timeval const &)
{
}

void
RMSInspector::setQth(Suscan::Location const &)
{

}

void
RMSInspector::inspectorMessage(Suscan::InspectorMessage const &)
{

}

void
RMSInspector::samplesMessage(Suscan::SamplesMessage const &)
{

}

Suscan::Serializable *
RMSInspector::allocConfig()
{
  m_uiConfig = new RMSInspectorConfig();

  return m_uiConfig;
}

void
RMSInspector::applyConfig()
{
  // TODO
}


void
RMSInspector::showEvent(QShowEvent *)
{

}

void
RMSInspector::floatStart()
{

}

void
RMSInspector::floatEnd()
{

}


std::string
RMSInspector::getLabel() const
{
  return this->getInspectorTabTitle().toStdString();
}
