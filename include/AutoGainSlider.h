#ifndef AUTOGAINSLIDER_H
#define AUTOGAINSLIDER_H

#include <QWidget>

namespace Ui {
  class AutoGainSlider;
}

class AutoGainSlider : public QWidget
{
    Q_OBJECT

  public:
    explicit AutoGainSlider(QWidget *parent = nullptr);
    ~AutoGainSlider();

  private:
    Ui::AutoGainSlider *ui;
};

#endif // AUTOGAINSLIDER_H
