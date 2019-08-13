//
//    Palette.cpp: Palette object implementation
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

#include <Palette.h>
#include <cstring>

using namespace SigDigger;

Palette::Palette()
{
  memset(this->bitmap, 0, sizeof(this->bitmap));
  this->thumbnail = QImage(
        SIGDIGGER_PALETTE_THUMB_WIDTH,
        SIGDIGGER_PALETTE_THUMB_HEIGHT,
        QImage::Format_RGB32);
}

Palette::Palette(const Suscan::Object &obj) : Palette()
{
  this->deserialize(obj);
}

Palette::Palette(std::string const &name) : Palette()
{
  this->name = name;
}

Palette::Palette(std::string const &name, qreal gradient[][3]) : Palette()
{
  unsigned int i;

  for (i = 0; i < SIGDIGGER_PALETTE_MAX_STOPS; ++i)
    this->gradient[i].setRgbF(
        gradient[i][0],
        gradient[i][1],
        gradient[i][2]);

  this->name = name;

  this->updateThumbnail();
}


void
Palette::updateThumbnail(void)
{
  int i, j;
  int index;
  QRgb rgb;
  /* Compose thumbnail */
  for (i = 0; i < SIGDIGGER_PALETTE_THUMB_WIDTH; ++i) {
    index = ((SIGDIGGER_PALETTE_MAX_STOPS - 1) * i)
        / (SIGDIGGER_PALETTE_THUMB_WIDTH - 1);

    rgb = this->gradient[static_cast<unsigned>(index)].rgb();

    for (j = 0; j < SIGDIGGER_PALETTE_THUMB_HEIGHT; ++j)
      this->thumbnail.setPixel(i, j, rgb);
  }
}

void
Palette::compose(void)
{
  unsigned char bit, byte;
  qreal alpha, r0, r1, g0, g1, b0, b1;

  int prev = -1;
  int i, j;

  for (i = 0; i < SIGDIGGER_PALETTE_MAX_STOPS; ++i) {
    byte = static_cast<unsigned char>(i >> 3);
    bit  = i & 7;

    if (this->bitmap[byte] & (1 << bit)) {
      /* Used color found! */
      if (prev == -1) {
        /* This is the first stop. Fill from 0 until this one */
        for (j = 0; j < i; ++j)
          this->gradient[j] = this->gradient[i];
      } else {
        /* Not the first stop. perform square root mixing */
        for (j = prev + 1; j < i; ++j) {
          alpha = static_cast<qreal>(j - prev) / static_cast<qreal>(i - prev);

          r0 = this->gradient[i].redF();
          r1 = this->gradient[prev].redF();

          g0 = this->gradient[i].greenF();
          g1 = this->gradient[prev].greenF();

          b0 = this->gradient[i].blueF();
          b1 = this->gradient[prev].blueF();

          this->gradient[j].setRgbF(
                sqrt(alpha * r0 * r0 + (1. - alpha) * r1 * r1),
                sqrt(alpha * g0 * g0 + (1. - alpha) * g1 * g1),
                sqrt(alpha * b0 * b0 + (1. - alpha) * b1 * b1));
        }
      }

      prev = i;
    }
  }

  if (prev != -1)
    for (j = prev + 1; j < SIGDIGGER_PALETTE_MAX_STOPS; ++j)
      this->gradient[j] = this->gradient[prev];

  this->updateThumbnail();
}

void
Palette::addStop(unsigned int stop, const QColor &color)
{
  unsigned char byte, bit;

  SU_ATTEMPT(stop < SIGDIGGER_PALETTE_MAX_STOPS);

  byte = static_cast<unsigned char>(stop >> 3);
  bit  = stop & 7;

  this->bitmap[byte] |= 1 << bit;
  this->gradient[stop] = color;
}

void
Palette::deserialize(const Suscan::Object &obj)
{
  Suscan::Object stops, entry;
  unsigned int i, count;
  int position;
  qreal red, green, blue;

  this->name = obj.getField("name").value();
  stops = obj.getField("stops");

  SU_ATTEMPT(stops.getType() == SUSCAN_OBJECT_TYPE_SET);

  memset(this->bitmap, 0, sizeof(this->bitmap));

  /* Traverse stop list */
  count = stops.length();

  for (i = 0; i < count; ++i) {
    try {
      QColor color;
      entry = stops[i];

      position = entry.get("position", -1);
      if (position < 0 || position > 255)
        continue;

      red   = static_cast<qreal>(entry.get("red", -1.f));
      green = static_cast<qreal>(entry.get("green", -1.f));
      blue  = static_cast<qreal>(entry.get("blue", -1.f));

      if (red < 0 || green < 0 || blue < 0)
        continue;

      if (red > 1 || green > 1 || blue > 1)
        continue;

      color.setRgbF(red, green, blue);

      this->addStop(static_cast<unsigned int>(position), color);
    } catch (Suscan::Exception &) {
    }
  }

  this->compose();
}
