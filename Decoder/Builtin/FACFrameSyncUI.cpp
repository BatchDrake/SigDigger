#include "include/FACFrameSyncUI.h"
#include "ui_FACFrameSyncUI.h"

FACFrameSyncUI::FACFrameSyncUI(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FACFrameSyncUI)
{
  ui->setupUi(this);
}

FACFrameSyncUI::~FACFrameSyncUI()
{
  delete ui;
}
