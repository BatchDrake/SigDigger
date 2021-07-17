#include "WaveformTab.h"
#include "ui_WaveformTab.h"
#include "SigDiggerHelpers.h"
#include "SuWidgetsHelpers.h"
#include <sigutils/types.h>
#include <string>

using namespace SigDigger;

#define WAVEFORM_TAB_MAX_SELECTION     4096

#ifdef __APPLE__
static void
adjustButtonToSize(QPushButton *button, QString text)
{
  if (text.size() == 0)
    text = button->text();

  button->setMaximumWidth(
        SuWidgetsHelpers::getWidgetTextWidth(button, text) +
        5 * SuWidgetsHelpers::getWidgetTextWidth(button, " "));
}
#endif // __APPLE__

WaveformTab::WaveformTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::WaveformTab)
{
  ui->setupUi(this);

  this->ui->realWaveform->setRealComponent(true);
  this->ui->imagWaveform->setRealComponent(false);

  QFontMetrics m(this->ui->selStartButtonsWidget->font());

#ifdef __APPLE__
  // Fix Qt limitations in MacOS
  this->ui->selStartButtonsWidget->setMaximumHeight(7 * m.height() / 4);
  this->ui->selEndButtonsWidget->setMaximumHeight(7 * m.height() / 4);

  adjustButtonToSize(this->ui->selStartDecDeltaTButton, ">>");
  adjustButtonToSize(this->ui->selStartDecSampleButton, ">>");
  adjustButtonToSize(this->ui->selStartIncSampleButton, ">>");
  adjustButtonToSize(this->ui->selStartIncDeltaTButton, ">>");

  adjustButtonToSize(this->ui->selEndDecDeltaTButton, ">>");
  adjustButtonToSize(this->ui->selEndDecSampleButton, ">>");
  adjustButtonToSize(this->ui->selEndIncSampleButton, ">>");
  adjustButtonToSize(this->ui->selEndIncDeltaTButton, ">>");
#endif // __APPLE__

  this->ui->realWaveform->setData(&this->buffer, true);
  this->ui->imagWaveform->setData(&this->buffer, true);

  this->refreshUi();
  this->refreshMeasures();
  SigDiggerHelpers::instance()->populatePaletteCombo(this->ui->paletteCombo);
  this->connectAll();
}

const SUCOMPLEX *
WaveformTab::getDisplayData(void) const
{
  return this->buffer.data();
}

size_t
WaveformTab::getDisplayDataLength(void) const
{
  return this->buffer.size();
}

void
WaveformTab::setSampleRate(qreal rate)
{
  this->fs = rate;

  if (sufeq(this->fs, 0, 1e-3)) {
    this->ui->recordButton->setChecked(false);
    this->ui->recordButton->setEnabled(false);
    this->recording = false;
  } else {
    this->ui->recordButton->setEnabled(true);
  }

  this->ui->realWaveform->setSampleRate(this->fs);
  this->ui->imagWaveform->setSampleRate(this->fs);
}

void
WaveformTab::feed(const SUCOMPLEX *data, unsigned int size)
{
  size_t prevSize = this->buffer.size();
  qreal prevDuration = prevSize / this->fs;
  qreal currDuration;

  std::copy(
        data,
        data + size,
        std::back_inserter(this->buffer));


  currDuration = this->buffer.size() / this->fs;

  this->ui->realWaveform->setData(&this->buffer, true);
  this->ui->imagWaveform->setData(&this->buffer, true);

  if (prevSize == 0) {
    this->onFit();
    this->ui->realWaveform->zoomHorizontal(
          static_cast<qint64>(0),
          static_cast<qint64>(this->fs));
    this->ui->imagWaveform->zoomHorizontal(
          static_cast<qint64>(0),
          static_cast<qint64>(this->fs));

    this->ui->realWaveform->invalidate();
    this->ui->imagWaveform->invalidate();
  }

  if (std::floor(currDuration) > std::floor(prevDuration)) {
    if (std::floor(currDuration) == 1)
      this->onFit();

    this->ui->realWaveform->zoomHorizontal(
          static_cast<qint64>(this->fs * std::floor(currDuration)),
          static_cast<qint64>(this->fs * (std::floor(currDuration) + 1.)));
    this->ui->imagWaveform->zoomHorizontal(
          static_cast<qint64>(this->fs * std::floor(currDuration)),
          static_cast<qint64>(this->fs * (std::floor(currDuration) + 1.)));
  }
}

