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

#include <android/input.h>
#include <android/keycodes.h>

static int32_t twl_android_clamp(int32_t value, int32_t low, int32_t high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

static void twl_android_push(Twl *twl, TwlEvent *event) {
  event->timestamp_us = twl_backend_time_microseconds(twl);
  twl_internal_push_event(twl, event);
}

static void twl_android_push_pointer(
    Twl *twl, TwlEventType type, int32_t x, int32_t y) {
  TwlEvent event;
  twl_internal_zero(&event, sizeof(event));
  event.type = type;
  event.x = x;
  event.y = y;
  event.button = 1u;
  twl_android_push(twl, &event);
}

static void twl_android_push_key(Twl *twl, TwlEventType type, TwlKey key) {
  TwlEvent event;
  twl_internal_zero(&event, sizeof(event));
  event.type = type;
  event.key = key;
  twl_android_push(twl, &event);
}

static void twl_android_motion_to_frame(
    const TwlAndroid *android, float raw_x, float raw_y,
    int32_t *out_x, int32_t *out_y) {
  const int32_t view_w = android->view_w > 0 ? android->view_w : 1;
  const int32_t view_h = android->view_h > 0 ? android->view_h : 1;
  *out_x = twl_android_clamp(
    ((int32_t) raw_x - android->view_x) * (int32_t) android->frame_width /
      view_w,
    0, (int32_t) android->frame_width - 1);
  *out_y = twl_android_clamp(
    ((int32_t) raw_y - android->view_y) * (int32_t) android->frame_height /
      view_h,
    0, (int32_t) android->frame_height - 1);
}

static int32_t twl_android_on_motion(
    Twl *twl, TwlAndroid *android, AInputEvent *event) {
  const int32_t action =
    AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
  int32_t x;
  int32_t y;
  twl_android_motion_to_frame(
    android, AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0), &x, &y);

  switch (action) {
    case AMOTION_EVENT_ACTION_DOWN:
      twl_android_push_pointer(twl, TWL_EVENT_POINTER_MOVE, x, y);
      twl_android_push_pointer(twl, TWL_EVENT_POINTER_DOWN, x, y);
      android->touching = true;
      break;
    case AMOTION_EVENT_ACTION_MOVE:
      twl_android_push_pointer(twl, TWL_EVENT_POINTER_MOVE, x, y);
      break;
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_CANCEL:
      twl_android_push_pointer(twl, TWL_EVENT_POINTER_UP, x, y);
      android->touching = false;
      break;
    default:
      break;
  }
  android->pointer_x = x;
  android->pointer_y = y;
  return 1;
}

static void twl_android_push_text(Twl *twl, uint32_t codepoint) {
  TwlEvent event;
  twl_internal_zero(&event, sizeof(event));
  event.type = TWL_EVENT_TEXT;
  event.codepoint = codepoint;
  twl_android_push(twl, &event);
}

static uint32_t twl_android_char_for(int32_t code, int32_t meta) {
  const int upper =
    ((meta & AMETA_SHIFT_ON) != 0) ^ ((meta & AMETA_CAPS_LOCK_ON) != 0);
  if (code >= AKEYCODE_A && code <= AKEYCODE_Z) {
    return (uint32_t) ((upper ? 'A' : 'a') + (code - AKEYCODE_A));
  }
  if (code >= AKEYCODE_0 && code <= AKEYCODE_9) {
    return (uint32_t) ('0' + (code - AKEYCODE_0));
  }
  if (code == AKEYCODE_SPACE) {
    return (uint32_t) ' ';
  }
  return 0u;
}

static TwlKey twl_android_key_for(int32_t code) {
  if (code >= AKEYCODE_A && code <= AKEYCODE_Z) {
    return (TwlKey) (TWL_KEY_A + (code - AKEYCODE_A));
  }
  if (code >= AKEYCODE_0 && code <= AKEYCODE_9) {
    return (TwlKey) (TWL_KEY_0 + (code - AKEYCODE_0));
  }
  switch (code) {
    case AKEYCODE_BACK: return TWL_KEY_ESCAPE;
    case AKEYCODE_ENTER:
    case AKEYCODE_DPAD_CENTER: return TWL_KEY_RETURN;
    case AKEYCODE_DPAD_UP: return TWL_KEY_UP;
    case AKEYCODE_DPAD_DOWN: return TWL_KEY_DOWN;
    case AKEYCODE_DPAD_LEFT: return TWL_KEY_LEFT;
    case AKEYCODE_DPAD_RIGHT: return TWL_KEY_RIGHT;
    case AKEYCODE_DEL: return TWL_KEY_BACKSPACE;
    case AKEYCODE_FORWARD_DEL: return TWL_KEY_DELETE;
    case AKEYCODE_SPACE: return TWL_KEY_SPACE;
    case AKEYCODE_TAB: return TWL_KEY_TAB;
    default: return TWL_KEY_UNKNOWN;
  }
}

static int32_t twl_android_on_key(Twl *twl, AInputEvent *event) {
  const int32_t action = AKeyEvent_getAction(event);
  const int32_t code = AKeyEvent_getKeyCode(event);
  const TwlKey key = twl_android_key_for(code);
  if (key == TWL_KEY_UNKNOWN) {
    return 0;
  }
  if (action == AKEY_EVENT_ACTION_DOWN) {
    const uint32_t codepoint =
      twl_android_char_for(code, AKeyEvent_getMetaState(event));
    twl_android_push_key(twl, TWL_EVENT_KEY_DOWN, key);
    if (codepoint != 0u) {
      twl_android_push_text(twl, codepoint);
    }
  } else if (action == AKEY_EVENT_ACTION_UP) {
    twl_android_push_key(twl, TWL_EVENT_KEY_UP, key);
  }
  return 1;
}

int32_t twl_android_on_input(struct android_app *app, AInputEvent *event) {
  Twl *twl = (Twl *) app->userData;
  TwlAndroid *android = twl ? (TwlAndroid *) twl->backend : NULL;
  if (!android) {
    return 0;
  }
  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
    return twl_android_on_motion(twl, android, event);
  }
  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
    return twl_android_on_key(twl, event);
  }
  return 0;
}

void twl_android_pump_once(TwlAndroid *android, int timeout_ms) {
  struct android_poll_source *source = NULL;
  int events = 0;
  if (ALooper_pollOnce(timeout_ms, NULL, &events, (void **) &source) >= 0 &&
      source) {
    source->process(android->app, source);
  }
}

void twl_backend_pump_events(Twl *twl) {
  TwlAndroid *android = twl ? (TwlAndroid *) twl->backend : NULL;
  struct android_poll_source *source = NULL;
  int events = 0;
  if (!android) {
    return;
  }
  while (ALooper_pollOnce(0, NULL, &events, (void **) &source) >= 0) {
    if (source) {
      source->process(android->app, source);
    }
    if (android->app->destroyRequested) {
      break;
    }
  }
  if (android->app->destroyRequested && !android->quit_pushed) {
    TwlEvent event;
    twl_internal_zero(&event, sizeof(event));
    event.type = TWL_EVENT_QUIT;
    twl_android_push(twl, &event);
    android->quit_pushed = true;
  }
}
