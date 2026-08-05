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

#include <string.h>

static const struct {
  uint32_t mask;
  TwlKey key;
} twl_vita_button_keys[] = {
  {SCE_CTRL_UP, TWL_KEY_UP},
  {SCE_CTRL_DOWN, TWL_KEY_DOWN},
  {SCE_CTRL_LEFT, TWL_KEY_LEFT},
  {SCE_CTRL_RIGHT, TWL_KEY_RIGHT},
  {SCE_CTRL_CROSS, TWL_KEY_RETURN},
  {SCE_CTRL_START, TWL_KEY_RETURN},
  {SCE_CTRL_CIRCLE, TWL_KEY_ESCAPE},
  {SCE_CTRL_SELECT, TWL_KEY_ESCAPE},
  {SCE_CTRL_TRIANGLE, TWL_KEY_I},
  {SCE_CTRL_RTRIGGER, TWL_KEY_R},
};

static int32_t twl_vita_clamp(int32_t value, int32_t low, int32_t high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

static int32_t twl_vita_axis_delta(uint8_t axis) {
  const int32_t value = (int32_t) axis - 128;
  if (value > -32 && value < 32) return 0;
  return value / 16;
}

static void twl_vita_push(Twl *twl, TwlEvent *event) {
  event->timestamp_us = twl_backend_time_microseconds(twl);
  twl_internal_push_event(twl, event);
}

static void twl_vita_push_key(Twl *twl, TwlEventType type, TwlKey key) {
  TwlEvent event;
  twl_internal_zero(&event, sizeof(event));
  event.type = type;
  event.key = key;
  twl_vita_push(twl, &event);
}

static void twl_vita_push_pointer(
    Twl *twl, TwlEventType type, int32_t x, int32_t y) {
  TwlEvent event;
  twl_internal_zero(&event, sizeof(event));
  event.type = type;
  event.x = x;
  event.y = y;
  event.button = 1u;
  twl_vita_push(twl, &event);
}

void twl_backend_pump_events(Twl *twl) {
  TwlVita *vita = twl ? (TwlVita *) twl->backend : NULL;
  SceCtrlData pad;
  SceTouchData touch;
  uint32_t buttons;
  uint32_t pressed;
  uint32_t released;
  int32_t next_x;
  int32_t next_y;
  bool want_down;
  size_t index;
  if (!vita) {
    return;
  }

  memset(&pad, 0, sizeof(pad));
  sceCtrlPeekBufferPositive(0, &pad, 1);
  buttons = pad.buttons;
  pressed = buttons & ~vita->prev_buttons;
  released = ~buttons & vita->prev_buttons;
  vita->prev_buttons = buttons;

  for (index = 0;
       index < sizeof(twl_vita_button_keys) / sizeof(twl_vita_button_keys[0]);
       ++index) {
    const uint32_t mask = twl_vita_button_keys[index].mask;
    if (pressed & mask) {
      twl_vita_push_key(twl, TWL_EVENT_KEY_DOWN, twl_vita_button_keys[index].key);
    }
    if (released & mask) {
      twl_vita_push_key(twl, TWL_EVENT_KEY_UP, twl_vita_button_keys[index].key);
    }
  }

  next_x = vita->pointer_x;
  next_y = vita->pointer_y;
  memset(&touch, 0, sizeof(touch));
  if (sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1) >= 0 &&
      touch.reportNum > 0) {
    const int32_t view_w = vita->view_w > 0 ? vita->view_w : 1;
    const int32_t view_h = vita->view_h > 0 ? vita->view_h : 1;
    const int32_t screen_x = (int32_t) touch.report[0].x *
      TWL_VITA_SCREEN_WIDTH / TWL_VITA_TOUCH_WIDTH;
    const int32_t screen_y = (int32_t) touch.report[0].y *
      TWL_VITA_SCREEN_HEIGHT / TWL_VITA_TOUCH_HEIGHT;
    next_x = twl_vita_clamp(
      (screen_x - vita->view_x) * (int32_t) vita->frame_width / view_w,
      0, (int32_t) vita->frame_width - 1);
    next_y = twl_vita_clamp(
      (screen_y - vita->view_y) * (int32_t) vita->frame_height / view_h,
      0, (int32_t) vita->frame_height - 1);
    vita->touching = true;
  } else {
    vita->touching = false;
    next_x = twl_vita_clamp(
      next_x + twl_vita_axis_delta(pad.lx), 0, (int32_t) vita->frame_width - 1);
    next_y = twl_vita_clamp(
      next_y + twl_vita_axis_delta(pad.ly), 0, (int32_t) vita->frame_height - 1);
  }

  if (next_x != vita->pointer_x || next_y != vita->pointer_y) {
    vita->pointer_x = next_x;
    vita->pointer_y = next_y;
    twl_vita_push_pointer(twl, TWL_EVENT_POINTER_MOVE, next_x, next_y);
  }

  vita->cross_held = (buttons & SCE_CTRL_CROSS) != 0u;
  want_down = vita->touching || vita->cross_held;
  if (want_down && !vita->pointer_down) {
    twl_vita_push_pointer(
      twl, TWL_EVENT_POINTER_DOWN, vita->pointer_x, vita->pointer_y);
    vita->pointer_down = true;
  } else if (!want_down && vita->pointer_down) {
    twl_vita_push_pointer(
      twl, TWL_EVENT_POINTER_UP, vita->pointer_x, vita->pointer_y);
    vita->pointer_down = false;
  }
}