void
WaveformTab::setPalette(std::string const &name)
{
  int index = SigDiggerHelpers::instance()->getPaletteIndex(name);

  if (index >= 0) {
    this->ui->paletteCombo->setCurrentIndex(index);
    this->onPaletteChanged(index);
  }
}

void
WaveformTab::setPaletteOffset(unsigned int offset)
{
  if (offset > 255)
    offset = 255;
  this->ui->offsetSlider->setValue(static_cast<int>(offset));
  this->onChangePaletteOffset(static_cast<int>(offset));
}

void
WaveformTab::setPaletteContrast(int contrast)
{
  this->ui->contrastSlider->setValue(contrast);
  this->onChangePaletteContrast(contrast);
}

void
WaveformTab::setColorConfig(ColorConfig const &cfg)
{
  this->ui->constellation->setBackgroundColor(cfg.constellationBackground);
  this->ui->constellation->setForegroundColor(cfg.constellationForeground);
  this->ui->constellation->setAxesColor(cfg.constellationAxes);

  this->ui->realWaveform->setBackgroundColor(cfg.spectrumBackground);
  this->ui->realWaveform->setForegroundColor(cfg.spectrumForeground);
  this->ui->realWaveform->setAxesColor(cfg.spectrumAxes);
  this->ui->realWaveform->setTextColor(cfg.spectrumText);
  this->ui->realWaveform->setSelectionColor(cfg.selection);

  this->ui->imagWaveform->setBackgroundColor(cfg.spectrumBackground);
  this->ui->imagWaveform->setForegroundColor(cfg.spectrumForeground);
  this->ui->imagWaveform->setAxesColor(cfg.spectrumAxes);
  this->ui->imagWaveform->setTextColor(cfg.spectrumText);
  this->ui->imagWaveform->setSelectionColor(cfg.selection);
}

void
WaveformTab::setThrottleControl(ThrottleControl *ctl)
{
  this->ui->realWaveform->setThrottleControl(ctl);
  this->ui->imagWaveform->setThrottleControl(ctl);
  this->ui->constellation->setThrottleControl(ctl);
}

void
WaveformTab::connectFineTuneSelWidgets(void)
{
  connect(
        this->ui->selStartDecDeltaTButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        this->ui->selStartDecSampleButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        this->ui->selStartIncDeltaTButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        this->ui->selStartIncSampleButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        this->ui->selEndDecDeltaTButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        this->ui->selEndDecSampleButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        this->ui->selEndIncDeltaTButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        this->ui->selEndIncSampleButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));
}


void
WaveformTab::connectAll(void)
{
  connect(
        this->ui->recordButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onRecord(void)));

  connect(
        this->ui->clearButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onClear(void)));

  connect(
        this->ui->realWaveform,
        SIGNAL(horizontalRangeChanged(qint64, qint64)),
        this,
        SLOT(onHZoom(qint64, qint64)));

  connect(
        this->ui->realWaveform,
        SIGNAL(horizontalSelectionChanged(qreal, qreal)),
        this,
        SLOT(onHSelection(qreal, qreal)));

  connect(
        this->ui->imagWaveform,
        SIGNAL(horizontalRangeChanged(qint64, qint64)),
        this,
        SLOT(onHZoom(qint64, qint64)));

  connect(
        this->ui->imagWaveform,
        SIGNAL(horizontalSelectionChanged(qreal, qreal)),
        this,
        SLOT(onHSelection(qreal, qreal)));

  connect(
        this->ui->realWaveform,
        SIGNAL(hoverTime(qreal)),
        this,
        SLOT(onHoverTime(qreal)));

  connect(
        this->ui->imagWaveform,
        SIGNAL(hoverTime(qreal)),
        this,
        SLOT(onHoverTime(qreal)));

  connect(
        this->ui->saveButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveAll(void)));

  connect(
        this->ui->saveSelectionButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveSelection(void)));

  connect(
        this->ui->fitButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onFit(void)));

  connect(
        this->ui->zoomSelButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onZoomToSelection(void)));

  connect(
        this->ui->resetZoomButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onZoomReset(void)));

  connect(
        this->ui->waveFormButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onShowWaveform(void)));

  connect(
        this->ui->envelopeButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onShowEnvelope(void)));

  connect(
        this->ui->phaseButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onShowPhase(void)));

  connect(
        this->ui->frequencyButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onPhaseDerivative(void)));

  connect(
        this->ui->periodicSelectionCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onTogglePeriodicSelection(void)));

  connect(
        this->ui->periodicDivisionsSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onPeriodicDivisionsChanged(void)));

  connect(
        this->ui->paletteCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onPaletteChanged(int)));

  connect(
        this->ui->componentCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onComponentChanged(void)));

  connect(
        this->ui->offsetSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onChangePaletteOffset(int)));

  connect(
        this->ui->contrastSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onChangePaletteContrast(int)));

  connectFineTuneSelWidgets();
}


