#ifndef GAINSLIDER_H
#define GAINSLIDER_H

#include <QWidget>

namespace Ui {
  class GainSlider;
}

class GainSlider : public QWidget
{
    Q_OBJECT

  public:
    explicit GainSlider(QWidget *parent = nullptr);
    ~GainSlider();

  private:
    Ui::GainSlider *ui;
};

#endif // GAINSLIDER_H
