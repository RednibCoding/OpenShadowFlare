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

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>

static void twl_vita_fit_viewport(TwlVita *vita) {
  const float sx = (float) TWL_VITA_SCREEN_WIDTH / (float) vita->frame_width;
  const float sy = (float) TWL_VITA_SCREEN_HEIGHT / (float) vita->frame_height;
  vita->scale = sx < sy ? sx : sy;
  vita->view_w = (int32_t) ((float) vita->frame_width * vita->scale);
  vita->view_h = (int32_t) ((float) vita->frame_height * vita->scale);
  vita->view_x = (TWL_VITA_SCREEN_WIDTH - vita->view_w) / 2;
  vita->view_y = (TWL_VITA_SCREEN_HEIGHT - vita->view_h) / 2;
}

size_t twl_backend_memory_alignment(void) {
  return _Alignof(TwlVita);
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  (void) config;
  return sizeof(TwlVita);
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  TwlVita *vita;
  if (!twl || !memory || memory_size < sizeof(TwlVita) || !config) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  vita = (TwlVita *) memory;
  vita->frame_width = config->width ? config->width : 640u;
  vita->frame_height = config->height ? config->height : 480u;

  vita2d_init();
  vita2d_set_vblank_wait(1);
  vita2d_set_clear_color(RGBA8(0, 0, 0, 255));

  vita->texture = vita2d_create_empty_texture_format(
    vita->frame_width, vita->frame_height,
    SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR);
  if (!vita->texture) {
    vita2d_fini();
    return TWL_RESULT_BACKEND_FAILURE;
  }
  vita2d_texture_set_filters(
    vita->texture, SCE_GXM_TEXTURE_FILTER_POINT, SCE_GXM_TEXTURE_FILTER_POINT);
  vita->tex_data = (uint32_t *) vita2d_texture_get_datap(vita->texture);
  vita->tex_stride_px =
    vita2d_texture_get_stride(vita->texture) / (uint32_t) sizeof(uint32_t);
  vita->vita2d_ready = true;

  twl_vita_fit_viewport(vita);
  vita->pointer_x = (int32_t) (vita->frame_width / 2u);
  vita->pointer_y = (int32_t) (vita->frame_height / 2u);

  sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
  sceTouchSetSamplingState(
    SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);

  twl_internal_set_display_size(twl, vita->frame_width, vita->frame_height);
  return TWL_RESULT_OK;
}

void twl_backend_shutdown(Twl *twl) {
  TwlVita *vita = twl ? (TwlVita *) twl->backend : NULL;
  if (!vita || !vita->vita2d_ready) {
    return;
  }
  if (vita->texture) {
    vita2d_free_texture(vita->texture);
    vita->texture = NULL;
  }
  vita2d_fini();
  vita->vita2d_ready = false;
}

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  (void) twl;
  return sceKernelGetProcessTimeWide();
}

void twl_backend_sleep_microseconds(Twl *twl, uint64_t duration) {
  (void) twl;
  sceKernelDelayThread((SceUInt) duration);
}
