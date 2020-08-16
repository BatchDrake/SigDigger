#ifndef WAVEFORMTAB_H
#define WAVEFORMTAB_H

#include <QWidget>
#include <sigutils/types.h>
#include "ColorConfig.h"

class ThrottleControl;
class QPushButton;

namespace Ui {
  class WaveformTab;
}

namespace SigDigger {
  class MultitaskController;

  class WaveformTab : public QWidget
  {
    Q_OBJECT

    qreal fs = 1;
    std::vector<SUCOMPLEX> buffer;
    MultitaskController *mtController = nullptr;

    bool recording = false;

    bool hadSelectionBefore = true; // Yep. This must be true.
    bool adjusting = false;
    bool firstShow = true;

    const SUCOMPLEX *getDisplayData(void) const;
    size_t getDisplayDataLength(void) const;

    int getPeriodicDivision(void) const;

    void recalcLimits(void);
    void refreshMeasures(void);
    void refreshUi(void);

    void samplingNotifySelection(bool, bool);
    void samplingSetEnabled(bool);

    bool fineTuneSenderIs(const QPushButton *sender) const;
    void fineTuneSelNotifySelection(bool);
    void fineTuneSelSetEnabled(bool);

    void connectFineTuneSelWidgets(void);
    void connectAll(void);
    void clear(void);

  public:
    explicit WaveformTab(QWidget *parent = 0);
    ~WaveformTab();

    void setThrottleControl(ThrottleControl *);    
    void setMultitaskController(MultitaskController *);
    void setSampleRate(qreal);
    void setColorConfig(ColorConfig const &cfg);
    void setPalette(std::string const &name);
    void setPaletteOffset(unsigned int offset);
    void setPaletteContrast(int contrast);

    std::string getPalette(void) const;
    unsigned int getPaletteOffset(void) const;
    int getPaletteContrast(void) const;

    inline bool
    isRecording(void) const
    {
      return this->recording;
    }

    void feed(const SUCOMPLEX *, unsigned int);

  public slots:
    void onHZoom(qint64 min, qint64 max);
    void onVZoom(qreal min, qreal max);
    void onClear(void);

    void onHSelection(qreal min, qreal max);
    void onVSelection(qreal min, qreal max);

    void onHoverTime(qreal);

    void onTogglePeriodicSelection(void);
    void onPeriodicDivisionsChanged(void);

    void onRecord(void);
    void onSaveAll(void);
    void onSaveSelection(void);
    void onFit(void);
    void onZoomToSelection(void);
    void onZoomReset(void);

    void onComponentChanged(void);

    void onShowWaveform(void);
    void onShowEnvelope(void);
    void onShowPhase(void);
    void onPhaseDerivative(void);

    void onPaletteChanged(int);
    void onChangePaletteOffset(int);
    void onChangePaletteContrast(int);

    void onFineTuneSelectionClicked(void);

  private:
    Ui::WaveformTab *ui;
  };
}

#endif // WAVEFORMTAB_H
