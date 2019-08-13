#ifndef SAVEPROFILEDIALOG_H
#define SAVEPROFILEDIALOG_H

#include <QDialog>

namespace Ui {
  class SaveProfileDialog;
}

class SaveProfileDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit SaveProfileDialog(QWidget *parent = nullptr);
    ~SaveProfileDialog();

  private:
    Ui::SaveProfileDialog *ui;
};

#endif // SAVEPROFILEDIALOG_H
