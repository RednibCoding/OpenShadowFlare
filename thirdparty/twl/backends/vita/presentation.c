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
 */

#include "backend.h"

#include <stdint.h>

TwlResult twl_backend_prepare_frame(Twl *twl, const TwlSurface *surface) {
  TwlVita *vita = twl ? (TwlVita *) twl->backend : NULL;
  const uint16_t *src;
  size_t src_stride;
  uint32_t y;
  uint32_t x;
  if (!vita || !surface || !surface->pixels) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if (surface->format != TWL_PIXEL_RGB555 ||
      surface->width != vita->frame_width ||
      surface->height != vita->frame_height) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if (!vita->vita2d_ready || !vita->tex_data) {
    return TWL_RESULT_OK;
  }

  src = (const uint16_t *) surface->pixels;
  src_stride = surface->stride_bytes / sizeof(uint16_t);
  for (y = 0; y < vita->frame_height; ++y) {
    const uint16_t *source_row = src + (size_t) y * src_stride;
    uint32_t *dest_row = vita->tex_data + (size_t) y * vita->tex_stride_px;
    for (x = 0; x < vita->frame_width; ++x) {
      const uint16_t pixel = source_row[x];
      const uint32_t r5 = (uint32_t) (pixel & 0x1fu);
      const uint32_t g5 = (uint32_t) ((pixel >> 5) & 0x1fu);
      const uint32_t b5 = (uint32_t) ((pixel >> 10) & 0x1fu);
      const uint32_t r8 = (r5 << 3) | (r5 >> 2);
      const uint32_t g8 = (g5 << 3) | (g5 >> 2);
      const uint32_t b8 = (b5 << 3) | (b5 >> 2);
      dest_row[x] = 0xff000000u | (b8 << 16) | (g8 << 8) | r8;
    }
  }
  return TWL_RESULT_OK;
}

TwlResult twl_backend_display_frame(Twl *twl) {
  TwlVita *vita = twl ? (TwlVita *) twl->backend : NULL;
  if (!vita || !vita->vita2d_ready || !vita->texture) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  vita2d_start_drawing();
  vita2d_clear_screen();
  vita2d_draw_texture_scale(
    vita->texture, (float) vita->view_x, (float) vita->view_y,
    vita->scale, vita->scale);
  vita2d_end_drawing();
  vita2d_swap_buffers();
  return TWL_RESULT_OK;
}
