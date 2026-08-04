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

#ifndef TWL_SWITCH_BACKEND_H
#define TWL_SWITCH_BACKEND_H

#include "twl_internal.h"
#include <switch.h>

/* The default nwindow framebuffer is 1280x720; the system scales it to the
 * handheld screen or the docked TV output. */
#define TWL_SWITCH_SCREEN_WIDTH 1280
#define TWL_SWITCH_SCREEN_HEIGHT 720
#define TWL_SWITCH_MAX_BUFFERS 4

typedef struct {
  Framebuffer framebuffer;
  PadState pad;
  uint32_t frame_width;
  uint32_t frame_height;
  int32_t view_x;
  int32_t view_y;
  int32_t view_w;
  int32_t view_h;
  int32_t pointer_x;
  int32_t pointer_y;
  void *cleared_buffers[TWL_SWITCH_MAX_BUFFERS];
  uint32_t cleared_count;
  bool touching;
  bool fb_ready;
  bool pad_ready;
  bool frame_prepared;
  bool quit_pushed;
} TwlSwitch;

#endif