std::string
WaveformTab::getPalette(void) const
{
  const Palette *palette = SigDiggerHelpers::instance()->getPalette(
        this->ui->paletteCombo->currentIndex());

  if (palette == nullptr)
    return "Suscan";

  return palette->getName();
}

unsigned int
WaveformTab::getPaletteOffset(void) const
{
  return static_cast<unsigned>(this->ui->offsetSlider->value());
}

int
WaveformTab::getPaletteContrast(void) const
{
  return this->ui->contrastSlider->value();
}

void
WaveformTab::fineTuneSelSetEnabled(bool enabled)
{
  this->ui->selStartButtonsWidget->setEnabled(enabled);
  this->ui->selEndButtonsWidget->setEnabled(enabled);
  this->ui->lockButton->setEnabled(enabled);
}

void
WaveformTab::fineTuneSelNotifySelection(bool sel)
{
  this->fineTuneSelSetEnabled(sel);
}

int
WaveformTab::getPeriodicDivision(void) const
{
  return this->ui->periodicDivisionsSpin->value();
}

void
WaveformTab::refreshUi(void)
{
  bool haveSelection = this->ui->realWaveform->getHorizontalSelectionPresent();
  this->ui->periodicDivisionsSpin->setEnabled(
        this->ui->periodicSelectionCheck->isChecked());
  this->ui->selStartLabel->setEnabled(haveSelection);
  this->ui->selEndLabel->setEnabled(haveSelection);
  this->ui->selLengthLabel->setEnabled(haveSelection);
  this->ui->periodLabel->setEnabled(haveSelection);
  this->ui->baudLabel->setEnabled(haveSelection);
  this->ui->saveSelectionButton->setEnabled(haveSelection);

  if (haveSelection != this->hadSelectionBefore)
    this->fineTuneSelNotifySelection(haveSelection);

  this->hadSelectionBefore = haveSelection;
}

void
WaveformTab::refreshMeasures(void)
{
  qreal selStart = 0;
  qreal selEnd   = 0;
  qreal deltaT = 1. / this->ui->realWaveform->getSampleRate();

  int length = static_cast<int>(this->getDisplayDataLength());

  if (this->ui->realWaveform->getHorizontalSelectionPresent()) {
    selStart = this->ui->realWaveform->getHorizontalSelectionStart();
    selEnd   = this->ui->realWaveform->getHorizontalSelectionEnd();

    if (selStart < 0)
      selStart = 0;
    if (selEnd > length)
      selEnd = length;
  }

  if (selEnd - selStart > 0) {
    qreal period =
        (selEnd - selStart) /
        (this->ui->periodicSelectionCheck->isChecked()
           ? this->getPeriodicDivision()
           : 1)
        * deltaT;
    qreal baud = 1 / period;

    this->ui->periodLabel->setText(
          SuWidgetsHelpers::formatQuantity(period, 4, "s"));
    this->ui->baudLabel->setText(SuWidgetsHelpers::formatReal(baud));
    this->ui->selStartLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            this->ui->realWaveform->samp2t(selStart),
            4,
            "s")
          + " (" + SuWidgetsHelpers::formatReal(selStart) + ")");
    this->ui->selEndLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            this->ui->realWaveform->samp2t(selEnd),
            4,
            "s")
          + " (" + SuWidgetsHelpers::formatReal(selEnd) + ")");
    this->ui->selLengthLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            (selEnd - selStart) * deltaT,
            4,
            "s")
          + " (" + SuWidgetsHelpers::formatReal(selEnd - selStart) + ")");
  } else {
    this->ui->periodLabel->setText("N/A");
    this->ui->baudLabel->setText("N/A");
    this->ui->selStartLabel->setText("N/A");
    this->ui->selEndLabel->setText("N/A");
    this->ui->selLengthLabel->setText("N/A");
  }
}

