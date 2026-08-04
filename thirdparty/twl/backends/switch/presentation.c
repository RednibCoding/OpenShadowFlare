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
  TwlSwitch *sw = twl ? (TwlSwitch *) twl->backend : NULL;
  uint32_t stride;
  uint32_t *output;
  uint32_t output_stride;
  const uint16_t *src;
  size_t src_stride;
  int32_t y;
  int32_t x;
  if (!sw || !sw->fb_ready || !surface || !surface->pixels) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if (surface->format != TWL_PIXEL_RGB555 ||
      surface->width != sw->frame_width ||
      surface->height != sw->frame_height) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }

  output = (uint32_t *) framebufferBegin(&sw->framebuffer, &stride);
  if (!output ||
      stride < (uint32_t) (TWL_SWITCH_SCREEN_WIDTH * sizeof(uint32_t))) {
    if (output) framebufferEnd(&sw->framebuffer);
    return TWL_RESULT_BACKEND_FAILURE;
  }
  output_stride = stride / (uint32_t) sizeof(uint32_t);
  src = (const uint16_t *) surface->pixels;
  src_stride = surface->stride_bytes / sizeof(uint16_t);

  for (y = 0; y < TWL_SWITCH_SCREEN_HEIGHT; ++y) {
    uint32_t *row = output + (uint32_t) y * output_stride;
    for (x = 0; x < TWL_SWITCH_SCREEN_WIDTH; ++x) {
      row[x] = RGBA8(0, 0, 0, 255);
    }
  }

  for (y = 0; y < sw->view_h; ++y) {
    const int32_t source_y = y * (int32_t) sw->frame_height / sw->view_h;
    uint32_t *row =
      output + (uint32_t) (sw->view_y + y) * output_stride +
      (uint32_t) sw->view_x;
    const uint16_t *source_row = src + (size_t) source_y * src_stride;
    for (x = 0; x < sw->view_w; ++x) {
      const int32_t source_x = x * (int32_t) sw->frame_width / sw->view_w;
      const uint16_t pixel = source_row[source_x];
      const uint8_t r5 = (uint8_t) (pixel & 0x1fu);
      const uint8_t g5 = (uint8_t) ((pixel >> 5) & 0x1fu);
      const uint8_t b5 = (uint8_t) ((pixel >> 10) & 0x1fu);
      row[x] = RGBA8(
        (uint8_t) ((r5 << 3) | (r5 >> 2)),
        (uint8_t) ((g5 << 3) | (g5 >> 2)),
        (uint8_t) ((b5 << 3) | (b5 >> 2)),
        255);
    }
  }

  sw->frame_prepared = true;
  return TWL_RESULT_OK;
}

TwlResult twl_backend_display_frame(Twl *twl) {
  TwlSwitch *sw = twl ? (TwlSwitch *) twl->backend : NULL;
  if (!sw || !sw->fb_ready) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if (sw->frame_prepared) {
    framebufferEnd(&sw->framebuffer);
    sw->frame_prepared = false;
  }
  return TWL_RESULT_OK;
}
