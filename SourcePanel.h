#ifndef SOURCEPANEL_H
#define SOURCEPANEL_H

#include <QWidget>

namespace Ui {
  class SourcePanel;
}

class SourcePanel : public QWidget
{
    Q_OBJECT

  public:
    explicit SourcePanel(QWidget *parent = nullptr);
    ~SourcePanel();

  private:
    Ui::SourcePanel *ui;
};

#endif // SOURCEPANEL_H
