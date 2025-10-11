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

  ui->realWaveform->setRealComponent(true);
  ui->imagWaveform->setRealComponent(false);
  ui->imagWaveform->reuseDisplayData(ui->realWaveform);
  ui->realWaveform->setAutoFitToEnvelope(false);
  ui->imagWaveform->setAutoFitToEnvelope(false);

  ui->realWaveform->setAutoScroll(true);
  ui->imagWaveform->setAutoScroll(true);

  QFontMetrics m(ui->selStartButtonsWidget->font());

#ifdef __APPLE__
  // Fix Qt limitations in MacOS
  ui->selStartButtonsWidget->setMaximumHeight(7 * m.height() / 4);
  ui->selEndButtonsWidget->setMaximumHeight(7 * m.height() / 4);

  adjustButtonToSize(ui->selStartDecDeltaTButton, ">>");
  adjustButtonToSize(ui->selStartDecSampleButton, ">>");
  adjustButtonToSize(ui->selStartIncSampleButton, ">>");
  adjustButtonToSize(ui->selStartIncDeltaTButton, ">>");

  adjustButtonToSize(ui->selEndDecDeltaTButton, ">>");
  adjustButtonToSize(ui->selEndDecSampleButton, ">>");
  adjustButtonToSize(ui->selEndIncSampleButton, ">>");
  adjustButtonToSize(ui->selEndIncDeltaTButton, ">>");
#endif // __APPLE__

  ui->realWaveform->setData(&m_buffer, true);
  ui->imagWaveform->setData(&m_buffer, true);

  refreshUi();
  refreshMeasures();
  SigDiggerHelpers::instance()->populatePaletteCombo(ui->paletteCombo);
  connectAll();
}

const SUCOMPLEX *
WaveformTab::getDisplayData(void) const
{
  return m_buffer.data();
}

size_t
WaveformTab::getDisplayDataLength(void) const
{
  return m_buffer.size();
}

void
WaveformTab::setSampleRate(qreal rate)
{
  m_fs = rate;

  if (sufeq(m_fs, 0, 1e-3)) {
    ui->recordButton->setChecked(false);
    ui->recordButton->setEnabled(false);
    m_recording = false;
  } else {
    ui->recordButton->setEnabled(true);
  }

  ui->realWaveform->setSampleRate(m_fs);
  ui->imagWaveform->setSampleRate(m_fs);
}

void
WaveformTab::scrollToLast(Waveform *wf)
{
  qint64 currSpan = wf->getSampleEnd() - wf->getSampleStart();
  qint64 lastSample = m_buffer.size() - 1;
  wf->zoomHorizontal(lastSample - currSpan, lastSample);
}

void
WaveformTab::feed(const SUCOMPLEX *data, unsigned int size)
{
  size_t prevSize = m_buffer.size();

  m_buffer.resize(prevSize + size);
  memcpy(m_buffer.data() + prevSize, data, size * sizeof (SUCOMPLEX));

  ui->realWaveform->setData(&m_buffer, true);
  ui->imagWaveform->setData(&m_buffer, true);

  if (prevSize == 0) {
    onFit();
    ui->realWaveform->zoomHorizontal(
          static_cast<qint64>(0),
          5 * static_cast<qint64>(m_fs));
    ui->imagWaveform->zoomHorizontal(
          static_cast<qint64>(0),
          5 * static_cast<qint64>(m_fs));

    ui->realWaveform->invalidate();
    ui->imagWaveform->invalidate();
  }


  scrollToLast(ui->realWaveform);
  scrollToLast(ui->imagWaveform);

  //currDuration = m_buffer.size() / m_fs;
  // qreal prevDuration = prevSize / m_fs;
  // qreal currDuration;

  /*
  if (std::floor(currDuration) > std::floor(prevDuration)) {
    if (std::floor(currDuration) == 1)
      onFit();

    ui->realWaveform->zoomHorizontal(
          static_cast<qint64>(m_fs * std::floor(currDuration)),
          static_cast<qint64>(m_fs * (std::floor(currDuration) + 1.)));
    ui->imagWaveform->zoomHorizontal(
          static_cast<qint64>(m_fs * std::floor(currDuration)),
          static_cast<qint64>(m_fs * (std::floor(currDuration) + 1.)));
  }
  */
}

