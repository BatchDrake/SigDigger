//
//    Averager.h: Simple PSD averager
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
#ifndef AVERAGER_H
#define AVERAGER_H

#include <Suscan/Messages/PSDMessage.h>

namespace SigDigger {
  class Averager {
    float *last = nullptr;
    unsigned long bufsiz = 0;
    float alpha = 1.;

  public:
    void feed(Suscan::PSDMessage const &m);
    void setAlpha(float alpha);
    ~Averager(void);

    float *
    get(void) const
    {
      return this->last;
    }

    unsigned long
    size(void) const
    {
      return this->bufsiz;
    }

    void
    reset(void)
    {
      if (this->last != nullptr) {
        delete [] this->last;
        this->bufsiz = 0;
      }
    }
  };
}
#endif // AVERAGER_H
