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

/*
 * Switch window/lifecycle backend. libnx presents through a linear nwindow
 * framebuffer that the CPU writes directly, so there is no shader or GPU setup;
 * the finished RGB555 frame is scaled into an aspect-fit rectangle in
 * presentation.c. HID is set up here and read in input.c.
 */

#include "backend.h"

static void twl_switch_fit_viewport(TwlSwitch *sw) {
  const int32_t content_w = (int32_t) sw->frame_width;
  const int32_t content_h = (int32_t) sw->frame_height;
  if ((int64_t) content_w * TWL_SWITCH_SCREEN_HEIGHT >=
      (int64_t) content_h * TWL_SWITCH_SCREEN_WIDTH) {
    sw->view_w = TWL_SWITCH_SCREEN_WIDTH;
    sw->view_h = content_h * TWL_SWITCH_SCREEN_WIDTH / content_w;
  } else {
    sw->view_h = TWL_SWITCH_SCREEN_HEIGHT;
    sw->view_w = content_w * TWL_SWITCH_SCREEN_HEIGHT / content_h;
  }
  sw->view_x = (TWL_SWITCH_SCREEN_WIDTH - sw->view_w) / 2;
  sw->view_y = (TWL_SWITCH_SCREEN_HEIGHT - sw->view_h) / 2;
}

size_t twl_backend_memory_alignment(void) {
  return _Alignof(TwlSwitch);
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  (void) config;
  return sizeof(TwlSwitch);
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  TwlSwitch *sw;
  if (!twl || !memory || memory_size < sizeof(TwlSwitch) || !config) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  sw = (TwlSwitch *) memory;
  sw->frame_width = config->width ? config->width : 640u;
  sw->frame_height = config->height ? config->height : 480u;

  if (R_FAILED(framebufferCreate(
        &sw->framebuffer, nwindowGetDefault(),
        TWL_SWITCH_SCREEN_WIDTH, TWL_SWITCH_SCREEN_HEIGHT,
        PIXEL_FORMAT_RGBA_8888, 2)) ||
      R_FAILED(framebufferMakeLinear(&sw->framebuffer))) {
    framebufferClose(&sw->framebuffer);
    return TWL_RESULT_BACKEND_FAILURE;
  }
  sw->fb_ready = true;

  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&sw->pad);
  hidInitializeTouchScreen();
  sw->pad_ready = true;
  sw->pointer_x = (int32_t) (sw->frame_width / 2u);
  sw->pointer_y = (int32_t) (sw->frame_height / 2u);

  twl_switch_fit_viewport(sw);
  twl_internal_set_display_size(twl, sw->frame_width, sw->frame_height);
  return TWL_RESULT_OK;
}

void twl_backend_shutdown(Twl *twl) {
  TwlSwitch *sw = twl ? (TwlSwitch *) twl->backend : NULL;
  if (!sw) {
    return;
  }
  if (sw->fb_ready) {
    framebufferClose(&sw->framebuffer);
    sw->fb_ready = false;
  }
}

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  (void) twl;
  return armTicksToNs(armGetSystemTick()) / UINT64_C(1000);
}

void twl_backend_sleep_microseconds(Twl *twl, uint64_t duration) {
  (void) twl;
  svcSleepThread((int64_t) (duration * UINT64_C(1000)));
}
