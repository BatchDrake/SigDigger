//
//    HistogramFeeder.cpp: Translate central frequency
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
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

#include <HistogramFeeder.h>

using namespace SigDigger;

HistogramFeeder::HistogramFeeder(
    SamplingProperties const &props,
    QObject *parent) : CancellableTask(parent)
{
  this->properties = props;
}

HistogramFeeder::~HistogramFeeder()
{
}

bool
HistogramFeeder::work(void)
{
  size_t amount = this->properties.length - this->p;
  size_t p = this->p;
  unsigned int q = 0;

  if (amount > SIGDIGGER_HISTOGRAM_FEEDER_BLOCK_LENGTH)
    amount = SIGDIGGER_HISTOGRAM_FEEDER_BLOCK_LENGTH;

  switch (this->properties.space) {
    case AMPLITUDE:
      while (amount--)
        this->block[q++] = SU_C_ABS(this->properties.data[p++]);
      break;

    case PHASE:
      while (amount--)
        this->block[q++] = SU_C_ARG(this->properties.data[p++]);
      break;

    case FREQUENCY:
      while (amount--) {
        if (p > 0)
          this->block[q++] = SU_C_ARG(
              this->properties.data[p]
              * SU_C_CONJ(this->properties.data[p - 1]));
        ++p;
      }

      break;
  }


  this->p = p;

  this->setStatus("Measuring ("
                  + QString::number(p)
                  + "/"
                  + QString::number(this->properties.length)
                  + ")...");

  this->setProgress(
        static_cast<qreal>(p) / static_cast<qreal>(this->properties.length));

  emit data(this->block, q);

  if (this->p < this->properties.length)
    return true;

  emit done();
  return false;
}

void
HistogramFeeder::cancel(void)
{
  emit cancelled();
}
