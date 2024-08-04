//
//    QTimeSlider: QSlider for time quantities
//    Copyright (C) 2021 Gonzalo José Carracedo Carballal
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
#include <QTimeSlider.h>
#include <sigutils/util/compat-time.h>
#include <QPainter>
#include <stdlib.h>
#include <cmath>
#include <QStyle>
#include <QMouseEvent>
#include <QStyleOptionSlider>
#include "SigDiggerHelpers.h"
#include <QProxyStyle>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  define MidButton MiddleButton
#endif

using namespace SigDigger;

class AbsolutePositioningStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    int styleHint(
        QStyle::StyleHint hint,
        const QStyleOption* option = nullptr,
        const QWidget* widget = nullptr,
        QStyleHintReturn* returnData = nullptr) const override
    {
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
            return (Qt::LeftButton | Qt::MidButton | Qt::RightButton);
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

QTimeSlider::QTimeSlider(QWidget *parent) : QSlider(parent)
{
  struct timeval delta = {1, 0};

  this->setTickInterval(1);

  gettimeofday(&this->startTime, nullptr);
  this->startTime.tv_usec = 0;
  timeradd(&this->startTime, &delta, &this->endTime);

  this->setTickPosition(TickPosition::NoTicks);
  this->setOrientation(Qt::Orientation::Horizontal);

  this->setMinimum(0);
  this->setMaximum(2e9);
  this->setMinimumHeight(42);
  this->setTickInterval(this->maximum());

  this->setStyle(new AbsolutePositioningStyle(this->style()));

  QSizePolicy sizePolicy2(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
  sizePolicy2.setHorizontalStretch(0);
  sizePolicy2.setVerticalStretch(0);
  sizePolicy2.setHeightForWidth(this->sizePolicy().hasHeightForWidth());

  this->setSizePolicy(sizePolicy2);
}

void
QTimeSlider::paintEvent(QPaintEvent *ev)
{
  if (timercmp(&this->startTime, &this->endTime, <=)) {
    QPainter p(this);
    QString tickFormat;
    QColor tickColor = this->palette().color(QPalette::WindowText);
    QFont font;
    qreal lastTextX = 0, textX;
    struct timeval tvFirstTick;
    int tw, th;
    int i;
    struct timeval diff;
    qreal asMsec, x;
    qreal pxPerTick, pxPerLabel;
    qreal tickStepMsec = 1;
    qreal tickSubStepMsec = .1;
    qreal startMsec, alignStartMsec;
    qreal saneTimeSteps[] = {
      1e-2, 1e-1, .25, .5, 1, 10, 25, 50, 1e2, 250, 500,
      1e3, 5e3, 10e3, 30e3, 60e3, 300e3, 900e3, 1800e3, 36000e3};

    qint64 minTicks = 10;
    qint64 maxTicks = this->width() / 2;
    qreal maxTickLen = this->height() / 3;

    QFontMetrics metrics(font);

    th = metrics.height();

    timersub(&this->endTime, &this->startTime, &diff);

    startMsec = static_cast<qreal>(startTime.tv_sec) * 1e3
        + static_cast<qreal>(startTime.tv_usec) * 1e-3;

    asMsec = static_cast<qreal>(diff.tv_sec) * 1e3
        + static_cast<qreal>(diff.tv_usec) * 1e-3;

    /*
     * We are going to assume that this tickStep has room for 10
     * additional subSteps.
     */
    for (
         i = 0;
         i < static_cast<int>(sizeof(saneTimeSteps) / sizeof(qreal));
         ++i) {
      tickStepMsec = saneTimeSteps[i];
      tickSubStepMsec = .1 * tickStepMsec;
      if (asMsec / tickSubStepMsec >= static_cast<qreal>(minTicks)
          && asMsec / tickSubStepMsec < static_cast<qreal>(maxTicks))
        break;
    }

    // Too few ticks
    if (asMsec / tickSubStepMsec < static_cast<qreal>(minTicks)) {
      tickSubStepMsec = asMsec / static_cast<qreal>(minTicks);
      tickStepMsec    = 10 * tickSubStepMsec;
    } else if (asMsec / tickSubStepMsec > static_cast<qreal>(maxTicks)){
      tickSubStepMsec = asMsec / static_cast<qreal>(maxTicks);
      tickStepMsec    = 10 * tickSubStepMsec;
    }

    // Calculate the right format
    if (tickStepMsec < 100)
      tickFormat = "hh:mm:ss.zzz";
    else if (tickStepMsec < 1000)
      tickFormat = "hh:mm:ss.z";
    else if (tickStepMsec < 60e3)
      tickFormat = "hh:mm:ss";
    else
      tickFormat = "hh:mm";

    pxPerTick  = this->width() / (asMsec / tickSubStepMsec);
    pxPerLabel = this->width() / (asMsec / tickStepMsec);

    // Calculate where the first labeled tick should start
    alignStartMsec = tickStepMsec * ceil(startMsec / tickStepMsec);
    x = (alignStartMsec - startMsec) / tickStepMsec * pxPerLabel;
    i = static_cast<int>(floor(x / pxPerTick));


    tvFirstTick.tv_sec  = static_cast<int>(floor(alignStartMsec / 1000));
    tvFirstTick.tv_usec = static_cast<int>(
          (alignStartMsec - tvFirstTick.tv_sec * 1000) * 1000);

    x -= i * pxPerTick;
    i = -i;

    p.setPen(QPen(tickColor));
    p.setRenderHint(QPainter::Antialiasing);

    while (x < this->width()) {
      if (i % 10 == 0) {
        SigDiggerHelpers *hlp = SigDiggerHelpers::instance();
        QString text;
        QRect rect;

        hlp->pushLocalTZ();

        {
          QDateTime dateTime;
          dateTime.setMSecsSinceEpoch(
                tvFirstTick.tv_sec * 1000
                + tvFirstTick.tv_usec / 1000
                + static_cast<qint64>((i / 10) * tickStepMsec));
          text = dateTime.toString(tickFormat);
        }

        hlp->popTZ();

      #if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
              tw = metrics.horizontalAdvance(text);
      #else
              tw = metrics.width(text);
      #endif // QT_VERSION_CHECK

        textX = x - .5 * tw;
        if (textX > lastTextX + 10) {
          rect.setRect(
                static_cast<int>(textX),
                static_cast<int>(this->height() - th),
                tw,
                th);
          lastTextX = textX + tw;
          p.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
        }

        p.drawLine(QLineF(x, 0 , x, maxTickLen));
      } else if (i % 5 == 0) {
        p.drawLine(QLineF(x, .5 * maxTickLen, x, maxTickLen));
      } else {
        p.drawLine(QLineF(x, .75 * maxTickLen, x, maxTickLen));
      }

      ++i;
      x += pxPerTick;
    }
  }

  QSlider::paintEvent(ev);
}

void
QTimeSlider::resizeEvent(QResizeEvent *ev)
{
  QSlider::resizeEvent(ev);
  this->repaint();
}


void
QTimeSlider::setSampleRate(quint64 rate)
{
  this->sampleRate = static_cast<qint64>(rate);
  this->repaint();
}

void
QTimeSlider::setStartTime(QDateTime const &dateTime)
{
  qint64 asMs = dateTime.currentMSecsSinceEpoch();
  struct timeval tv;

  tv.tv_sec  = asMs / 1000;
  tv.tv_usec = (asMs - 1000 * tv.tv_sec) * 1000;

  this->setStartTime(tv);
}

void
QTimeSlider::setStartTime(struct timeval const &tv)
{
  this->startTime = tv;
  this->repaint();
}

void
QTimeSlider::setEndTime(QDateTime const &dateTime)
{
  qint64 asMs = dateTime.currentMSecsSinceEpoch();
  struct timeval tv;

  tv.tv_sec  = asMs / 1000;
  tv.tv_usec = (asMs - 1000 * tv.tv_sec) * 1000;

  this->setEndTime(tv);
}
void
QTimeSlider::setEndTime(struct timeval const &tv)
{
  this->endTime = tv;
  this->repaint();
}

void
QTimeSlider::setTimeStamp(struct timeval const &tv)
{
  struct timeval span, diff;
  qreal tvMsec, spanMsec;

  timersub(&this->endTime, &this->startTime, &span);
  timersub(&tv, &this->startTime, &diff);

  spanMsec = static_cast<qreal>(span.tv_sec) * 1e3
      + static_cast<qreal>(span.tv_usec) * 1e-3;

  tvMsec = static_cast<qreal>(diff.tv_sec) * 1e3
      + static_cast<qreal>(diff.tv_usec) * 1e-3;

  this->blockSignals(true);
  this->setValue(
        static_cast<int>((tvMsec / spanMsec) * this->maximum()));
  this->blockSignals(false);
}

QDateTime
QTimeSlider::getDateTime(void) const
{
  QDateTime dateTime;
  struct timeval tv = this->getTimeStamp();

  dateTime.setMSecsSinceEpoch(tv.tv_sec * 1000 + tv.tv_usec / 1000);

  return dateTime;
}

struct timeval
QTimeSlider::getTimeStamp(void) const
{
  struct timeval span, tv;
  qint64 spanMsec;
  qreal fraction;

  timersub(&this->endTime, &this->startTime, &span);

  spanMsec = span.tv_sec * 1000 + span.tv_usec / 1000;
  fraction = static_cast<qreal>(this->value()) / this->maximum();

  spanMsec = static_cast<qint64>(fraction * static_cast<qreal>(spanMsec));

  tv.tv_sec  = spanMsec / 1000;
  tv.tv_usec = (spanMsec - 1000 * tv.tv_sec) * 1000;

  return tv;
}

qint64
QTimeSlider::getSample(void) const
{
  struct timeval span;
  qint64 spanMsec;
  qreal fraction;

  timersub(&this->endTime, &this->startTime, &span);

  spanMsec = span.tv_sec * 1000 + span.tv_usec / 1000;
  fraction = static_cast<qreal>(this->value()) / this->maximum();

  spanMsec = static_cast<qint64>(fraction * static_cast<qreal>(spanMsec));

  return
      (spanMsec / 1000) * this->sampleRate
      + (spanMsec % 1000) * this->sampleRate * 1000;
}

