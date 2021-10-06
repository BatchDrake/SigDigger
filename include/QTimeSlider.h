//
//    filename: description
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this program.  If not, see
//    <http://www.gnu.org/licenses/>
//
#ifndef QTIMESLIDER_H
#define QTIMESLIDER_H

#include <QSlider>
#include <QDateTime>

class QTimeSlider : public QSlider
{
  Q_OBJECT

  bool ticksAreSamples = false;
  qint64 sampleRate = 1000000;
  struct timeval startTime;
  struct timeval endTime;

  void adjustTickInterval(void);

  protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;

  public:
    QTimeSlider(QWidget *parent = nullptr);

    void setSampleRate(quint64);
    void setStartTime(QDateTime const &);
    void setStartTime(struct timeval const &);

    void setEndTime(QDateTime const &);
    void setEndTime(struct timeval const &);

    void setTimeStamp(struct timeval const &);

    QDateTime getDateTime(void) const;
    struct timeval getTimeStamp(void) const;
    qint64 getSample(void) const;
};

#endif // QTIMESLIDER_H
