#ifndef TVPROCESSORTAB_H
#define TVPROCESSORTAB_H

#include <QWidget>
#include "TVProcessorWorker.h"
#include <SuWidgets/Decider.h>

namespace Ui {
  class TVProcessorTab;
}

namespace SigDigger {
  class TVProcessorTab : public QWidget
  {
    Q_OBJECT

    qreal sampleRate = 0;
    bool tvProcessing = false;
    bool editingTVProcessorParams = false;

    Decider::DecisionMode decisionMode = Decider::MODULUS;

    TVProcessorWorker *tvWorker = nullptr;
    QThread *tvThread = nullptr;
    std::vector<SUFLOAT> floatBuffer;

    void connectAll(void);
    void emitParameters(void);
    void refreshUiState(void);
    void refreshUi(
        struct sigutils_tv_processor_params const &params);
    bool parseUi(
        struct sigutils_tv_processor_params &params);
    SUFLOAT getSampleRateFloat(void) const;
    unsigned int getSampleRate(void) const;

  public:
    bool
    isEnabled(void) const
    {
      return this->tvProcessing;
    }

    explicit TVProcessorTab(QWidget *parent, qreal sampleRate);
    ~TVProcessorTab();

    void setDecisionMode(Decider::DecisionMode);
    void feed(const SUCOMPLEX *, unsigned int size);
    void setSampleRate(qreal);

  signals:
    void startTVProcessor(void);
    void stopTVProcessor(void);
    void tvProcessorDisposeFrame(struct sigutils_tv_frame_buffer *);
    void tvProcessorData();
    void tvProcessorParams(struct sigutils_tv_processor_params);

  public slots:
    void onTVProcessorUiChanged(void);
    void onToggleTVProcessor(void);
    void onTVProcessorFrame(struct sigutils_tv_frame_buffer *frame);
    void onTVProcessorParamsChanged(
        struct sigutils_tv_processor_params params);
    void onTVProcessorError(QString error);
    void onTVContrastChanged(void);
    void onTVBrightnessChanged(void);
    void onTVAspectChanged(void);

  private:
    Ui::TVProcessorTab *ui;
  };
}

#endif // TVPROCESSORTAB_H
