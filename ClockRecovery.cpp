#include "ClockRecovery.h"
#include "ui_ClockRecovery.h"

ClockRecovery::ClockRecovery(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ClockRecovery)
{
  ui->setupUi(this);
}

ClockRecovery::~ClockRecovery()
{
  delete ui;
}
