//
//    RMSInspector.h: description
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
#ifndef RMSINSPECTOR_H
#define RMSINSPECTOR_H

#include <QWidget>

#include <QWidget>
#include <Suscan/Analyzer.h>
#include <Suscan/Config.h>

#include <InspectionWidgetFactory.h>

namespace Ui {
  class RMSInspector;
}

namespace SigDigger {
  class AppConfig;
  class RMSViewTab;

  struct RMSInspectorConfig : public Suscan::Serializable {
    unsigned integrate = 1;
    bool dBscale = true;
    bool autoFit = true;
    bool autoScroll = true;

    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize() override;
  };

  class RMSInspector : public InspectionWidget
  {
    Q_OBJECT

    // Config
    RMSInspectorConfig *m_uiConfig = nullptr;

    // RMSViewTab
    RMSViewTab *m_rmsTab = nullptr;

    qint64 m_tunerFreq = 0;

    QString getInspectorTabTitle() const;

    void connectAll();

  public:
      void attachAnalyzer(Suscan::Analyzer *) override;
      void detachAnalyzer() override;

      void setProfile(Suscan::Source::Config &) override;
      void setTimeStamp(struct timeval const &) override;
      void setQth(Suscan::Location const &) override;

      void inspectorMessage(Suscan::InspectorMessage const &) override;
      void samplesMessage(Suscan::SamplesMessage const &) override;

      Suscan::Serializable *allocConfig() override;
      void applyConfig() override;

      void showEvent(QShowEvent *event) override;
      void floatStart() override;
      void floatEnd() override;

      std::string getLabel() const override;

      explicit RMSInspector(
          InspectionWidgetFactory *factory,
          Suscan::AnalyzerRequest const &request,
          UIMediator *mediator,
          QWidget *parent);

      ~RMSInspector() override;

  private:
    Ui::RMSInspector *ui;
  };
}

#endif // RMSINSPECTOR_H
