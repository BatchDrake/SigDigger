#ifndef ASYNCDATASAVER_H
#define ASYNCDATASAVER_H

#include <QObject>

class AsyncDataSaver : public QObject
{
    Q_OBJECT
  public:
    explicit AsyncDataSaver(QObject *parent = nullptr);

  signals:

  public slots:
};

#endif // ASYNCDATASAVER_H