#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QWidget>
#include <Suscan/Analyzer.h>
#include <Suscan/Config.h>
#include <InspectorUI.h>
#include <Suscan/Messages/InspectorMessage.h>

namespace SigDigger {
  class AppConfig;

  class Inspector : public QWidget
  {
      Q_OBJECT

      // Inspector config
      Suscan::Config config;

      // Inspector UI
      std::unique_ptr<InspectorUI> ui;

      // Handle of the inspector this object refers to
      Suscan::Handle handle;
      Suscan::InspectorId id;
      Suscan::Analyzer *analyzer = nullptr;
      bool adjusted = false;

    public:
      Suscan::InspectorId
      getId(void) const
      {
        return this->id;
      }

      void
      setId(Suscan::InspectorId id)
      {
        this->id = id;
      }

      Suscan::Analyzer *
      getAnalyzer(void) const
      {
        return this->analyzer;
      }

      Suscan::Handle
      getHandle(void) const
      {
        return this->handle;
      }

      void setAnalyzer(Suscan::Analyzer *analyzer);
      void feed(const SUCOMPLEX *data, unsigned int size);
      void feedSpectrum(const SUFLOAT *data, SUSCOUNT len, SUSCOUNT rate);
      void updateEstimator(Suscan::EstimatorId id, float val);
      void notifyOrbitReport(Suscan::OrbitReport const &);
      void disableCorrection(void);
      void setTunerFrequency(SUFREQ freq);
      void setRealTime(bool);
      void setTimeLimits(
          struct timeval const &start,
          struct timeval const &end);
      void setTimeStamp(struct timeval const &);
      void setQth(xyz_t const &qth);

      void showEvent(QShowEvent *event);

      explicit Inspector(
          QWidget *parent,
          const Suscan::InspectorMessage &msg,
          AppConfig const &config);

      ~Inspector();

    public slots:
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
  };
}

#endif // INSPECTOR_H
