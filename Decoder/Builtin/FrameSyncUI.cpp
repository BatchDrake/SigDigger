#include "include/FrameSyncUI.h"
#include "ui_FrameSyncUI.h"

using namespace SigDigger;

FrameSyncUI::FrameSyncUI(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FrameSyncUI)
{
  ui->setupUi(this);

  this->ui->symEdit->setValidator(
        new QRegExpValidator(QRegExp("[0-9a-fA-F]*"), this));

  this->ui->bitsEdit->setValidator(
        new QRegExpValidator(QRegExp("[0-1]*"), this));

  this->controlsChanged();

  connect(
        this->ui->bitsEdit,
        SIGNAL(textEdited(const QString &)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->symEdit,
        SIGNAL(textEdited(const QString &)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->timesSpin,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->symRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->bitsRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->repeatRadio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onControlsChanged(void)));

  connect(
        this->ui->lsbCheck,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onControlsChanged(void)));
}


void
FrameSyncUI::setThrottleControl(ThrottleControl *)
{

}

void
FrameSyncUI::setBps(unsigned int bps)
{
  this->bps = bps;
  this->mask = (1 << bps) - 1;

  if (bps > 5) {
    if (this->ui->symRadio->isChecked())
      this->ui->bitsRadio->setChecked(true);
    this->ui->symRadio->setEnabled(false);
  } else {
    this->ui->symRadio->setEnabled(true);
  }

  // Recalculate lsb2msb dictionary
  for (unsigned i = 0; i < this->lsb2msb.size(); ++i) {
    Symbol s = 0;
    for (unsigned j = 0; j < bps; ++j)
      if (i & (1 << j))
        s |= 1 << (bps - j - 1);
    this->lsb2msb[i] = s;
  }
}

std::vector<Symbol> &
FrameSyncUI::getSequence(void)
{
  this->sequence.clear();

  if (this->ui->symRadio->isChecked()) {
    QString text = this->ui->symEdit->text();
    Symbol s;
    for (auto i = 0; i < text.length(); ++i) {
      s = text[i].cell() > '9'
          ? text[i].cell() - 'a' + 10
          : text[i].cell() - '0';

      this->sequence.push_back(s & this->mask);
    }
  } else if (this->ui->bitsRadio->isChecked()) {
    QString text = this->ui->bitsEdit->text();
    bool lsb = this->lsb();
    Symbol s = 0;
    unsigned ptr = 0;

    for (auto i = 0; i < text.length(); ++i) {
      if (text[i] != '0')
        s |= lsb ? 1 << ptr : 1 << (this->bps - ptr - 1);

      if (++ptr == this->bps) {
        ptr = 0;
        this->sequence.push_back(s);
        s = 0;
      }
    }
  }

  return this->sequence;
}

int
FrameSyncUI::getTimes(void) const
{
  return this->ui->timesSpin->value();
}

bool
FrameSyncUI::isRepetition(void) const
{
  return this->ui->repeatRadio->isChecked();
}

bool
FrameSyncUI::lsb(void) const
{
  return this->ui->lsbCheck->isChecked();
}

QWidget *
FrameSyncUI::asWidget(void)
{
  return this;
}

void
FrameSyncUI::onControlsChanged(void)
{
  this->ui->bitsEdit->setEnabled(this->ui->bitsRadio->isChecked());
  this->ui->lsbCheck->setEnabled(this->ui->bitsRadio->isChecked());

  this->ui->symEdit->setEnabled(this->ui->symRadio->isChecked());
  this->ui->timesSpin->setEnabled(this->ui->repeatRadio->isChecked());

  emit controlsChanged();
}

FrameSyncUI::~FrameSyncUI()
{
  delete ui;
}
