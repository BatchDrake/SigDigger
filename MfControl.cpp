#include "MfControl.h"
#include "ui_MfControl.h"

MfControl::MfControl(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::MfControl)
{
  ui->setupUi(this);
}

MfControl::~MfControl()
{
  delete ui;
}
