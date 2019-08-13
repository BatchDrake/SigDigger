#include "ToneControl.h"
#include "ui_ToneControl.h"

ToneControl::ToneControl(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ToneControl)
{
  ui->setupUi(this);
}

ToneControl::~ToneControl()
{
  delete ui;
}
