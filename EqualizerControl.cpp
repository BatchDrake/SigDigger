#include "EqualizerControl.h"
#include "ui_EqualizerControl.h"

EqualizerControl::EqualizerControl(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::EqualizerControl)
{
  ui->setupUi(this);
}

EqualizerControl::~EqualizerControl()
{
  delete ui;
}
