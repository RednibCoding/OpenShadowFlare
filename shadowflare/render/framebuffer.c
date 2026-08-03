/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of OpenShadowFlare.
 *
 * OpenShadowFlare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * OpenShadowFlare is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
 */

#include "render/framebuffer.h"

uint16_t sf_rgb555(uint8_t red, uint8_t green, uint8_t blue) {
  return (uint16_t) ((red & 31u) | ((uint16_t) (green & 31u) << 5u) |
                     ((uint16_t) (blue & 31u) << 10u));
}

bool sf_framebuffer_init(
    SfFramebuffer *framebuffer, void *memory, size_t memory_size,
    uint16_t width, uint16_t height) {
  const size_t required = (size_t) width * (size_t) height * sizeof(uint16_t);
  if (!framebuffer || !memory || width == 0u || height == 0u ||
      required > memory_size || ((uintptr_t) memory % sizeof(uint16_t)) != 0u)
    return false;
  framebuffer->pixels = (uint16_t *) memory;
  framebuffer->width = width;
  framebuffer->height = height;
  framebuffer->stride = width;
  return true;
}

void sf_framebuffer_clear(SfFramebuffer *framebuffer, uint16_t color) {
  uint16_t y;
  if (!framebuffer || !framebuffer->pixels) return;
  for (y = 0u; y < framebuffer->height; ++y) {
    uint16_t *row = framebuffer->pixels + (size_t) y * framebuffer->stride;
    uint16_t x;
    for (x = 0u; x < framebuffer->width; ++x) row[x] = color;
  }
}

void sf_framebuffer_fill_rect(
    SfFramebuffer *framebuffer, int x, int y, int width, int height,
    uint16_t color) {
  int right;
  int bottom;
  int row_index;
  if (!framebuffer || !framebuffer->pixels || width <= 0 || height <= 0)
    return;
  right = x + width;
  bottom = y + height;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (right > framebuffer->width) right = framebuffer->width;
  if (bottom > framebuffer->height) bottom = framebuffer->height;
  if (x >= right || y >= bottom) return;
  for (row_index = y; row_index < bottom; ++row_index) {
    uint16_t *row = framebuffer->pixels +
      (size_t) row_index * framebuffer->stride;
    int column;
    for (column = x; column < right; ++column) row[column] = color;
  }
}
