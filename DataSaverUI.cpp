#include "DataSaverUI.h"
#include "ui_DataSaverUI.h"

DataSaverUI::DataSaverUI(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::DataSaverUI)
{
  ui->setupUi(this);
}

DataSaverUI::~DataSaverUI()
{
  delete ui;
}
