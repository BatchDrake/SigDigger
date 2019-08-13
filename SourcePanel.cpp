#include "SourcePanel.h"
#include "ui_SourcePanel.h"

SourcePanel::SourcePanel(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SourcePanel)
{
  ui->setupUi(this);
}

SourcePanel::~SourcePanel()
{
  delete ui;
}
