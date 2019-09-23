/*
//
//    chopper.c.h: C implementation of the symbol correlator
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

#include "chopper.h"

#include <stdlib.h>
#include <sigutils/log.h>
#include <string.h>

chopper_t *
chopper_new(const struct chopper_params *params)
{
  chopper_t *self = NULL;
  unsigned int i, j;

  SU_TRYCATCH(self = calloc(1, sizeof(chopper_t)), goto fail);

  self->seq_len = params->seq_len;

  SU_TRYCATCH(
      self->buffer = calloc(params->seq_len, sizeof(uint8_t)),
      goto fail);

  SU_TRYCATCH(
        self->seq_shift = calloc(params->seq_len, sizeof(uint8_t *)),
        goto fail);

  for (i = 0; i < params->seq_len; ++i) {
    SU_TRYCATCH(
        self->seq_shift[i] = calloc(params->seq_len, sizeof(uint8_t)),
        goto fail);

    for (j = 0; j < params->seq_len; ++j)
      self->seq_shift[i][j] =
          params->seq_data[(j + params->seq_len - i) % params->seq_len];
  }

  return self;

fail:
  if (self != NULL)
    chopper_destroy(self);

  return NULL;
}

static inline int
chopper_feed_internal(chopper_t *self, uint8_t symbol)
{
  self->buffer[self->p++] = symbol;
  if (self->p == self->seq_len) {
    self->p = 0;
    self->ready = 1;
  }

  if (self->count < self->seq_len)
    self->full = ++self->count == self->seq_len;

  if (self->full
      && memcmp(self->seq_shift[self->p], self->buffer, self->seq_len) == 0) {
    self->full = self->count = 0;
    return 1;
  }

  return 0;
}

int
chopper_feed(chopper_t *self, uint8_t symbol)
{
  return chopper_feed_internal(self, symbol);
}

int
chopper_feed_bulk(
    chopper_t *self,
    const uint8_t *data,
    unsigned int len,
    int *flag)
{
  unsigned int i;

  for (i = 0; i < len; ++i) {
    if (chopper_feed_internal(self, *data++)) {
      *flag = 1;
      return (int) i + 1;
    }
  }

  *flag = 0;
  return (int) len;
}

void
chopper_destroy(chopper_t *self)
{
  unsigned int i;

  if (self->buffer != NULL)
    free(self->buffer);

  if (self->seq_shift != NULL) {
    for (i = 0; i < self->seq_len; ++i)
      if (self->seq_shift[i] != NULL)
        free(self->seq_shift[i]);

    free(self->seq_shift);
  }

  free(self);
}