void
WaveformTab::clear(void)
{
  this->ui->realWaveform->setSampleRate(this->fs);
  this->ui->imagWaveform->setSampleRate(this->fs);

  this->buffer.clear();

  this->ui->realWaveform->setData(&this->buffer);
  this->ui->imagWaveform->setData(&this->buffer);
}

WaveformTab::~WaveformTab()
{
  delete ui;
}

//////////////////////////////////// Slots ////////////////////////////////////
void
WaveformTab::onHZoom(qint64 min, qint64 max)
{
  QObject* obj = sender();

  if (!this->adjusting) {
    Waveform *wf = nullptr;
    this->adjusting = true;

    if (obj == this->ui->realWaveform)
      wf = this->ui->imagWaveform;
    else
      wf = this->ui->realWaveform;

    wf->zoomHorizontal(min, max);
    wf->invalidate();
    this->adjusting = false;
  }
}

void
WaveformTab::onVZoom(qreal, qreal)
{

}

void
WaveformTab::onHSelection(qreal min, qreal max)
{
  QObject *obj = sender();

  if (!this->adjusting) {
    Waveform *curr = static_cast<Waveform *>(obj);
    Waveform *wf;
    this->adjusting = true;

    if (obj == this->ui->realWaveform)
      wf = this->ui->imagWaveform;
    else
      wf = this->ui->realWaveform;

    if (curr->getHorizontalSelectionPresent())
      wf->selectHorizontal(min, max);
    else
      wf->selectHorizontal(0, 0);

    this->refreshUi();
    this->refreshMeasures();
    wf->invalidate();

    this->adjusting = false;
  }
}

void
WaveformTab::onVSelection(qreal, qreal)
{

}

void
WaveformTab::onHoverTime(qreal time)
{
  const SUCOMPLEX *data = this->getDisplayData();
  int length = static_cast<int>(this->getDisplayDataLength());
  qreal samp = this->ui->realWaveform->t2samp(time);
  this->ui->positionLabel->setText(
        SuWidgetsHelpers::formatQuantity(time, 4, "s")
        + " (" + SuWidgetsHelpers::formatReal(samp) + ")");

  if (length > 0) {
    qint64 iSamp = static_cast<qint64>(std::floor(samp));
    qint64 selStart = 0, selEnd = 0, selLen = 0;
    qreal max = std::max<qreal>(
          std::max<qreal>(
            std::fabs(this->ui->realWaveform->getMax()),
            std::fabs(this->ui->realWaveform->getMin())),
          std::max<qreal>(
            std::fabs(this->ui->imagWaveform->getMax()),
            std::fabs(this->ui->imagWaveform->getMin())));


    qreal ampl = 1;
    if (max > 0)
      ampl = 1. / max;

    if (iSamp < 0)
      samp = iSamp = 0;
    if (iSamp > length)
      samp = iSamp = length - 1;

    SUFLOAT t = static_cast<SUFLOAT>(samp - iSamp);
    SUCOMPLEX val = (1 - t) * data[iSamp] + t * data[iSamp + 1];

    this->ui->constellation->setGain(ampl);

    if (this->ui->realWaveform->getHorizontalSelectionPresent()) {
       selStart = static_cast<qint64>(
            this->ui->realWaveform->getHorizontalSelectionStart());
      selEnd = static_cast<qint64>(
            this->ui->realWaveform->getHorizontalSelectionEnd());

      if (selStart < 0)
        selStart = 0;
      if (selEnd > length)
        selEnd = length;

      if (selEnd - selStart > WAVEFORM_TAB_MAX_SELECTION)
        selStart = selEnd - WAVEFORM_TAB_MAX_SELECTION;

      selLen = selEnd - selStart;

      if (selLen > 0) {
        this->ui->constellation->setHistorySize(
              static_cast<unsigned int>(selLen));
        this->ui->constellation->feed(
              data + selStart,
              static_cast<unsigned int>(selLen));
      }
    } else {
      if (iSamp == length - 1) {
        this->ui->constellation->setHistorySize(1);
        this->ui->constellation->feed(data + iSamp, 1);
      } else if (iSamp >= 0 && iSamp < length - 1) {
        this->ui->constellation->setHistorySize(1);
        this->ui->constellation->feed(&val, 1);
      } else {
        this->ui->constellation->setHistorySize(0);
      }
    }

    this->ui->iLabel->setText(SuWidgetsHelpers::formatScientific(SU_C_REAL(val)));
    this->ui->qLabel->setText(SuWidgetsHelpers::formatScientific(SU_C_IMAG(val)));
    this->ui->magPhaseLabel->setText(
          SuWidgetsHelpers::formatReal(SU_C_ABS(val))
          + "("
          + SuWidgetsHelpers::formatReal(SU_C_ARG(val) / M_PI * 180)
          + "º)");
  } else {
    this->ui->constellation->setHistorySize(0);

    this->ui->iLabel->setText("N/A");
    this->ui->qLabel->setText("N/A");
    this->ui->magPhaseLabel->setText("N/A");
  }
}

