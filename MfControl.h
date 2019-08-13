#ifndef MFCONTROL_H
#define MFCONTROL_H

#include <QWidget>

namespace Ui {
  class MfControl;
}

class MfControl : public QWidget
{
    Q_OBJECT

  public:
    explicit MfControl(QWidget *parent = nullptr);
    ~MfControl();

  private:
    Ui::MfControl *ui;
};

#endif // MFCONTROL_H
