/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of LWL.
 *
 * LWL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "lwl.h"

#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/touch.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define LWL_VITA_WIDTH 960
#define LWL_VITA_HEIGHT 544
#define LWL_VITA_TOUCH_WIDTH 1920
#define LWL_VITA_TOUCH_HEIGHT 1088

struct LwlWindow {
  int width;
  int height;
};

static struct LwlWindow g_window;

enum { LWL_VITA_EVENT_CAPACITY = 32 };

static LwlEvent g_events[LWL_VITA_EVENT_CAPACITY];
static int g_event_count;
static int g_event_read;
static bool g_polled_this_frame;
static int g_cursor_x = LWL_VITA_WIDTH / 2;
static int g_cursor_y = LWL_VITA_HEIGHT / 2;
static unsigned int g_prev_buttons;
static bool g_was_touching;

static int lwl_vita_clamp(int value, int low, int high) {
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

static int lwl_vita_axis_delta(unsigned char axis) {
  enum { DEAD_ZONE = 24, AXIS_RANGE = 127, MAX_SPEED = 12 };
  const int offset = (int) axis - 128;
  const int magnitude = offset < 0 ? -offset : offset;
  float normalized;
  int speed;

  if (magnitude <= DEAD_ZONE) {
    return 0;
  }
  normalized = (float) (magnitude - DEAD_ZONE) /
      (float) (AXIS_RANGE - DEAD_ZONE);
  if (normalized > 1.0f) {
    normalized = 1.0f;
  }
  speed = (int) (normalized * normalized * (float) MAX_SPEED + 0.5f);
  return offset < 0 ? -speed : speed;
}

static LwlEvent *lwl_vita_push(LwlEventType type) {
  LwlEvent *event;
  if (g_event_count >= LWL_VITA_EVENT_CAPACITY) {
    return NULL;
  }
  event = &g_events[g_event_count++];
  memset(event, 0, sizeof(*event));
  event->type = type;
  return event;
}

static void lwl_vita_push_pointer(LwlEventType type, int button) {
  LwlEvent *event = lwl_vita_push(type);
  if (event) {
    event->x = g_cursor_x;
    event->y = g_cursor_y;
    event->button = button;
    event->clicks = 1;
  }
}

static void lwl_vita_push_key(LwlEventType type, const char *key) {
  LwlEvent *event = lwl_vita_push(type);
  if (event) {
    strncpy(event->key, key, sizeof(event->key) - 1);
  }
}

struct LwlVitaKeyMapping {
  unsigned int mask;
  const char *key;
};

static const struct LwlVitaKeyMapping g_key_mappings[] = {
  {SCE_CTRL_UP, "up"},
  {SCE_CTRL_DOWN, "down"},
  {SCE_CTRL_LEFT, "left"},
  {SCE_CTRL_RIGHT, "right"},
  {SCE_CTRL_CROSS, "return"},
  {SCE_CTRL_START, "return"},
  {SCE_CTRL_CIRCLE, "escape"},
  {SCE_CTRL_SELECT, "escape"},
  {SCE_CTRL_TRIANGLE, "i"},
  {SCE_CTRL_SQUARE, "n"},
  {SCE_CTRL_LTRIGGER, "q"},
  {SCE_CTRL_RTRIGGER, "r"},
};

static void lwl_vita_sample_input(void) {
  SceCtrlData pad;
  SceTouchData touch;
  unsigned int buttons = 0;
  unsigned int pressed;
  unsigned int released;
  size_t index;

  g_event_count = 0;
  g_event_read = 0;

  memset(&pad, 0, sizeof(pad));
  if (sceCtrlPeekBufferPositive(0, &pad, 1) > 0) {
    const int delta_x = lwl_vita_axis_delta(pad.lx);
    const int delta_y = lwl_vita_axis_delta(pad.ly);
    buttons = pad.buttons;
    if (delta_x != 0 || delta_y != 0) {
      const int old_x = g_cursor_x;
      const int old_y = g_cursor_y;
      LwlEvent *move;
      g_cursor_x = lwl_vita_clamp(
          g_cursor_x + delta_x, 0, LWL_VITA_WIDTH - 1);
      g_cursor_y = lwl_vita_clamp(
          g_cursor_y + delta_y, 0, LWL_VITA_HEIGHT - 1);
      move = lwl_vita_push(LWL_EVENT_MOUSE_MOVE);
      if (move) {
        move->x = g_cursor_x;
        move->y = g_cursor_y;
        move->dx = g_cursor_x - old_x;
        move->dy = g_cursor_y - old_y;
      }
    }
  }

  memset(&touch, 0, sizeof(touch));
  if (sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1) > 0 &&
      touch.reportNum > 0) {
    const int x = lwl_vita_clamp(
        (int) touch.report[0].x * LWL_VITA_WIDTH / LWL_VITA_TOUCH_WIDTH,
        0, LWL_VITA_WIDTH - 1);
    const int y = lwl_vita_clamp(
        (int) touch.report[0].y * LWL_VITA_HEIGHT / LWL_VITA_TOUCH_HEIGHT,
        0, LWL_VITA_HEIGHT - 1);
    if (x != g_cursor_x || y != g_cursor_y || !g_was_touching) {
      LwlEvent *move = lwl_vita_push(LWL_EVENT_MOUSE_MOVE);
      if (move) {
        move->dx = x - g_cursor_x;
        move->dy = y - g_cursor_y;
        move->x = x;
        move->y = y;
      }
      g_cursor_x = x;
      g_cursor_y = y;
    }
    if (!g_was_touching) {
      lwl_vita_push_pointer(LWL_EVENT_MOUSE_DOWN, 1);
      g_was_touching = true;
    }
  } else if (g_was_touching) {
    lwl_vita_push_pointer(LWL_EVENT_MOUSE_UP, 1);
    g_was_touching = false;
  }

  pressed = buttons & ~g_prev_buttons;
  released = ~buttons & g_prev_buttons;
  if (pressed & SCE_CTRL_CROSS) {
    lwl_vita_push_pointer(LWL_EVENT_MOUSE_DOWN, 1);
  }
  if (released & SCE_CTRL_CROSS) {
    lwl_vita_push_pointer(LWL_EVENT_MOUSE_UP, 1);
  }
  if (pressed & SCE_CTRL_CIRCLE) {
    lwl_vita_push_pointer(LWL_EVENT_MOUSE_DOWN, 3);
  }
  if (released & SCE_CTRL_CIRCLE) {
    lwl_vita_push_pointer(LWL_EVENT_MOUSE_UP, 3);
  }
  for (index = 0;
       index < sizeof(g_key_mappings) / sizeof(g_key_mappings[0]);
       ++index) {
    if (pressed & g_key_mappings[index].mask) {
      lwl_vita_push_key(LWL_EVENT_KEY_DOWN, g_key_mappings[index].key);
    }
    if (released & g_key_mappings[index].mask) {
      lwl_vita_push_key(LWL_EVENT_KEY_UP, g_key_mappings[index].key);
    }
  }
  g_prev_buttons = buttons;
}

