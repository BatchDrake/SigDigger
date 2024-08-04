//
//    GenericInspector.h: Generic inspector widget
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

#ifndef GENERICINSPECTOR_H
#define GENERICINSPECTOR_H

#include <QWidget>
#include <Suscan/Analyzer.h>
#include <Suscan/Config.h>
#include "InspectorUI.h"

#include <InspectionWidgetFactory.h>

namespace SigDigger {
  class AppConfig;

  struct GenericInspectorConfig : public Suscan::Serializable {
    std::string  spectrumPalette   = "Inferno (Feely)";
    float        spectrumRatio     = .3f;
    std::string  waveFormPalette   = "Inferno (Feely)";
    unsigned int waveFormOffset    = 0;
    int          waveFormContrast  = 1;
    bool         peakHold          = false;
    bool         peakDetect        = false;
    std::string  units             = "dBFS";
    float        gain              = 0;
    float        zeroPoint         = 0;

    void deserialize(Suscan::Object const &conf) override;
    Suscan::Object &&serialize() override;
  };

  class GenericInspector : public InspectionWidget
  {
      Q_OBJECT

      // Config
      GenericInspectorConfig *m_uiConfig = nullptr;

      // Inspector UI
      InspectorUI *ui = nullptr;
      uint32_t lastSpectrumId = 0;
      bool adjusted = false;
      qint64 m_tunerFreq;

      QString getInspectorTabTitle() const;

      void feed(const SUCOMPLEX *data, unsigned int size);
      void feedSpectrum(
          const SUFLOAT *data,
          SUSCOUNT len,
          SUSCOUNT rate,
          uint32_t id);
      void updateEstimator(Suscan::EstimatorId id, float val);
      void notifyOrbitReport(Suscan::OrbitReport const &);
      void disableCorrection(void);
      void setTunerFrequency(SUFREQ freq);
      void setRealTime(bool);
      void setTimeLimits(
          struct timeval const &start,
          struct timeval const &end);

  public:
      void attachAnalyzer(Suscan::Analyzer *) override;
      void detachAnalyzer() override;

      void setProfile(Suscan::Source::Config &) override;
      void setTimeStamp(struct timeval const &) override;
      void setQth(Suscan::Location const &) override;

      void inspectorMessage(Suscan::InspectorMessage const &) override;
      void samplesMessage(Suscan::SamplesMessage const &) override;

      Suscan::Serializable *allocConfig(void) override;
      void applyConfig(void) override;

      void showEvent(QShowEvent *event) override;
      void floatStart() override;
      void floatEnd() override;

      std::string getLabel() const override;

      explicit GenericInspector(
          InspectionWidgetFactory *factory,
          Suscan::AnalyzerRequest const &request,
          UIMediator *mediator,
          QWidget *parent);

      ~GenericInspector() override;

    public slots:
      // UI slots
      void onConfigChanged(void);
      void onSetSpectrumSource(unsigned int index);
      void onLoChanged(void);
      void onBandwidthChanged(void);
      void onToggleEstimator(Suscan::EstimatorId, bool);
      void onApplyEstimation(QString, float);
      void onDopplerCorrection(Suscan::Orbit);
      void onDisableCorrection(void);
      void onOpenInspector(
          QString inspClass,
          qint64 freq,
          qreal bw,
          bool precise);

      // Analyzer slots
      void onSourceInfoMessage(Suscan::SourceInfoMessage const &);
  };
}

#endif // GENERICINSPECTOR_H
