#ifndef TONECONTROL_H
#define TONECONTROL_H

#include <QWidget>

namespace Ui {
  class ToneControl;
}

class ToneControl : public QWidget
{
    Q_OBJECT

  public:
    explicit ToneControl(QWidget *parent = nullptr);
    ~ToneControl();

  private:
    Ui::ToneControl *ui;
};

#endif // TONECONTROL_H
