#include "AskControl.h"
#include "ui_AskControl.h"

AskControl::AskControl(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::AskControl)
{
  ui->setupUi(this);
}

AskControl::~AskControl()
{
  delete ui;
}
