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

#include <gx2/draw.h>
#include <gx2/enum.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/sampler.h>
#include <gx2/shaders.h>
#include <gx2/texture.h>
#include <gx2r/draw.h>
#include <whb/gfx.h>
#include <whb/log.h>

#include <stddef.h>
#include <stdint.h>

/* Expand the RGB555 frame (red in the low 5 bits) into the RGBA8 texture. */
static void twl_wiiu_upload(TwlWiiU *wiiu, const TwlSurface *surface) {
  const uint16_t *src = (const uint16_t *) surface->pixels;
  uint8_t *dst = (uint8_t *) wiiu->texture.surface.image;
  const size_t src_stride = surface->stride_bytes / sizeof(uint16_t);
  const uint32_t dst_pitch = wiiu->texture.surface.pitch;
  const uint32_t width = wiiu->frame_width;
  const uint32_t height = wiiu->frame_height;
  uint32_t x;
  uint32_t y;
  if (!dst) {
    return;
  }
  for (y = 0; y < height; ++y) {
    const uint16_t *src_row = src + (size_t) y * src_stride;
    uint8_t *dst_row = dst + (size_t) y * dst_pitch * 4u;
    for (x = 0; x < width; ++x) {
      const uint16_t pixel = src_row[x];
      const uint8_t r5 = (uint8_t) (pixel & 0x1fu);
      const uint8_t g5 = (uint8_t) ((pixel >> 5) & 0x1fu);
      const uint8_t b5 = (uint8_t) ((pixel >> 10) & 0x1fu);
      uint8_t *texel = dst_row + (size_t) x * 4u;
      texel[0] = (uint8_t) ((r5 << 3) | (r5 >> 2));
      texel[1] = (uint8_t) ((g5 << 3) | (g5 >> 2));
      texel[2] = (uint8_t) ((b5 << 3) | (b5 >> 2));
      texel[3] = 0xffu;
    }
  }
  GX2Invalidate(
    GX2_INVALIDATE_MODE_CPU_TEXTURE, wiiu->texture.surface.image,
    wiiu->texture.surface.imageSize);
}

static void twl_wiiu_draw(TwlWiiU *wiiu) {
  GX2SetCullOnlyControl(GX2_FRONT_FACE_CCW, FALSE, FALSE);
  GX2SetDepthOnlyControl(FALSE, FALSE, GX2_COMPARE_FUNC_ALWAYS);
  GX2SetFetchShader(&wiiu->shader.fetchShader);
  GX2SetVertexShader(wiiu->shader.vertexShader);
  GX2SetPixelShader(wiiu->shader.pixelShader);
  GX2SetPixelTexture(&wiiu->texture, 0);
  GX2SetPixelSampler(&wiiu->sampler, 0);
  GX2RSetAttributeBuffer(
    &wiiu->position_buffer, 0, wiiu->position_buffer.elemSize, 0);
  GX2RSetAttributeBuffer(
    &wiiu->texcoord_buffer, 1, wiiu->texcoord_buffer.elemSize, 0);
  GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLE_STRIP, 4, 0, 1);
}

TwlResult twl_backend_prepare_frame(Twl *twl, const TwlSurface *surface) {
  static int logged;
  TwlWiiU *wiiu = twl ? (TwlWiiU *) twl->backend : NULL;
  if (!wiiu || !surface || !surface->pixels) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if (surface->format != TWL_PIXEL_RGB555 ||
      surface->width != wiiu->frame_width ||
      surface->height != wiiu->frame_height) {
    if (logged < 4) {
      WHBLogPrintf(
        "[twl-wiiu] prepare_frame rejected surface: fmt=%d %ux%u "
        "(expected %ux%u RGB555)",
        (int) surface->format, surface->width, surface->height,
        wiiu->frame_width, wiiu->frame_height);
      ++logged;
    }
    return TWL_RESULT_INVALID_ARGUMENT;
  }

  twl_wiiu_upload(wiiu, surface);

  WHBGfxBeginRender();

  WHBGfxBeginRenderTV();
  WHBGfxClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  twl_wiiu_draw(wiiu);
  WHBGfxFinishRenderTV();

  WHBGfxBeginRenderDRC();
  WHBGfxClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  twl_wiiu_draw(wiiu);
  WHBGfxFinishRenderDRC();

  return TWL_RESULT_OK;
}

TwlResult twl_backend_display_frame(Twl *twl) {
  TwlWiiU *wiiu = twl ? (TwlWiiU *) twl->backend : NULL;
  if (!wiiu) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  WHBGfxFinishRender();
  return TWL_RESULT_OK;
}
