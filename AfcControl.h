#ifndef AFCCONTROL_H
#define AFCCONTROL_H

#include <QWidget>

namespace Ui {
  class AfcControl;
}

class AfcControl : public QWidget
{
    Q_OBJECT

  public:
    explicit AfcControl(QWidget *parent = nullptr);
    ~AfcControl();

  private:
    Ui::AfcControl *ui;
};

#endif // AFCCONTROL_H
