//
//    Averager.cpp: PSD averager
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

#include "Averager.h"

using namespace SigDigger;

void
Averager::feed(Suscan::PSDMessage const &m)
{
  bool blend = this->alpha != 1.f;

  if (this->last == nullptr || m.size() != this->bufsiz) {
    try {
      float *buf = new float[m.size()];
      if (this->last != nullptr)
        delete [] this->last;
      this->last = buf;
      this->bufsiz = m.size();
      blend = false;
    } catch (std::bad_alloc &) {
      throw Suscan::Exception("Failed to allocate PSD buffer");
    }
  }

  if (blend) {
    const SUFLOAT *original = m.get();
    for (unsigned long i = 0; i < this->bufsiz; ++i)
      this->last[i] += this->alpha * (original[i] - this->last[i]);
  } else {
    memcpy(this->last, m.get(), m.size() * sizeof(float));
  }
}

void
Averager::setAlpha(float alpha)
{
  this->alpha = alpha;
}

Averager::~Averager(void)
{
  this->reset();
}

