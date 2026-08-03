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

#ifndef SHADOWFLARE_RENDER_DIRTY_H
#define SHADOWFLARE_RENDER_DIRTY_H

#include "render/renderer.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_DIRTY_RECT_CAPACITY 16u

typedef struct SfDirtyRects {
  SfRect rectangles[SF_DIRTY_RECT_CAPACITY];
  uint8_t count;
  bool full;
} SfDirtyRects;

void sf_dirty_clear(SfDirtyRects *dirty);
void sf_dirty_add(
  SfDirtyRects *dirty, SfRect rectangle, uint16_t width, uint16_t height);

#endif
