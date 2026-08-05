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
#include <string.h>

static const struct {
  uint64_t mask;
  TwlKey key;
} twl_switch_button_keys[] = {
  {HidNpadButton_Up, TWL_KEY_UP},
  {HidNpadButton_Down, TWL_KEY_DOWN},
  {HidNpadButton_Left, TWL_KEY_LEFT},
  {HidNpadButton_Right, TWL_KEY_RIGHT},
  {HidNpadButton_A, TWL_KEY_RETURN},
  {HidNpadButton_Plus, TWL_KEY_RETURN},
  {HidNpadButton_B, TWL_KEY_ESCAPE},
  {HidNpadButton_Minus, TWL_KEY_ESCAPE},
  {HidNpadButton_X, TWL_KEY_I},
  {HidNpadButton_R, TWL_KEY_R},
};

static int32_t twl_switch_clamp(int32_t value, int32_t low, int32_t high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

static void twl_switch_push(Twl *twl, TwlEvent *event) {
  event->timestamp_us = twl_backend_time_microseconds(twl);
  twl_internal_push_event(twl, event);
}

static void twl_switch_push_key(Twl *twl, TwlEventType type, TwlKey key) {
  TwlEvent event;
  twl_internal_zero(&event, sizeof(event));
  event.type = type;
  event.key = key;
  twl_switch_push(twl, &event);
}

static void twl_switch_push_pointer(
    Twl *twl, TwlEventType type, int32_t x, int32_t y) {
  TwlEvent event;
  twl_internal_zero(&event, sizeof(event));
  event.type = type;
  event.x = x;
  event.y = y;
  event.button = 1u;
  twl_switch_push(twl, &event);
}

static void twl_switch_push_text(Twl *twl, const char *text) {
  const char *cursor;
  for (cursor = text; *cursor; ++cursor) {
    TwlEvent event;
    twl_internal_zero(&event, sizeof(event));
    event.type = TWL_EVENT_TEXT;
    event.codepoint = (uint32_t) (unsigned char) *cursor;
    twl_switch_push(twl, &event);
  }
}

static void twl_switch_read_touch(Twl *twl, TwlSwitch *sw) {
  HidTouchScreenState touch;
  memset(&touch, 0, sizeof(touch));
  if (hidGetTouchScreenStates(&touch, 1) && touch.count > 0) {
    const int32_t x = twl_switch_clamp(
      ((int32_t) touch.touches[0].x - sw->view_x) *
        (int32_t) sw->frame_width / sw->view_w,
      0, (int32_t) sw->frame_width - 1);
    const int32_t y = twl_switch_clamp(
      ((int32_t) touch.touches[0].y - sw->view_y) *
        (int32_t) sw->frame_height / sw->view_h,
      0, (int32_t) sw->frame_height - 1);
    if (!sw->touching || x != sw->pointer_x || y != sw->pointer_y) {
      twl_switch_push_pointer(twl, TWL_EVENT_POINTER_MOVE, x, y);
    }
    if (!sw->touching) {
      twl_switch_push_pointer(twl, TWL_EVENT_POINTER_DOWN, x, y);
    }
    sw->pointer_x = x;
    sw->pointer_y = y;
    sw->touching = true;
  } else if (sw->touching) {
    twl_switch_push_pointer(
      twl, TWL_EVENT_POINTER_UP, sw->pointer_x, sw->pointer_y);
    sw->touching = false;
  }
}

void twl_backend_pump_events(Twl *twl) {
  TwlSwitch *sw = twl ? (TwlSwitch *) twl->backend : NULL;
  uint64_t pressed;
  uint64_t released;
  size_t index;
  if (!sw) {
    return;
  }
  if (!appletMainLoop()) {
    if (!sw->quit_pushed) {
      TwlEvent event;
      twl_internal_zero(&event, sizeof(event));
      event.type = TWL_EVENT_QUIT;
      twl_switch_push(twl, &event);
      sw->quit_pushed = true;
    }
    return;
  }
  if (!sw->pad_ready) {
    return;
  }

  padUpdate(&sw->pad);
  pressed = padGetButtonsDown(&sw->pad);
  released = padGetButtonsUp(&sw->pad);

  twl_switch_read_touch(twl, sw);

  for (index = 0;
       index < sizeof(twl_switch_button_keys) /
         sizeof(twl_switch_button_keys[0]);
       ++index) {
    const uint64_t mask = twl_switch_button_keys[index].mask;
    if (pressed & mask) {
      twl_switch_push_key(
        twl, TWL_EVENT_KEY_DOWN, twl_switch_button_keys[index].key);
    }
    if (released & mask) {
      twl_switch_push_key(
        twl, TWL_EVENT_KEY_UP, twl_switch_button_keys[index].key);
    }
  }

  if (pressed & HidNpadButton_ZL) {
    twl_switch_push_text(twl, "Player");
  }
}
