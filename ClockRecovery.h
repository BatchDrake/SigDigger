#ifndef CLOCKRECOVERY_H
#define CLOCKRECOVERY_H

#include <QWidget>

namespace Ui {
  class ClockRecovery;
}

class ClockRecovery : public QWidget
{
    Q_OBJECT

  public:
    explicit ClockRecovery(QWidget *parent = nullptr);
    ~ClockRecovery();

  private:
    Ui::ClockRecovery *ui;
};

#endif // CLOCKRECOVERY_H
