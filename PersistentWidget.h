#ifndef PERSISTENTWIDGET_H
#define PERSISTENTWIDGET_H

#include <QWidget>

class PersistentWidget : public QWidget
{
    Q_OBJECT
  public:
    explicit PersistentWidget(QWidget *parent = nullptr);

  signals:

  public slots:
};

#endif // PERSISTENTWIDGET_H