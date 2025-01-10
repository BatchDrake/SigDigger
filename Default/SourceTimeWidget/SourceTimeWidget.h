//
//    SourceTimeWidget.h: Display source time
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

#ifndef SOURCETIMEWIDGET_H
#define SOURCETIMEWIDGET_H

#include <QWidget>
#include <ToolBarWidgetFactory.h>

namespace Ui {
  class SourceTimeWidget;
}

namespace SigDigger {
  class SourceTimeWidgetFactory;

  class SourceTimeWidget : public ToolBarWidget
  {
    Q_OBJECT

    bool m_utc = false;
    QColor m_lcdFg;
    QColor m_lcdBg;
    QColor m_lcdFgStopped;
    struct timeval m_ts;

    void drawTimeStamp();

  public:
    explicit SourceTimeWidget(
        SourceTimeWidgetFactory *,
        UIMediator *,
        QWidget *parent);

    virtual void setColorConfig(ColorConfig const &config) override;
    virtual void setTimeStamp(struct timeval const &) override;
    virtual void setState(int, Suscan::Analyzer *) override;

    virtual ~SourceTimeWidget() override;

    void setColors(QColor const &fg, QColor const &bg);
    void setUTC(bool);

  private:
    Ui::SourceTimeWidget *ui;
  };

  class SourceTimeWidgetFactory : public ToolBarWidgetFactory {
  public:
    using ToolBarWidgetFactory::ToolBarWidgetFactory;

    virtual const char *name() const override;
    virtual const char *desc() const override;

    SourceTimeWidget *make(UIMediator *);
  };
}

#endif // SOURCETIMEWIDGET_H
