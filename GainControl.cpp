#include "GainControl.h"
#include "ui_GainControl.h"

GainControl::GainControl(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::GainControl)
{
  ui->setupUi(this);
}

GainControl::~GainControl()
{
  delete ui;
}