void
WaveformTab::onTogglePeriodicSelection(void)
{
  this->ui->realWaveform->setPeriodicSelection(
        this->ui->periodicSelectionCheck->isChecked());
  this->ui->imagWaveform->setPeriodicSelection(
        this->ui->periodicSelectionCheck->isChecked());

  this->ui->realWaveform->invalidate();
  this->ui->imagWaveform->invalidate();

  this->refreshUi();
}

void
WaveformTab::onPeriodicDivisionsChanged(void)
{
  this->ui->realWaveform->setDivsPerSelection(
        this->getPeriodicDivision());
  this->ui->imagWaveform->setDivsPerSelection(
        this->getPeriodicDivision());

  this->ui->realWaveform->invalidate();
  this->ui->imagWaveform->invalidate();

  this->refreshMeasures();
}

void
WaveformTab::onRecord(void)
{
  bool wasRecording = this->recording;

  this->recording = this->ui->recordButton->isChecked();

  if (!this->recording) {
    this->onFit();
    this->refreshMeasures();
    this->refreshUi();
  } else if (!wasRecording) {
    this->clear();
    this->onZoomReset();
  }
}

void
WaveformTab::onSaveAll(void)
{
  SigDiggerHelpers::openSaveSamplesDialog(
        this,
        this->getDisplayData(),
        this->getDisplayDataLength(),
        this->fs,
        0,
        static_cast<int>(this->getDisplayDataLength()),
        Suscan::Singleton::get_instance()->getBackgroundTaskController());
}

void
WaveformTab::onSaveSelection(void)
{
  SigDiggerHelpers::openSaveSamplesDialog(
        this,
        this->getDisplayData(),
        this->getDisplayDataLength(),
        this->fs,
        static_cast<int>(this->ui->realWaveform->getHorizontalSelectionStart()),
        static_cast<int>(this->ui->realWaveform->getHorizontalSelectionEnd()),
        Suscan::Singleton::get_instance()->getBackgroundTaskController());
}

void
WaveformTab::onFit(void)
{
  this->ui->realWaveform->fitToEnvelope();
  this->ui->imagWaveform->fitToEnvelope();
  this->ui->realWaveform->invalidate();
  this->ui->imagWaveform->invalidate();
}

void
WaveformTab::onZoomToSelection(void)
{
  if (this->ui->realWaveform->getHorizontalSelectionPresent()) {
    this->ui->realWaveform->zoomHorizontal(
          static_cast<qint64>(
            this->ui->realWaveform->getHorizontalSelectionStart()),
          static_cast<qint64>(
            this->ui->realWaveform->getHorizontalSelectionEnd()));
    this->ui->imagWaveform->zoomHorizontal(
          static_cast<qint64>(
            this->ui->realWaveform->getHorizontalSelectionStart()),
          static_cast<qint64>(
            this->ui->realWaveform->getHorizontalSelectionEnd()));
    this->ui->realWaveform->invalidate();
    this->ui->imagWaveform->invalidate();
  }
}

void
WaveformTab::onZoomReset(void)
{
  // Should propagate to imaginary
  this->ui->realWaveform->zoomHorizontalReset();
  this->ui->imagWaveform->zoomHorizontalReset();

  this->ui->realWaveform->invalidate();
  this->ui->imagWaveform->invalidate();
}

void
WaveformTab::onComponentChanged(void)
{
  bool haveReal = false;
  bool haveImag = false;

  switch (this->ui->componentCombo->currentIndex()) {
    case 0:
      haveReal = true;
      haveImag = false;
      break;

    case 1:
      haveReal = false;
      haveImag = true;
      break;

    case 2:
      haveReal = true;
      haveImag = true;
      break;
  }

  this->ui->realWaveform->setVisible(haveReal);
  this->ui->inPhaseLabel->setVisible(haveReal);

  this->ui->imagWaveform->setVisible(haveImag);
  this->ui->qualLabel->setVisible(haveImag);
}

