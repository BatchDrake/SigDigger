#include "Inspector.h"
#include "ui_Inspector.h"

Inspector::Inspector(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::Inspector)
{
  ui->setupUi(this);
}

Inspector::~Inspector()
{
  delete ui;
}
