#ifndef GAINCONTROL_H
#define GAINCONTROL_H

#include <QWidget>

namespace Ui {
  class GainControl;
}

class GainControl : public QWidget
{
    Q_OBJECT

  public:
    explicit GainControl(QWidget *parent = nullptr);
    ~GainControl();

  private:
    Ui::GainControl *ui;
};

#endif // GAINCONTROL_H
