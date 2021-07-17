//
//    Palette.h: Palette object
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
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

#ifndef PALETTE_H
#define PALETTE_H

#include <QColor>
#include <QPixmap>
#include <Suscan/Object.h>

#define SIGDIGGER_PALETTE_MAX_STOPS 256
#define SIGDIGGER_PALETTE_BITMAP_SZ ((SIGDIGGER_PALETTE_MAX_STOPS + 7) / 8)

#define SIGDIGGER_PALETTE_THUMB_WIDTH  64

#ifdef __APPLE__
#  define SIGDIGGER_PALETTE_THUMB_HEIGHT 10
#else
#  define SIGDIGGER_PALETTE_THUMB_HEIGHT 16
#endif // __APPLE__

namespace SigDigger {
  class Palette {
      std::string name;
      QColor gradient[SIGDIGGER_PALETTE_MAX_STOPS];
      uint8_t bitmap[SIGDIGGER_PALETTE_BITMAP_SZ];
      QImage thumbnail;

      void updateThumbnail(void);

    public:
      Palette();
      Palette(std::string const &name);
      Palette(std::string const &name, qreal gradient[][3]);
      Palette(const Suscan::Object &obj);
      void compose(void);
      void deserialize(const Suscan::Object &obj);
      void addStop(unsigned int stop, const QColor &color);

      const QImage &
      getThumbnail(void) const
      {
        return this->thumbnail;
      }

      std::string
      getName(void) const
      {
        return this->name;
      }

      const QColor *
      getGradient(void) const
      {
        return this->gradient;
      }
  };
}

#endif // PALETTE_H
