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

#include "render/dirty.h"

static bool sf_rectangles_touch(SfRect first, SfRect second) {
  return first.x <= second.x + second.width &&
    second.x <= first.x + first.width &&
    first.y <= second.y + second.height &&
    second.y <= first.y + first.height;
}

static SfRect sf_rectangle_union(SfRect first, SfRect second) {
  const int left = first.x < second.x ? first.x : second.x;
  const int top = first.y < second.y ? first.y : second.y;
  const int first_right = first.x + first.width;
  const int second_right = second.x + second.width;
  const int first_bottom = first.y + first.height;
  const int second_bottom = second.y + second.height;
  const int right = first_right > second_right ? first_right : second_right;
  const int bottom = first_bottom > second_bottom ? first_bottom : second_bottom;
  SfRect result;
  result.x = (int16_t) left;
  result.y = (int16_t) top;
  result.width = (int16_t) (right - left);
  result.height = (int16_t) (bottom - top);
  return result;
}

void sf_dirty_clear(SfDirtyRects *dirty) {
  if (!dirty) return;
  dirty->count = 0u;
  dirty->full = false;
}

void sf_dirty_add(
    SfDirtyRects *dirty, SfRect rectangle,
    uint16_t width, uint16_t height) {
  uint8_t index;
  int right;
  int bottom;
  if (!dirty || dirty->full || rectangle.width <= 0 || rectangle.height <= 0)
    return;
  right = rectangle.x + rectangle.width;
  bottom = rectangle.y + rectangle.height;
  if (rectangle.x < 0) rectangle.x = 0;
  if (rectangle.y < 0) rectangle.y = 0;
  if (right > width) right = width;
  if (bottom > height) bottom = height;
  rectangle.width = (int16_t) (right - rectangle.x);
  rectangle.height = (int16_t) (bottom - rectangle.y);
  if (rectangle.width <= 0 || rectangle.height <= 0) return;
  for (index = 0u; index < dirty->count;) {
    if (sf_rectangles_touch(dirty->rectangles[index], rectangle)) {
      rectangle = sf_rectangle_union(dirty->rectangles[index], rectangle);
      --dirty->count;
      dirty->rectangles[index] = dirty->rectangles[dirty->count];
      index = 0u;
    } else {
      ++index;
    }
  }
  if (dirty->count == SF_DIRTY_RECT_CAPACITY) {
    dirty->count = 0u;
    dirty->full = true;
    return;
  }
  dirty->rectangles[dirty->count++] = rectangle;
}
