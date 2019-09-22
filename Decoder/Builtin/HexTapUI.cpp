#include "include/HexTapUI.h"
#include "ui_HexTap.h"

using namespace SigDigger;

HexTapUI::HexTapUI(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::HexTap)
{
  ui->setupUi(this);
}

HexTapUI::~HexTapUI()
{
  delete ui;
}