bool lwl_init(void) {
  sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
  sceTouchSetSamplingState(
      SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
  g_cursor_x = LWL_VITA_WIDTH / 2;
  g_cursor_y = LWL_VITA_HEIGHT / 2;
  g_prev_buttons = 0;
  g_was_touching = false;
  g_event_count = 0;
  g_event_read = 0;
  g_polled_this_frame = false;
  return true;
}

void lwl_shutdown(void) {
}

LwlWindow *lwl_window_create(const char *title, int width, int height) {
  (void) title;
  (void) width;
  (void) height;
  g_window.width = LWL_VITA_WIDTH;
  g_window.height = LWL_VITA_HEIGHT;
  return &g_window;
}

LwlWindow *lwl_window_create_with_native_message_handler(
    const char *title, int width, int height,
    LwlNativeMessageHandler handler, void *user_data) {
  (void) handler;
  (void) user_data;
  return lwl_window_create(title, width, height);
}

LwlWindow *lwl_window_attach_native(
    void *native_window, int width, int height) {
  (void) native_window;
  return lwl_window_create(NULL, width, height);
}

void *lwl_window_get_native_handle(LwlWindow *window) {
  (void) window;
  return NULL;
}

void lwl_window_destroy(LwlWindow *window) {
  (void) window;
}

void lwl_window_show(LwlWindow *window) {
  (void) window;
}

void lwl_window_set_title(LwlWindow *window, const char *title) {
  (void) window;
  (void) title;
}

void lwl_window_set_mode(LwlWindow *window, LwlWindowMode mode) {
  (void) window;
  (void) mode;
}

bool lwl_window_has_focus(LwlWindow *window) {
  (void) window;
  return true;
}

void lwl_window_set_cursor(LwlWindow *window, LwlCursor cursor) {
  (void) window;
  (void) cursor;
}

bool lwl_window_set_cursor_image(
    LwlWindow *window, const LwlColor *pixels,
    int width, int height, int hotspot_x, int hotspot_y) {
  (void) window;
  (void) pixels;
  (void) width;
  (void) height;
  (void) hotspot_x;
  (void) hotspot_y;
  return false;
}

void lwl_window_set_cursor_visible(LwlWindow *window, bool visible) {
  (void) window;
  (void) visible;
}

bool lwl_window_set_size(LwlWindow *window, int width, int height) {
  (void) window;
  (void) width;
  (void) height;
  return false;
}

void lwl_window_get_size(LwlWindow *window, int *width, int *height) {
  if (width) {
    *width = window ? window->width : LWL_VITA_WIDTH;
  }
  if (height) {
    *height = window ? window->height : LWL_VITA_HEIGHT;
  }
}

LwlColor *lwl_window_get_framebuffer(
    LwlWindow *window, int *width, int *height) {
  (void) window;
  if (width) {
    *width = 0;
  }
  if (height) {
    *height = 0;
  }
  return NULL;
}

bool lwl_window_resize_framebuffer(
    LwlWindow *window, int width, int height) {
  (void) window;
  (void) width;
  (void) height;
  return false;
}

void lwl_window_update_rects(
    LwlWindow *window, const LwlRect *rects, int count) {
  (void) window;
  (void) rects;
  (void) count;
}

bool lwl_poll_event(LwlWindow *window, LwlEvent *event) {
  (void) window;
  if (!event) {
    return false;
  }
  if (g_event_read < g_event_count) {
    *event = g_events[g_event_read++];
    return true;
  }
  if (g_polled_this_frame) {
    g_polled_this_frame = false;
    return false;
  }
  g_polled_this_frame = true;
  lwl_vita_sample_input();
  if (g_event_read < g_event_count) {
    *event = g_events[g_event_read++];
    return true;
  }
  g_polled_this_frame = false;
  return false;
}

bool lwl_wait_event(LwlWindow *window, double timeout_seconds) {
  (void) window;
  (void) timeout_seconds;
  return false;
}

char *lwl_clipboard_get(LwlWindow *window) {
  (void) window;
  return NULL;
}

void lwl_clipboard_set(LwlWindow *window, const char *text) {
  (void) window;
  (void) text;
}

char *lwl_select_folder(LwlWindow *window, const char *title) {
  (void) window;
  (void) title;
  return NULL;
}

void lwl_free(void *ptr) {
  free(ptr);
}

double lwl_time_seconds(void) {
  return (double) sceKernelGetProcessTimeWide() / 1000000.0;
}

void lwl_sleep_seconds(double seconds) {
  if (seconds > 0.0) {
    sceKernelDelayThread((unsigned int) (seconds * 1000000.0));
  }
}

void lwl_sleep_until_seconds(double time_seconds) {
  const double now = lwl_time_seconds();
  if (time_seconds > now) {
    lwl_sleep_seconds(time_seconds - now);
  }
}

const char *lwl_platform_name(void) {
  return "vita";
}

double lwl_display_scale(void) {
  return 1.0;
}

bool lwl_exe_path(char *buf, int size) {
  (void) buf;
  (void) size;
  return false;
}

LwlGlConfig lwl_gl_config_default(void) {
  LwlGlConfig config;
  memset(&config, 0, sizeof(config));
  config.api = LWL_GL_API_ES;
  config.major_version = 2;
  config.double_buffer = true;
  return config;
}

LwlGlContext *lwl_gl_context_create(
    LwlWindow *window, const LwlGlConfig *config) {
  (void) window;
  (void) config;
  return NULL;
}

void lwl_gl_context_destroy(LwlGlContext *context) {
  (void) context;
}

bool lwl_gl_context_make_current(LwlGlContext *context) {
  (void) context;
  return false;
}

void lwl_gl_context_swap_buffers(LwlGlContext *context) {
  (void) context;
}

bool lwl_gl_context_set_swap_interval(LwlGlContext *context, int interval) {
  (void) context;
  (void) interval;
  return false;
}

void *lwl_gl_get_proc_address(const char *name) {
  (void) name;
  return NULL;
}