void
WaveformTab::setPalette(std::string const &name)
{
  int index = SigDiggerHelpers::instance()->getPaletteIndex(name);

  if (index >= 0) {
    ui->paletteCombo->setCurrentIndex(index);
    onPaletteChanged(index);
  }
}

void
WaveformTab::setPaletteOffset(unsigned int offset)
{
  if (offset > 255)
    offset = 255;
  ui->offsetSlider->setValue(static_cast<int>(offset));
  onChangePaletteOffset(static_cast<int>(offset));
}

void
WaveformTab::setPaletteContrast(int contrast)
{
  ui->contrastSlider->setValue(contrast);
  onChangePaletteContrast(contrast);
}

void
WaveformTab::setColorConfig(ColorConfig const &cfg)
{
  ui->constellation->setBackgroundColor(cfg.constellationBackground);
  ui->constellation->setForegroundColor(cfg.constellationForeground);
  ui->constellation->setAxesColor(cfg.constellationAxes);

  ui->realWaveform->setBackgroundColor(cfg.spectrumBackground);
  ui->realWaveform->setForegroundColor(cfg.spectrumForeground);
  ui->realWaveform->setAxesColor(cfg.spectrumAxes);
  ui->realWaveform->setTextColor(cfg.spectrumText);
  ui->realWaveform->setSelectionColor(cfg.selection);

  ui->imagWaveform->setBackgroundColor(cfg.spectrumBackground);
  ui->imagWaveform->setForegroundColor(cfg.spectrumForeground);
  ui->imagWaveform->setAxesColor(cfg.spectrumAxes);
  ui->imagWaveform->setTextColor(cfg.spectrumText);
  ui->imagWaveform->setSelectionColor(cfg.selection);
}

void
WaveformTab::setThrottleControl(ThrottleControl *ctl)
{
  ui->realWaveform->setThrottleControl(ctl);
  ui->imagWaveform->setThrottleControl(ctl);
  ui->constellation->setThrottleControl(ctl);
}

void
WaveformTab::connectFineTuneSelWidgets(void)
{
  connect(
        ui->selStartDecDeltaTButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        ui->selStartDecSampleButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        ui->selStartIncDeltaTButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        ui->selStartIncSampleButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        ui->selEndDecDeltaTButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        ui->selEndDecSampleButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        ui->selEndIncDeltaTButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));

  connect(
        ui->selEndIncSampleButton,
        SIGNAL(clicked(void)),
        this,
        SLOT(onFineTuneSelectionClicked(void)));
}


