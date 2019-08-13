#include "SaveProfileDialog.h"
#include "ui_SaveProfileDialog.h"

SaveProfileDialog::SaveProfileDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SaveProfileDialog)
{
  ui->setupUi(this);
}

SaveProfileDialog::~SaveProfileDialog()
{
  delete ui;
}
