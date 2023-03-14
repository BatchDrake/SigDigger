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
#include <suscan/cli/datasaver.h>
#include <InspectionWidgetFactory.h>

#define RMS_INSPECTOR_DEFAULT_INTEGRATION_TIME_MS 20

namespace Ui {
  class RMSInspector;
}

namespace SigDigger {
  class AppConfig;
  class RMSViewTab;

  extern "C" {
    typedef void (*datasaver_param_init_cb) (
      struct suscli_datasaver_params *self,
      const hashlist_t *params);
  }

  struct RMSInspectorConfig : public Suscan::Serializable {
    unsigned integrate = 1;
    float integrationTime = RMS_INSPECTOR_DEFAULT_INTEGRATION_TIME_MS * 1e-3;
    bool dBscale = true;
    bool autoFit = true;
    bool autoScroll = true;

    bool logData = false;
    std::string logDir = "";
    std::string logFormat = "csv";
    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize() override;
  };

  class RMSInspector : public InspectionWidget
  {
    Q_OBJECT

    Suscan::Analyzer *m_analyzer = nullptr;
    uint32_t m_lastSpectrumId;
    qreal m_sampleRate = 0;
    qreal m_kahanAcc = 0;
    qreal m_kahanC = 0;
    quint64 m_count = 0;
    quint64 m_maxSamples = 0;

    std::vector<suscli_datasaver_params> m_datasaverList;
    QString               m_dataFile;
    std::string           m_fullPathStd;
    hashlist_t           *m_datasaverParams = nullptr;
    suscli_datasaver     *m_datasaver = nullptr;
    struct timeval        m_t0;
    struct timeval        m_lastUpdate;
    std::vector<SUFLOAT>  m_fftData;
    SUSCOUNT              m_lastRate = 0;
    SUSCOUNT              m_lastLen = 0;
    unsigned int          m_spectrumAdjustCounter = 0;
    bool                  m_haveSpectrumLimits = false;

    // Config
    RMSInspectorConfig *m_uiConfig = nullptr;

    // RMSViewTab
    RMSViewTab *m_rmsTab = nullptr;

    qint64 m_tunerFreq = 0;

    QString getInspectorTabTitle() const;

    void updateMaxSamples();
    void checkMaxSamples();

    void feedSpectrum(
        const SUFLOAT *data,
        SUSCOUNT len,
        SUSCOUNT rate,
        uint32_t id);

    void registerDataSaver(
        QString const &desc,
        datasaver_param_init_cb);

    const suscli_datasaver_params *currentDataSaverParams();

    void connectAll();

  public:
      void attachAnalyzer(Suscan::Analyzer *) override;
      void detachAnalyzer() override;

      void setProfile(Suscan::Source::Config &) override;
      void setTimeStamp(struct timeval const &) override;
      void setQth(Suscan::Location const &) override;
      void setColorConfig(ColorConfig const &) override;
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

  public slots:
      void onToggleDataLogger();
      void onConfigChanged();
      void onTabChanged();
      void onRangeChanged(float min, float max);
      void onChangeLo();
      void onChangeBandwidth();
      void onBrowseDirectory();
      void onSourceInfoMessage(Suscan::SourceInfoMessage const &);
  private:
    Ui::RMSInspector *ui;
  };
}

#endif // RMSINSPECTOR_H
