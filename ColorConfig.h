#ifndef COLORCONFIG_H
#define COLORCONFIG_H

#include <QObject>

class ColorConfig : public QObject
{
    Q_OBJECT
  public:
    explicit ColorConfig(QObject *parent = nullptr);

  signals:

  public slots:
};

#endif // COLORCONFIG_H