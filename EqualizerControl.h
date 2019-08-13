#ifndef EQUALIZERCONTROL_H
#define EQUALIZERCONTROL_H

#include <QWidget>

namespace Ui {
  class EqualizerControl;
}

class EqualizerControl : public QWidget
{
    Q_OBJECT

  public:
    explicit EqualizerControl(QWidget *parent = nullptr);
    ~EqualizerControl();

  private:
    Ui::EqualizerControl *ui;
};

#endif // EQUALIZERCONTROL_H
