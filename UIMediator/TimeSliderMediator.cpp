//
//    TimeSliderMediator.cpp: Coordinate time slider mediator signals
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

#include "UIMediator.h"
#include <QTimeSlider.h>

using namespace SigDigger;

void
UIMediator::setSourceTimeStart(struct timeval const &tv)
{
  m_ui->timeSlider->setStartTime(tv);
}

void
UIMediator::setSourceTimeEnd(struct timeval const &tv)
{
  m_ui->timeSlider->setEndTime(tv);
}

void
UIMediator::setTimeStamp(struct timeval const &tv)
{
  m_lastTimeStamp = tv;
  m_ui->timeSlider->setTimeStamp(tv);
}

void
UIMediator::connectTimeSlider(void)
{
  connect(
        m_ui->timeSlider,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onTimeStampChanged(void)));
}

void
UIMediator::onTimeStampChanged(void)
{
  emit seek(m_ui->timeSlider->getTimeStamp());
}

