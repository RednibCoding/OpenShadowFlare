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

#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <gx2/enum.h>
#include <gx2/sampler.h>
#include <gx2/surface.h>
#include <gx2/texture.h>
#include <gx2r/buffer.h>
#include <gx2r/surface.h>
#include <vpad/input.h>
#include <whb/file.h>
#include <whb/gfx.h>
#include <whb/log.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/proc.h>

#include <stdint.h>
#include <string.h>

static const float twl_wiiu_texcoords[] = {
  0.0f, 1.0f,
  1.0f, 1.0f,
  0.0f, 0.0f,
  1.0f, 0.0f,
};

static void twl_wiiu_letterbox_positions(
    float *positions, uint32_t content_w, uint32_t content_h,
    uint32_t screen_w, uint32_t screen_h) {
  float scale_x = 1.0f;
  float scale_y = 1.0f;
  size_t vertex;
  if (content_w > 0u && content_h > 0u && screen_w > 0u && screen_h > 0u) {
    const uint64_t content = (uint64_t) content_w * screen_h;
    const uint64_t screen = (uint64_t) screen_w * content_h;
    if (screen > content) {
      scale_x = (float) content / (float) screen;
    } else if (content > screen) {
      scale_y = (float) screen / (float) content;
    }
  }
  {
    const float xs[4] = {-scale_x, scale_x, -scale_x, scale_x};
    const float ys[4] = {-scale_y, -scale_y, scale_y, scale_y};
    for (vertex = 0u; vertex < 4u; ++vertex) {
      positions[vertex * 4u + 0u] = xs[vertex];
      positions[vertex * 4u + 1u] = ys[vertex];
      positions[vertex * 4u + 2u] = 0.0f;
      positions[vertex * 4u + 3u] = 1.0f;
    }
  }
}

static bool twl_wiiu_create_buffer(
    GX2RBuffer *buffer, uint32_t elem_size, uint32_t elem_count,
    const void *data) {
  void *mapped;
  memset(buffer, 0, sizeof(*buffer));
  buffer->flags = GX2R_RESOURCE_BIND_VERTEX_BUFFER |
                  GX2R_RESOURCE_USAGE_CPU_READ |
                  GX2R_RESOURCE_USAGE_CPU_WRITE |
                  GX2R_RESOURCE_USAGE_GPU_READ;
  buffer->elemSize = elem_size;
  buffer->elemCount = elem_count;
  if (!GX2RCreateBuffer(buffer)) {
    return false;
  }
  mapped = GX2RLockBufferEx(buffer, 0);
  if (!mapped) {
    return false;
  }
  memcpy(mapped, data, (size_t) elem_size * elem_count);
  GX2RUnlockBufferEx(buffer, 0);
  return true;
}

static bool twl_wiiu_create_texture(
    TwlWiiU *wiiu, uint32_t width, uint32_t height) {
  GX2Texture *texture = &wiiu->texture;
  memset(texture, 0, sizeof(*texture));
  texture->surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
  texture->surface.width = width;
  texture->surface.height = height;
  texture->surface.depth = 1;
  texture->surface.mipLevels = 1;
  texture->surface.format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
  texture->surface.aa = GX2_AA_MODE1X;
  texture->surface.use = GX2_SURFACE_USE_TEXTURE;
  texture->surface.tileMode = GX2_TILE_MODE_LINEAR_ALIGNED;
  texture->surface.swizzle = 0;
  texture->viewFirstMip = 0;
  texture->viewNumMips = 1;
  texture->viewFirstSlice = 0;
  texture->viewNumSlices = 1;
  texture->compMap = 0x00010203u;
  if (!GX2RCreateSurface(
        &texture->surface,
        GX2R_RESOURCE_BIND_TEXTURE | GX2R_RESOURCE_USAGE_CPU_WRITE |
          GX2R_RESOURCE_USAGE_GPU_READ)) {
    return false;
  }
  GX2InitTextureRegs(texture);
  return true;
}

static void twl_wiiu_teardown(TwlWiiU *wiiu) {
  if (!wiiu) {
    return;
  }
  if (wiiu->texture_ready) {
    GX2RDestroySurfaceEx(&wiiu->texture.surface, 0);
    wiiu->texture_ready = false;
  }
  if (wiiu->tv_position_buffer.flags) {
    GX2RDestroyBufferEx(&wiiu->tv_position_buffer, 0);
  }
  if (wiiu->drc_position_buffer.flags) {
    GX2RDestroyBufferEx(&wiiu->drc_position_buffer, 0);
  }
  if (wiiu->texcoord_buffer.flags) {
    GX2RDestroyBufferEx(&wiiu->texcoord_buffer, 0);
  }
  if (wiiu->shader_ready) {
    WHBGfxFreeShaderGroup(&wiiu->shader);
    wiiu->shader_ready = false;
  }
  if (wiiu->gfx_ready) {
    WHBGfxShutdown();
    wiiu->gfx_ready = false;
  }
  if (wiiu->vpad_ready) {
    VPADShutdown();
    wiiu->vpad_ready = false;
  }
  if (wiiu->proc_ready) {
    WHBProcShutdown();
    wiiu->proc_ready = false;
  }
}

