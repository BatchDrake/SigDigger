//
//    TVProcessorTab.h: Inspector's TV processor tab
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

#ifndef TVPROCESSORTAB_H
#define TVPROCESSORTAB_H

#include <QWidget>
#include "TVProcessorWorker.h"
#include <Decider.h>

namespace Ui {
  class TVProcessorTab;
}

namespace SigDigger {
  class TVProcessorTab : public QWidget
  {
    Q_OBJECT

    qreal                 m_sampleRate = 0;
    bool                  m_tvProcessing = false;
    bool                  m_editingTVProcessorParams = false;

    Decider::DecisionMode m_decisionMode = Decider::MODULUS;

    TVProcessorWorker    *m_tvWorker = nullptr;
    QThread              *m_tvThread = nullptr;
    std::vector<SUFLOAT>  m_floatBuffer;

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
      return this->m_tvProcessing;
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
    void onSaveSnapshot(void);
    void onTVProcessorUiChanged(void);
    void onToggleTVProcessor(void);
    void onTVProcessorFrame(struct sigutils_tv_frame_buffer *frame);
    void onTVProcessorParamsChanged(
        struct sigutils_tv_processor_params params);
    void onTVProcessorError(QString error);
    void onTVContrastChanged(void);
    void onTVBrightnessChanged(void);
    void onTVGammaChanged(void);
    void onTVAspectChanged(void);
    void onAccumChanged(void);
    void onEnableLPFChanged(void);
    void onAccumSpinChanged(void);

  private:
    Ui::TVProcessorTab *ui;
  };
}

#endif // TVPROCESSORTAB_H
