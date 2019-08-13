#ifndef DATASAVERUI_H
#define DATASAVERUI_H

#include <QWidget>

namespace Ui {
  class DataSaverUI;
}

class DataSaverUI : public QWidget
{
    Q_OBJECT

  public:
    explicit DataSaverUI(QWidget *parent = nullptr);
    ~DataSaverUI();

  private:
    Ui::DataSaverUI *ui;
};

#endif // DATASAVERUI_H