size_t twl_backend_memory_alignment(void) {
  return _Alignof(TwlWiiU);
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  (void) config;
  return sizeof(TwlWiiU);
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  TwlWiiU *wiiu;
  char *shader_file;
  if (!twl || !memory || memory_size < sizeof(TwlWiiU) || !config) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  wiiu = (TwlWiiU *) memory;
  wiiu->frame_width = config->width ? config->width : 640u;
  wiiu->frame_height = config->height ? config->height : 480u;

  WHBProcInit();
  wiiu->proc_ready = true;
  VPADInit();
  wiiu->vpad_ready = true;
  WHBLogCafeInit();
  WHBLogUdpInit();

  if (!WHBGfxInit()) {
    WHBLogPrint("[twl-wiiu] ERROR: WHBGfxInit failed");
    twl_wiiu_teardown(wiiu);
    return TWL_RESULT_BACKEND_FAILURE;
  }
  wiiu->gfx_ready = true;

  shader_file = WHBReadWholeFile(TWL_WIIU_SHADER_PATH, NULL);
  if (!shader_file) {
    WHBLogPrintf(
      "[twl-wiiu] ERROR: could not read shader %s", TWL_WIIU_SHADER_PATH);
    twl_wiiu_teardown(wiiu);
    return TWL_RESULT_BACKEND_FAILURE;
  }
  if (!WHBGfxLoadGFDShaderGroup(&wiiu->shader, 0, shader_file)) {
    WHBLogPrint("[twl-wiiu] ERROR: WHBGfxLoadGFDShaderGroup failed");
    WHBFreeWholeFile(shader_file);
    twl_wiiu_teardown(wiiu);
    return TWL_RESULT_BACKEND_FAILURE;
  }
  WHBGfxInitShaderAttribute(
    &wiiu->shader, "aPosition", 0, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
  WHBGfxInitShaderAttribute(
    &wiiu->shader, "aTexCoord", 1, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);
  WHBGfxInitFetchShader(&wiiu->shader);
  WHBFreeWholeFile(shader_file);
  wiiu->shader_ready = true;

  {
    const GX2ColorBuffer *tv = WHBGfxGetTVColourBuffer();
    const GX2ColorBuffer *drc = WHBGfxGetDRCColourBuffer();
    float tv_positions[16];
    float drc_positions[16];
    twl_wiiu_letterbox_positions(
      tv_positions, wiiu->frame_width, wiiu->frame_height,
      tv ? tv->surface.width : wiiu->frame_width,
      tv ? tv->surface.height : wiiu->frame_height);
    twl_wiiu_letterbox_positions(
      drc_positions, wiiu->frame_width, wiiu->frame_height,
      drc ? drc->surface.width : wiiu->frame_width,
      drc ? drc->surface.height : wiiu->frame_height);
    if (!twl_wiiu_create_buffer(
          &wiiu->tv_position_buffer, 4u * sizeof(float), 4u, tv_positions) ||
        !twl_wiiu_create_buffer(
          &wiiu->drc_position_buffer, 4u * sizeof(float), 4u, drc_positions) ||
        !twl_wiiu_create_buffer(
          &wiiu->texcoord_buffer, 2u * sizeof(float), 4u, twl_wiiu_texcoords)) {
      WHBLogPrint("[twl-wiiu] ERROR: attribute buffer creation failed");
      twl_wiiu_teardown(wiiu);
      return TWL_RESULT_INSUFFICIENT_MEMORY;
    }
  }

  if (!twl_wiiu_create_texture(
        wiiu, wiiu->frame_width, wiiu->frame_height)) {
    WHBLogPrint("[twl-wiiu] ERROR: texture creation failed");
    twl_wiiu_teardown(wiiu);
    return TWL_RESULT_INSUFFICIENT_MEMORY;
  }
  wiiu->texture_ready = true;

  GX2InitSampler(
    &wiiu->sampler, GX2_TEX_CLAMP_MODE_CLAMP, GX2_TEX_XY_FILTER_MODE_LINEAR);

  twl_internal_set_display_size(twl, wiiu->frame_width, wiiu->frame_height);
  return TWL_RESULT_OK;
}

void twl_backend_shutdown(Twl *twl) {
  TwlWiiU *wiiu = twl ? (TwlWiiU *) twl->backend : NULL;
  if (!wiiu) {
    return;
  }
  twl_wiiu_teardown(wiiu);
  WHBLogCafeDeinit();
  WHBLogUdpDeinit();
}

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  (void) twl;
  return (uint64_t) OSTicksToMicroseconds(OSGetSystemTime());
}

void twl_backend_sleep_microseconds(Twl *twl, uint64_t duration) {
  (void) twl;
  OSSleepTicks(OSMicrosecondsToTicks(duration));
}
