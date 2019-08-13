#include "GainSlider.h"
#include "ui_GainSlider.h"

GainSlider::GainSlider(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::GainSlider)
{
  ui->setupUi(this);
}

GainSlider::~GainSlider()
{
  delete ui;
}
