/*
//
//    chopper.h: C implementation of the symbol correlator
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
*/

#ifndef CHOPPER_H
#define CHOPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct chopper_params {
  const uint8_t *seq_data;
  unsigned int seq_len;
};

struct chopper {
  uint8_t **seq_shift;
  unsigned int seq_len;
  uint8_t *buffer;

  unsigned int p;
  unsigned int ready;
  unsigned int count;
  unsigned int full;
};

typedef struct chopper chopper_t;

chopper_t *chopper_new(const struct chopper_params *params);

int chopper_feed(chopper_t *self, uint8_t symbol);
int chopper_feed_bulk(
    chopper_t *self,
    const uint8_t *data,
    unsigned int len,
    int *flag);

static inline int
chopper_is_ready(const chopper_t *self)
{
  return self->ready;
}

static inline uint8_t
chopper_read(const chopper_t *self)
{
  return self->buffer[self->p];
}

void chopper_destroy(chopper_t *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MAIN_INCLUDE_H */
