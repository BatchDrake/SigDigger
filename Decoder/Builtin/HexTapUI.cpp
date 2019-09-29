#include "include/HexTapUI.h"
#include "ui_HexTap.h"

using namespace SigDigger;

HexTapUI::HexTapUI(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::HexTap)
{
  ui->setupUi(this);

  this->onControlsChanged();

  connect(
        this->ui->verticalScrollBar,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onScroll(int)));

  connect(
        this->ui->frameSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->shiftSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->packCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->lsbCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->reverseCheck,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->clearButton,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onClear(void)));
}

QWidget *
HexTapUI::asWidget(void)
{
  return this;
}

void
HexTapUI::setThrottleControl(ThrottleControl *control)
{
  this->ui->egaConsole->setThrottleControl(control);
}

void
HexTapUI::repaint(void)
{
  size_t len = this->frameBytes->size();
  char lines[20];
  bool ascii = this->pack();

  lines[16] = 0;

  for (size_t i = this->lastLen; i < len; ++i) {
    if ((i & 0xf) == 0) {
      this->ui->egaConsole->setForeground(QColor(255, 0, 0));
      snprintf(lines, 18, "%08lx ", i);
      this->ui->egaConsole->print(lines);

      this->ui->egaConsole->setForeground(QColor(0, 255, 0));
    }

    snprintf(lines, 16, " %02x", (*this->frameBytes)[i]);
    this->ui->egaConsole->print(lines);

    if ((i & 7) == 7)
      this->ui->egaConsole->print(" ");

    if ((i & 0xf) == 0xf) {
      this->ui->egaConsole->print(" ");
      this->ui->egaConsole->setForeground(QColor(255, 255, 255));

      if (ascii) {
        this->ui->egaConsole->put(
            reinterpret_cast<const char *>(&(*this->frameBytes)[i - 15]),
            16);
      } else {
        for (size_t j = 0; j < 16; ++j)
          lines[j] = '0' + static_cast<char>((*this->frameBytes)[i + j - 15]);
        lines[16] = 0;
        this->ui->egaConsole->put(lines, 16);
      }

      this->ui->egaConsole->print("\n");
    }
  }

  this->ui->verticalScrollBar->setMinimum(0);

  if (this->ui->egaConsole->rows() >= this->ui->egaConsole->length()) {
    this->ui->verticalScrollBar->setEnabled(false);
    this->ui->verticalScrollBar->setMaximum(0);
  } else {
    this->ui->verticalScrollBar->setEnabled(true);
    this->ui->verticalScrollBar->setMaximum(this->ui->egaConsole->length());
    this->ui->verticalScrollBar->setPageStep(this->ui->egaConsole->rows());

    this->ui->verticalScrollBar->setValue(
          this->ui->egaConsole->length() - this->ui->egaConsole->rows());
  }

  this->lastLen = len;
}

void
HexTapUI::setFrameCount(int count)
{
  this->ui->frameCountLabel->setText("of " + QString::number(count));
  this->ui->frameSpin->setMaximum(count);
  if (this->autoScroll() && count > 0)
    this->ui->frameSpin->setValue(count - 1);
}

void
HexTapUI::setFrameBytes(FrameBytes *frameBytes)
{
  this->frameBytes = frameBytes;
  this->lastLen = 0;
  this->ui->egaConsole->clear();
  this->repaint();
  this->ui->egaConsole->invalidate();
  this->ui->egaConsole->scrollTo(0);
}

void
HexTapUI::refreshFrom(size_t)
{
  this->repaint();
}

unsigned int
HexTapUI::frameId(void) const
{
  return static_cast<unsigned>(this->ui->frameSpin->value() - 1);
}

int
HexTapUI::shift(void) const
{
  return this->ui->shiftSpin->value();
}

bool
HexTapUI::pack(void) const
{
  return this->ui->packCheck->isChecked();
}

bool
HexTapUI::lsb(void) const
{
  return this->ui->lsbCheck->isChecked();
}

bool
HexTapUI::reverse(void) const
{
  return this->ui->reverseCheck->isChecked();
}

bool
HexTapUI::autoScroll(void) const
{
  return this->ui->autoScrollButton->isChecked();
}

void
HexTapUI::resizeEvent(QResizeEvent *)
{
  this->ui->verticalScrollBar->setPageStep(this->ui->egaConsole->rows());
}

void
HexTapUI::onControlsChanged(void)
{
  this->ui->lsbCheck->setEnabled(this->pack());
  this->ui->reverseCheck->setEnabled(this->pack());
  this->ui->shiftSpin->setEnabled(this->pack());

  emit controlsChanged();
}

void
HexTapUI::onScroll(int pos)
{
  this->ui->egaConsole->scrollTo(pos);
}

void
HexTapUI::onClear(void)
{
  emit clear();
}

HexTapUI::~HexTapUI()
{
  delete ui;
}
