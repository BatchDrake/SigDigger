//
//    ../include/InspectionWidgetFactory.h: description
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
#ifndef INSPECTIONWIDGETFACTORY_H
#define INSPECTIONWIDGETFACTORY_H

#include <TabWidgetFactory.h>
#include <Suscan/AnalyzerRequestTracker.h>
#include <WFHelpers.h>

class QColorDialog;

namespace SigDigger {
  class InspectionWidgetFactory;
  class InspectionWidget : public TabWidget {
    Q_OBJECT

    QColorDialog           *m_colorDialog = nullptr;

  protected:
    Suscan::AnalyzerRequest m_request;
    Suscan::Config          m_config;
    int                     m_state = 0;
    Suscan::Analyzer       *m_analyzer = nullptr;
    bool                    m_onceAttached = false;
    NamedChannelSetIterator m_namedChannel;
    bool                    m_haveNamedChannel = false;

    int               state() const;
    Suscan::Analyzer *analyzer() const;
    NamedChannelSetIterator &namedChannel();
    void refreshNamedChannel();

  public:
    Suscan::AnalyzerRequest const &request() const;
    Suscan::Config const &config() const;

    InspectionWidget(
        InspectionWidgetFactory *,
        Suscan::AnalyzerRequest const &,
        UIMediator *,
        QWidget *parent = nullptr);
    ~InspectionWidget();

    // Overridable methods
    virtual void attachAnalyzer(Suscan::Analyzer *);
    virtual void detachAnalyzer();
    virtual void inspectorMessage(Suscan::InspectorMessage const &);
    virtual void samplesMessage(Suscan::SamplesMessage const &);

    // Overriden methods
    virtual void setState(int, Suscan::Analyzer *) override;
    virtual void closeRequested() override;

    public slots:
      void onNameChanged(QString name);
      void onRequestChangeColor();
      void onColorSelected(const QColor &);
  };

  class InspectionWidgetFactory : public TabWidgetFactory
  {
  public:
    virtual const char *description() const;

    virtual TabWidget *make(UIMediator *) override; // This one should fail.
    virtual InspectionWidget *make(
        Suscan::AnalyzerRequest const &,
        UIMediator *) = 0;

    // Overriden methods
    bool registerGlobally() override;
    bool unregisterGlobally() override;

    InspectionWidgetFactory(Suscan::Plugin *);
  };
}

#endif // INSPECTIONWIDGETFACTORY_H
