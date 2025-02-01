//
//    SourceTimeWidget.cpp: Display source time
//    Copyright (C) 2024 Gonzalo José Carracedo Carballal
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

#include "SourceTimeWidget.h"
#include "ui_SourceTimeWidget.h"
#include <ColorConfig.h>
#include <Suscan/Library.h>

using namespace SigDigger;

#ifdef _WIN32
#  define localtime_r(a, b) localtime_s(b, a)
#  define gmtime_r(a, b)    gmtime_r(b, a)
#endif // _WIN32

////////////////////////////// SourceTimeWidget ////////////////////////////////
SourceTimeWidget::SourceTimeWidget(
    SourceTimeWidgetFactory *factory,
    UIMediator *mediator,
    QWidget *parent) :
    ToolBarWidget(factory, mediator, parent),
  ui(new Ui::SourceTimeWidget)
{
  ui->setupUi(this);

  gettimeofday(&m_ts, nullptr);
}

SourceTimeWidget::~SourceTimeWidget()
{
  delete ui;
}

void
SourceTimeWidget::drawTimeStamp()
{
  struct tm tm;

  if (m_utc)
    gmtime_r(&m_ts.tv_sec, &tm);
  else
    localtime_r(&m_ts.tv_sec, &tm);

  ui->hourLCD->setValue(tm.tm_hour);
  ui->minLCD->setValue(tm.tm_min);
  ui->secLCD->setValue(tm.tm_sec);

  ui->dayLCD->setValue(tm.tm_mday);
  ui->monthLCD->setValue(tm.tm_mon + 1);
  ui->yearLCD->setValue(tm.tm_year + 1900);
}

void
SourceTimeWidget::setUTC(bool utc)
{
  m_utc = utc;
  drawTimeStamp();
}

void
SourceTimeWidget::setColors(const QColor &fg, const QColor &bg)
{
  LCD *lcds[] = {
    ui->hourLCD, ui->minLCD, ui->secLCD,
    ui->dayLCD, ui->monthLCD, ui->yearLCD};

  for (auto i = 0; i < 6; ++i) {
    lcds[i]->setForegroundColor(fg);
    lcds[i]->setBackgroundColor(bg);
  }

  QString styleSheet =
      "background-color: " + bg.name() + ";\n"
      "color: " + fg.name() + ";\n"
      "font-weight: bold;\n";

  QLabel *labels[] = {
    ui->label, ui->label_1, ui->label_2,
    ui->label_3, ui->label_4, ui->label_5,
    ui->label_6
  };

  for (auto i = 0; i < 7; ++i)
    labels[i]->setStyleSheet(styleSheet);

  drawTimeStamp();
}

void
SourceTimeWidget::setColorConfig(ColorConfig const &config)
{
  m_lcdBg = config.lcdBackground;
  m_lcdFg = config.lcdForeground;
  m_lcdFgStopped = QColor::fromRgbF(
        .5 * (m_lcdFg.redF() + m_lcdBg.redF()),
        .5 * (m_lcdFg.greenF() + m_lcdBg.greenF()),
        .5 * (m_lcdFg.blueF() + m_lcdBg.blueF()));

  setColors(m_lcdFgStopped, m_lcdBg);
}

void
SourceTimeWidget::setTimeStamp(struct timeval const &tv)
{
  m_ts = tv;
  drawTimeStamp();
}

void
SourceTimeWidget::setState(int, Suscan::Analyzer *analyzer)
{
  setColors(analyzer == nullptr ? m_lcdFgStopped : m_lcdFg, m_lcdBg);
}

////////////////////// SourceTimeWidgetFactory ////////////////////////////////
const char *
SourceTimeWidgetFactory::name() const
{
  return "SourceTimeWidget";
}

const char *
SourceTimeWidgetFactory::desc() const
{
  return "Current source time";
}

SourceTimeWidget *
SourceTimeWidgetFactory::make(UIMediator *mediator)
{
  return new SourceTimeWidget(this, mediator, nullptr);
}
