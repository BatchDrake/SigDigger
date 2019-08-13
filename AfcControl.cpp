#include "AfcControl.h"
#include "ui_AfcControl.h"

AfcControl::AfcControl(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::AfcControl)
{
  ui->setupUi(this);
}

AfcControl::~AfcControl()
{
  delete ui;
}
