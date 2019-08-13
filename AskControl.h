#ifndef ASKCONTROL_H
#define ASKCONTROL_H

#include <QWidget>

namespace Ui {
  class AskControl;
}

class AskControl : public QWidget
{
    Q_OBJECT

  public:
    explicit AskControl(QWidget *parent = nullptr);
    ~AskControl();

  private:
    Ui::AskControl *ui;
};

#endif // ASKCONTROL_H