void
WaveformTab::onShowWaveform(void)
{
  this->ui->realWaveform->setShowWaveform(
        this->ui->waveFormButton->isChecked());

  this->ui->imagWaveform->setShowWaveform(
        this->ui->waveFormButton->isChecked());
}

void
WaveformTab::onShowEnvelope(void)
{
  this->ui->realWaveform->setShowEnvelope(
        this->ui->envelopeButton->isChecked());

  this->ui->imagWaveform->setShowEnvelope(
        this->ui->envelopeButton->isChecked());

  this->ui->phaseButton->setEnabled(
        this->ui->envelopeButton->isChecked());

  this->ui->frequencyButton->setEnabled(
        this->ui->envelopeButton->isChecked());
}

void
WaveformTab::onShowPhase(void)
{
  this->ui->realWaveform->setShowPhase(this->ui->phaseButton->isChecked());
  this->ui->imagWaveform->setShowPhase(this->ui->phaseButton->isChecked());

  this->ui->frequencyButton->setEnabled(
        this->ui->phaseButton->isChecked());
}

void
WaveformTab::onPhaseDerivative(void)
{
  this->ui->realWaveform->setShowPhaseDiff(
        this->ui->frequencyButton->isChecked());

  this->ui->imagWaveform->setShowPhaseDiff(
        this->ui->frequencyButton->isChecked());
}

void
WaveformTab::onPaletteChanged(int index)
{
  const Palette *palette = SigDiggerHelpers::instance()->getPalette(index);

  if (palette != nullptr) {
    this->ui->realWaveform->setPalette(palette->getGradient());
    this->ui->imagWaveform->setPalette(palette->getGradient());
  }
}

void
WaveformTab::onChangePaletteOffset(int val)
{
  this->ui->realWaveform->setPhaseDiffOrigin(static_cast<unsigned>(val));
  this->ui->imagWaveform->setPhaseDiffOrigin(static_cast<unsigned>(val));
}

void
WaveformTab::onChangePaletteContrast(int contrast)
{
  qreal realContrast = std::pow(
        static_cast<qreal>(10),
        static_cast<qreal>(contrast / 20.));

  this->ui->realWaveform->setPhaseDiffContrast(realContrast);
  this->ui->imagWaveform->setPhaseDiffContrast(realContrast);
}

bool
WaveformTab::fineTuneSenderIs(const QPushButton *button) const
{
  QPushButton *sender = static_cast<QPushButton *>(this->sender());

  if (this->ui->lockButton->isChecked()) {
#define CHECKPAIR(a, b)                                   \
  if (button == this->ui->a || button == this->ui->b)     \
    return sender == this->ui->a || sender == this->ui->b

    CHECKPAIR(selStartIncDeltaTButton, selEndIncDeltaTButton);
    CHECKPAIR(selStartIncSampleButton, selEndIncSampleButton);
    CHECKPAIR(selStartDecDeltaTButton, selEndDecDeltaTButton);
    CHECKPAIR(selStartDecSampleButton, selEndDecSampleButton);
#undef CHECKPAIR
  }

  return button == sender;
}

void
WaveformTab::onFineTuneSelectionClicked(void)
{
  qint64 newSelStart =
      static_cast<qint64>(this->ui->realWaveform->getHorizontalSelectionStart());
  qint64 newSelEnd =
      static_cast<qint64>(this->ui->realWaveform->getHorizontalSelectionEnd());
  qint64 delta = newSelEnd - newSelStart;

#define CHECKBUTTON(btn) this->fineTuneSenderIs(this->ui->btn)

  if (CHECKBUTTON(selStartIncDeltaTButton))
    newSelStart += delta;

  if (CHECKBUTTON(selStartIncSampleButton))
    ++newSelStart;

  if (CHECKBUTTON(selStartDecDeltaTButton))
    newSelStart -= delta;

  if (CHECKBUTTON(selStartDecSampleButton))
    --newSelStart;

  if (CHECKBUTTON(selEndIncDeltaTButton))
    newSelEnd += delta;

  if (CHECKBUTTON(selEndIncSampleButton))
    ++newSelEnd;

  if (CHECKBUTTON(selEndDecDeltaTButton))
    newSelEnd -= delta;

  if (CHECKBUTTON(selEndDecSampleButton))
    --newSelEnd;

#undef CHECKBUTTON

  this->ui->imagWaveform->selectHorizontal(newSelStart, newSelEnd);
  this->ui->realWaveform->selectHorizontal(newSelStart, newSelEnd);
}

void
WaveformTab::onClear(void)
{
  this->clear();
}
