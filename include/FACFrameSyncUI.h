#ifndef FACFRAMESYNCUI_H
#define FACFRAMESYNCUI_H

#include <QWidget>

namespace Ui {
  class FACFrameSyncUI;
}

class FACFrameSyncUI : public QWidget
{
    Q_OBJECT

  public:
    explicit FACFrameSyncUI(QWidget *parent = nullptr);
    ~FACFrameSyncUI();

  private:
    Ui::FACFrameSyncUI *ui;
};

#endif // FACFRAMESYNCUI_H