void
WaveformTab::connectAll(void)
{
  connect(
        ui->recordButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onRecord(void)));

  connect(
        ui->clearButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onClear(void)));

  connect(
        ui->realWaveform,
        SIGNAL(horizontalRangeChanged(qint64, qint64)),
        this,
        SLOT(onHZoom(qint64, qint64)));

  connect(
        ui->realWaveform,
        SIGNAL(horizontalSelectionChanged(qreal, qreal)),
        this,
        SLOT(onHSelection(qreal, qreal)));

  connect(
        ui->imagWaveform,
        SIGNAL(horizontalRangeChanged(qint64, qint64)),
        this,
        SLOT(onHZoom(qint64, qint64)));

  connect(
        ui->imagWaveform,
        SIGNAL(horizontalSelectionChanged(qreal, qreal)),
        this,
        SLOT(onHSelection(qreal, qreal)));

  connect(
        ui->realWaveform,
        SIGNAL(hoverTime(qreal)),
        this,
        SLOT(onHoverTime(qreal)));

  connect(
        ui->imagWaveform,
        SIGNAL(hoverTime(qreal)),
        this,
        SLOT(onHoverTime(qreal)));

  connect(
        ui->saveButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveAll(void)));

  connect(
        ui->saveSelectionButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveSelection(void)));

  connect(
        ui->fitButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onFit(void)));

  connect(
        ui->zoomSelButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onZoomToSelection(void)));

  connect(
        ui->resetZoomButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onZoomReset(void)));

  connect(
        ui->waveFormButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onShowWaveform(void)));

  connect(
        ui->envelopeButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onShowEnvelope(void)));

  connect(
        ui->phaseButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onShowPhase(void)));

  connect(
        ui->frequencyButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onPhaseDerivative(void)));

  connect(
        ui->periodicSelectionCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onTogglePeriodicSelection(void)));

  connect(
        ui->periodicDivisionsSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onPeriodicDivisionsChanged(void)));

  connect(
        ui->paletteCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onPaletteChanged(int)));

  connect(
        ui->componentCombo,
        SIGNAL(activated(int)),
        this,
        SLOT(onComponentChanged(void)));

  connect(
        ui->offsetSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onChangePaletteOffset(int)));

  connect(
        ui->contrastSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onChangePaletteContrast(int)));

  connectFineTuneSelWidgets();
}


std::string
WaveformTab::getPalette(void) const
{
  const Palette *palette = SigDiggerHelpers::instance()->getPalette(
        ui->paletteCombo->currentIndex());

  if (palette == nullptr)
    return "Suscan";

  return palette->getName();
}

unsigned int
WaveformTab::getPaletteOffset(void) const
{
  return static_cast<unsigned>(ui->offsetSlider->value());
}

int
WaveformTab::getPaletteContrast(void) const
{
  return ui->contrastSlider->value();
}

void
WaveformTab::fineTuneSelSetEnabled(bool enabled)
{
  ui->selStartButtonsWidget->setEnabled(enabled);
  ui->selEndButtonsWidget->setEnabled(enabled);
  ui->lockButton->setEnabled(enabled);
}

void
WaveformTab::fineTuneSelNotifySelection(bool sel)
{
  fineTuneSelSetEnabled(sel);
}

int
WaveformTab::getPeriodicDivision(void) const
{
  return ui->periodicDivisionsSpin->value();
}

void
WaveformTab::refreshUi(void)
{
  bool haveSelection = ui->realWaveform->getHorizontalSelectionPresent();
  ui->periodicDivisionsSpin->setEnabled(
        ui->periodicSelectionCheck->isChecked());
  ui->selStartLabel->setEnabled(haveSelection);
  ui->selEndLabel->setEnabled(haveSelection);
  ui->selLengthLabel->setEnabled(haveSelection);
  ui->periodLabel->setEnabled(haveSelection);
  ui->baudLabel->setEnabled(haveSelection);
  ui->saveSelectionButton->setEnabled(haveSelection);

  if (haveSelection != m_hadSelectionBefore)
    fineTuneSelNotifySelection(haveSelection);

  m_hadSelectionBefore = haveSelection;
}

void
WaveformTab::refreshMeasures(void)
{
  qreal selStart = 0;
  qreal selEnd   = 0;
  qreal deltaT = 1. / ui->realWaveform->getSampleRate();

  int length = static_cast<int>(getDisplayDataLength());

  if (ui->realWaveform->getHorizontalSelectionPresent()) {
    selStart = ui->realWaveform->getHorizontalSelectionStart();
    selEnd   = ui->realWaveform->getHorizontalSelectionEnd();

    if (selStart < 0)
      selStart = 0;
    if (selEnd > length)
      selEnd = length;
  }

  if (selEnd - selStart > 0) {
    qreal period =
        (selEnd - selStart) /
        (ui->periodicSelectionCheck->isChecked()
           ? getPeriodicDivision()
           : 1)
        * deltaT;
    qreal baud = 1 / period;

    ui->periodLabel->setText(
          SuWidgetsHelpers::formatQuantity(period, 4, "s"));
    ui->baudLabel->setText(SuWidgetsHelpers::formatReal(baud));
    ui->selStartLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            ui->realWaveform->samp2t(selStart),
            4,
            "s")
          + " (" + SuWidgetsHelpers::formatReal(selStart) + ")");
    ui->selEndLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            ui->realWaveform->samp2t(selEnd),
            4,
            "s")
          + " (" + SuWidgetsHelpers::formatReal(selEnd) + ")");
    ui->selLengthLabel->setText(
          SuWidgetsHelpers::formatQuantity(
            (selEnd - selStart) * deltaT,
            4,
            "s")
          + " (" + SuWidgetsHelpers::formatReal(selEnd - selStart) + ")");
  } else {
    ui->periodLabel->setText("N/A");
    ui->baudLabel->setText("N/A");
    ui->selStartLabel->setText("N/A");
    ui->selEndLabel->setText("N/A");
    ui->selLengthLabel->setText("N/A");
  }
}

