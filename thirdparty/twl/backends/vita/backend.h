/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of TWL.
 *
 * TWL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TWL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with TWL. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TWL_VITA_BACKEND_H
#define TWL_VITA_BACKEND_H

#include "twl_internal.h"

#include <vita2d.h>

/* The Vita screen is 960x544; the game framebuffer is uploaded to a GPU texture
 * and vita2d scales it into an aspect-fit rectangle. The front touch panel
 * reports at double the screen resolution. */
#define TWL_VITA_SCREEN_WIDTH 960
#define TWL_VITA_SCREEN_HEIGHT 544
#define TWL_VITA_TOUCH_WIDTH 1920
#define TWL_VITA_TOUCH_HEIGHT 1088

typedef struct {
  vita2d_texture *texture;
  uint32_t *tex_data;
  uint32_t tex_stride_px;
  uint32_t frame_width;
  uint32_t frame_height;
  float scale;
  int32_t view_x;
  int32_t view_y;
  int32_t view_w;
  int32_t view_h;
  int32_t pointer_x;
  int32_t pointer_y;
  uint32_t prev_buttons;
  bool touching;
  bool cross_held;
  bool pointer_down;
  bool vita2d_ready;
} TwlVita;

#endif
