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
 * Wii U GamePad (VPAD) input. The touchscreen drives the game pointer; the
 * buttons map to the key events the runtime already understands, so no game
 * code changes are needed. WHBProcIsRunning also services ProcUI and reports
 * when the user leaves via the HOME menu.
 */

#include "backend.h"

#include <vpad/input.h>
#include <whb/proc.h>

#include <string.h>

/* The GamePad's calibrated touch resolution. The game frame is stretched to
 * fill this screen, so touch coordinates scale linearly to frame coordinates. */
#define TWL_WIIU_TP_WIDTH 854
#define TWL_WIIU_TP_HEIGHT 480

static const struct {
  uint32_t mask;
  TwlKey key;
} twl_wiiu_button_keys[] = {
  {VPAD_BUTTON_UP, TWL_KEY_UP},
  {VPAD_BUTTON_DOWN, TWL_KEY_DOWN},
  {VPAD_BUTTON_LEFT, TWL_KEY_LEFT},
  {VPAD_BUTTON_RIGHT, TWL_KEY_RIGHT},
  {VPAD_BUTTON_A, TWL_KEY_RETURN},
  {VPAD_BUTTON_PLUS, TWL_KEY_RETURN},
  {VPAD_BUTTON_B, TWL_KEY_ESCAPE},
  {VPAD_BUTTON_MINUS, TWL_KEY_ESCAPE},
  {VPAD_BUTTON_X, TWL_KEY_I},
  {VPAD_BUTTON_R, TWL_KEY_R},
};

static int32_t twl_wiiu_clamp(int32_t value, int32_t low, int32_t high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

static void twl_wiiu_push(Twl *twl, TwlEvent *event) {
  event->timestamp_us = twl_backend_time_microseconds(twl);
  twl_internal_push_event(twl, event);
}

static void twl_wiiu_push_key(Twl *twl, TwlEventType type, TwlKey key) {
  TwlEvent event;
  twl_internal_zero(&event, sizeof(event));
  event.type = type;
  event.key = key;
  twl_wiiu_push(twl, &event);
}

static void twl_wiiu_push_pointer(
    Twl *twl, TwlEventType type, int32_t x, int32_t y) {
  TwlEvent event;
  twl_internal_zero(&event, sizeof(event));
  event.type = type;
  event.x = x;
  event.y = y;
  event.button = 1u;
  twl_wiiu_push(twl, &event);
}

/* There is no keyboard, so ZL fills the fixed name "Player" one code point at a
 * time, matching the TWL_EVENT_TEXT contract. */
static void twl_wiiu_push_text(Twl *twl, const char *text) {
  const char *cursor;
  for (cursor = text; *cursor; ++cursor) {
    TwlEvent event;
    twl_internal_zero(&event, sizeof(event));
    event.type = TWL_EVENT_TEXT;
    event.codepoint = (uint32_t) (unsigned char) *cursor;
    twl_wiiu_push(twl, &event);
  }
}

static void twl_wiiu_read_touch(
    Twl *twl, TwlWiiU *wiiu, VPADStatus *status) {
  VPADTouchData touch;
  memset(&touch, 0, sizeof(touch));
  VPADGetTPCalibratedPointEx(
    VPAD_CHAN_0, VPAD_TP_854X480, &touch, &status->tpNormal);
  if (touch.touched) {
    const int32_t frame_w = (int32_t) wiiu->frame_width;
    const int32_t frame_h = (int32_t) wiiu->frame_height;
    int32_t content_w = TWL_WIIU_TP_WIDTH;
    int32_t content_h = TWL_WIIU_TP_HEIGHT;
    int32_t origin_x = 0;
    int32_t origin_y = 0;
    int32_t x;
    int32_t y;
    if ((int64_t) TWL_WIIU_TP_WIDTH * frame_h >=
        (int64_t) TWL_WIIU_TP_HEIGHT * frame_w) {
      content_w = TWL_WIIU_TP_HEIGHT * frame_w / frame_h;
      origin_x = (TWL_WIIU_TP_WIDTH - content_w) / 2;
    } else {
      content_h = TWL_WIIU_TP_WIDTH * frame_h / frame_w;
      origin_y = (TWL_WIIU_TP_HEIGHT - content_h) / 2;
    }
    x = twl_wiiu_clamp(
      ((int32_t) touch.x - origin_x) * frame_w / content_w, 0, frame_w - 1);
    y = twl_wiiu_clamp(
      ((int32_t) touch.y - origin_y) * frame_h / content_h, 0, frame_h - 1);
    if (!wiiu->touching || x != wiiu->pointer_x || y != wiiu->pointer_y) {
      twl_wiiu_push_pointer(twl, TWL_EVENT_POINTER_MOVE, x, y);
    }
    if (!wiiu->touching) {
      twl_wiiu_push_pointer(twl, TWL_EVENT_POINTER_DOWN, x, y);
    }
    wiiu->pointer_x = x;
    wiiu->pointer_y = y;
    wiiu->touching = true;
  } else if (wiiu->touching) {
    twl_wiiu_push_pointer(
      twl, TWL_EVENT_POINTER_UP, wiiu->pointer_x, wiiu->pointer_y);
    wiiu->touching = false;
  }
}

static void twl_wiiu_read_gamepad(Twl *twl, TwlWiiU *wiiu) {
  VPADStatus status;
  VPADReadError error;
  size_t index;
  if (VPADRead(VPAD_CHAN_0, &status, 1, &error) <= 0 ||
      error != VPAD_READ_SUCCESS) {
    return;
  }

  twl_wiiu_read_touch(twl, wiiu, &status);

  for (index = 0;
       index < sizeof(twl_wiiu_button_keys) / sizeof(twl_wiiu_button_keys[0]);
       ++index) {
    const uint32_t mask = twl_wiiu_button_keys[index].mask;
    if (status.trigger & mask) {
      twl_wiiu_push_key(twl, TWL_EVENT_KEY_DOWN, twl_wiiu_button_keys[index].key);
    }
    if (status.release & mask) {
      twl_wiiu_push_key(twl, TWL_EVENT_KEY_UP, twl_wiiu_button_keys[index].key);
    }
  }

  if (status.trigger & VPAD_BUTTON_ZL) {
    twl_wiiu_push_text(twl, "Player");
  }
}

void twl_backend_pump_events(Twl *twl) {
  TwlWiiU *wiiu = twl ? (TwlWiiU *) twl->backend : NULL;
  if (!wiiu) {
    return;
  }
  if (!WHBProcIsRunning()) {
    if (!wiiu->quit_pushed) {
      TwlEvent event;
      twl_internal_zero(&event, sizeof(event));
      event.type = TWL_EVENT_QUIT;
      twl_wiiu_push(twl, &event);
      wiiu->quit_pushed = true;
    }
    return;
  }
  twl_wiiu_read_gamepad(twl, wiiu);
}