void
WaveformTab::clear(void)
{
  ui->realWaveform->setSampleRate(m_fs);
  ui->imagWaveform->setSampleRate(m_fs);

  m_buffer.clear();

  ui->realWaveform->setData(nullptr);
  ui->imagWaveform->setData(nullptr);
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

  if (!m_adjusting) {
    Waveform *wf = nullptr;
    m_adjusting = true;

    if (obj == ui->realWaveform)
      wf = ui->imagWaveform;
    else
      wf = ui->realWaveform;

    wf->zoomHorizontal(min, max);
    wf->invalidate();
    m_adjusting = false;
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

  if (!m_adjusting) {
    Waveform *curr = static_cast<Waveform *>(obj);
    Waveform *wf;
    m_adjusting = true;

    if (obj == ui->realWaveform)
      wf = ui->imagWaveform;
    else
      wf = ui->realWaveform;

    if (curr->getHorizontalSelectionPresent())
      wf->selectHorizontal(min, max);
    else
      wf->selectHorizontal(0, 0);

    refreshUi();
    refreshMeasures();
    wf->invalidate();

    m_adjusting = false;
  }
}

void
WaveformTab::onVSelection(qreal, qreal)
{

}

void
WaveformTab::onHoverTime(qreal time)
{
  const SUCOMPLEX *data = getDisplayData();
  int length = static_cast<int>(getDisplayDataLength());
  qreal samp = ui->realWaveform->t2samp(time);
  ui->positionLabel->setText(
        SuWidgetsHelpers::formatQuantity(time, 4, "s")
        + " (" + SuWidgetsHelpers::formatReal(samp) + ")");

  if (length > 0) {
    SUCOMPLEX val;
    qint64 iSamp = static_cast<qint64>(std::floor(samp));
    qint64 selStart = 0, selEnd = 0, selLen = 0;
    qreal max = std::max<qreal>(
          std::max<qreal>(
            std::fabs(ui->realWaveform->getMax()),
            std::fabs(ui->realWaveform->getMin())),
          std::max<qreal>(
            std::fabs(ui->imagWaveform->getMax()),
            std::fabs(ui->imagWaveform->getMin())));


    qreal ampl = 1;
    if (max > 0)
      ampl = 1. / max;

    if (iSamp < 0)
      samp = iSamp = 0;
    if (iSamp > length)
      samp = iSamp = length - 1;

    SUFLOAT t = static_cast<SUFLOAT>(samp - iSamp);
    if (iSamp == length - 1)
      val = data[iSamp];
    else
      val = (1 - t) * data[iSamp] + t * data[iSamp + 1];

    ui->constellation->setGain(ampl);

    if (ui->realWaveform->getHorizontalSelectionPresent()) {
       selStart = static_cast<qint64>(
            ui->realWaveform->getHorizontalSelectionStart());
      selEnd = static_cast<qint64>(
            ui->realWaveform->getHorizontalSelectionEnd());

      if (selStart < 0)
        selStart = 0;
      if (selEnd > length)
        selEnd = length;

      if (selEnd - selStart > WAVEFORM_TAB_MAX_SELECTION)
        selStart = selEnd - WAVEFORM_TAB_MAX_SELECTION;

      selLen = selEnd - selStart;

      if (selLen > 0) {
        ui->constellation->setHistorySize(
              static_cast<unsigned int>(selLen));
        ui->constellation->feed(
              data + selStart,
              static_cast<unsigned int>(selLen));
      }
    } else {
      if (iSamp == length - 1) {
        ui->constellation->setHistorySize(1);
        ui->constellation->feed(data + iSamp, 1);
      } else if (iSamp >= 0 && iSamp < length - 1) {
        ui->constellation->setHistorySize(1);
        ui->constellation->feed(&val, 1);
      } else {
        ui->constellation->setHistorySize(0);
      }
    }

    ui->iLabel->setText(SuWidgetsHelpers::formatScientific(SU_C_REAL(val)));
    ui->qLabel->setText(SuWidgetsHelpers::formatScientific(SU_C_IMAG(val)));
    ui->magPhaseLabel->setText(
          SuWidgetsHelpers::formatReal(SU_C_ABS(val))
          + "("
          + SuWidgetsHelpers::formatReal(SU_C_ARG(val) / M_PI * 180)
          + "º)");
  } else {
    ui->constellation->setHistorySize(0);

    ui->iLabel->setText("N/A");
    ui->qLabel->setText("N/A");
    ui->magPhaseLabel->setText("N/A");
  }
}

void
WaveformTab::onTogglePeriodicSelection(void)
{
  ui->realWaveform->setPeriodicSelection(
        ui->periodicSelectionCheck->isChecked());
  ui->imagWaveform->setPeriodicSelection(
        ui->periodicSelectionCheck->isChecked());

  ui->realWaveform->invalidate();
  ui->imagWaveform->invalidate();

  refreshUi();
}

void
WaveformTab::onPeriodicDivisionsChanged(void)
{
  ui->realWaveform->setDivsPerSelection(
        getPeriodicDivision());
  ui->imagWaveform->setDivsPerSelection(
        getPeriodicDivision());

  ui->realWaveform->invalidate();
  ui->imagWaveform->invalidate();

  refreshMeasures();
}

void
WaveformTab::onRecord(void)
{
  bool wasRecording = m_recording;

  m_recording = ui->recordButton->isChecked();

  if (!m_recording) {
    onFit();
    refreshMeasures();
    refreshUi();
  } else if (!wasRecording) {
    clear();
    onZoomReset();
  }
}

void
WaveformTab::onSaveAll(void)
{
  SigDiggerHelpers::openSaveSamplesDialog(
        this,
        getDisplayData(),
        getDisplayDataLength(),
        m_fs,
        0,
        static_cast<int>(getDisplayDataLength()),
        Suscan::Singleton::get_instance()->getBackgroundTaskController());
}

void
WaveformTab::onSaveSelection(void)
{
  SigDiggerHelpers::openSaveSamplesDialog(
        this,
        getDisplayData(),
        getDisplayDataLength(),
        m_fs,
        static_cast<int>(ui->realWaveform->getHorizontalSelectionStart()),
        static_cast<int>(ui->realWaveform->getHorizontalSelectionEnd()),
        Suscan::Singleton::get_instance()->getBackgroundTaskController());
}

void
WaveformTab::onFit(void)
{
  ui->realWaveform->fitToEnvelope();
  ui->imagWaveform->fitToEnvelope();
  ui->realWaveform->invalidate();
  ui->imagWaveform->invalidate();
}

void
WaveformTab::onZoomToSelection(void)
{
  if (ui->realWaveform->getHorizontalSelectionPresent()) {
    ui->realWaveform->zoomHorizontal(
          static_cast<qint64>(
            ui->realWaveform->getHorizontalSelectionStart()),
          static_cast<qint64>(
            ui->realWaveform->getHorizontalSelectionEnd()));
    ui->imagWaveform->zoomHorizontal(
          static_cast<qint64>(
            ui->realWaveform->getHorizontalSelectionStart()),
          static_cast<qint64>(
            ui->realWaveform->getHorizontalSelectionEnd()));
    ui->realWaveform->invalidate();
    ui->imagWaveform->invalidate();
  }
}

void
WaveformTab::onZoomReset(void)
{
  ui->realWaveform->zoomHorizontalReset();
  ui->imagWaveform->zoomHorizontalReset();

  ui->realWaveform->invalidate();
  ui->imagWaveform->invalidate();
}

void
WaveformTab::onComponentChanged(void)
{
  bool haveReal = false;
  bool haveImag = false;

  switch (ui->componentCombo->currentIndex()) {
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

  ui->realWaveform->setVisible(haveReal);
  ui->inPhaseLabel->setVisible(haveReal);

  ui->imagWaveform->setVisible(haveImag);
  ui->qualLabel->setVisible(haveImag);
}

void
WaveformTab::onShowWaveform(void)
{
  ui->realWaveform->setShowWaveform(
        ui->waveFormButton->isChecked());

  ui->imagWaveform->setShowWaveform(
        ui->waveFormButton->isChecked());
}

void
WaveformTab::onShowEnvelope(void)
{
  ui->realWaveform->setShowEnvelope(
        ui->envelopeButton->isChecked());

  ui->imagWaveform->setShowEnvelope(
        ui->envelopeButton->isChecked());

  ui->phaseButton->setEnabled(
        ui->envelopeButton->isChecked());

  ui->frequencyButton->setEnabled(
        ui->envelopeButton->isChecked());
}

void
WaveformTab::onShowPhase(void)
{
  ui->realWaveform->setShowPhase(ui->phaseButton->isChecked());
  ui->imagWaveform->setShowPhase(ui->phaseButton->isChecked());

  ui->frequencyButton->setEnabled(
        ui->phaseButton->isChecked());
}

void
WaveformTab::onPhaseDerivative(void)
{
  ui->realWaveform->setShowPhaseDiff(
        ui->frequencyButton->isChecked());

  ui->imagWaveform->setShowPhaseDiff(
        ui->frequencyButton->isChecked());
}

void
WaveformTab::onPaletteChanged(int index)
{
  const Palette *palette = SigDiggerHelpers::instance()->getPalette(index);

  if (palette != nullptr) {
    ui->realWaveform->setPalette(palette->getGradient());
    ui->imagWaveform->setPalette(palette->getGradient());
  }
}

void
WaveformTab::onChangePaletteOffset(int val)
{
  ui->realWaveform->setPhaseDiffOrigin(static_cast<unsigned>(val));
  ui->imagWaveform->setPhaseDiffOrigin(static_cast<unsigned>(val));
}

void
WaveformTab::onChangePaletteContrast(int contrast)
{
  qreal realContrast = std::pow(
        static_cast<qreal>(10),
        static_cast<qreal>(contrast / 20.));

  ui->realWaveform->setPhaseDiffContrast(realContrast);
  ui->imagWaveform->setPhaseDiffContrast(realContrast);
}

bool
WaveformTab::fineTuneSenderIs(const QPushButton *button) const
{
  QPushButton *sender = static_cast<QPushButton *>(QObject::sender());

  if (ui->lockButton->isChecked()) {
#define CHECKPAIR(a, b)                                   \
  if (button == ui->a || button == ui->b)     \
    return sender == ui->a || sender == ui->b

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
      static_cast<qint64>(ui->realWaveform->getHorizontalSelectionStart());
  qint64 newSelEnd =
      static_cast<qint64>(ui->realWaveform->getHorizontalSelectionEnd());
  qint64 delta = newSelEnd - newSelStart;

#define CHECKBUTTON(btn) fineTuneSenderIs(ui->btn)

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

  ui->imagWaveform->selectHorizontal(newSelStart, newSelEnd);
  ui->realWaveform->selectHorizontal(newSelStart, newSelEnd);
}

void
WaveformTab::onClear(void)
{
  clear();
}
