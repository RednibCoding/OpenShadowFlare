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

/* Wii U GamePad backend: touchscreen maps directly to the game pointer. */

#include "lwl.h"

#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <vpad/input.h>
#include <whb/proc.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* SDL stretches the 640x480 game texture to the 854x480 GamePad output. */
#define LWL_WIIU_WIDTH 640
#define LWL_WIIU_HEIGHT 480
#define LWL_WIIU_GAMEPAD_WIDTH 854

struct LwlWindow {
  int width;
  int height;
};

static struct LwlWindow g_window;

enum { LWL_WIIU_EVENT_CAPACITY = 32 };

static LwlEvent g_events[LWL_WIIU_EVENT_CAPACITY];
static int g_event_count;
static int g_event_read;
static bool g_polled_this_frame;
static bool g_vpad_initialized;
static bool g_was_touching;
static int g_pointer_x = LWL_WIIU_WIDTH / 2;
static int g_pointer_y = LWL_WIIU_HEIGHT / 2;

static int lwl_wiiu_clamp(int value, int low, int high) {
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

static LwlEvent *lwl_wiiu_push(LwlEventType type) {
  LwlEvent *event;
  if (g_event_count >= LWL_WIIU_EVENT_CAPACITY) {
    return NULL;
  }
  event = &g_events[g_event_count++];
  memset(event, 0, sizeof(*event));
  event->type = type;
  return event;
}

static void lwl_wiiu_push_pointer(LwlEventType type, int button) {
  LwlEvent *event = lwl_wiiu_push(type);
  if (event) {
    event->x = g_pointer_x;
    event->y = g_pointer_y;
    event->button = button;
    event->clicks = 1;
  }
}

static void lwl_wiiu_push_key(LwlEventType type, const char *key) {
  LwlEvent *event = lwl_wiiu_push(type);
  if (event) {
    strncpy(event->key, key, sizeof(event->key) - 1);
  }
}

static void lwl_wiiu_push_text(const char *text) {
  LwlEvent *event = lwl_wiiu_push(LWL_EVENT_TEXT_INPUT);
  if (event) {
    strncpy(event->text, text, sizeof(event->text) - 1);
  }
}

struct LwlWiiUKeyMapping {
  uint32_t mask;
  const char *key;
};

static const struct LwlWiiUKeyMapping g_key_mappings[] = {
  {VPAD_BUTTON_UP, "up"},
  {VPAD_BUTTON_DOWN, "down"},
  {VPAD_BUTTON_LEFT, "left"},
  {VPAD_BUTTON_RIGHT, "right"},
  {VPAD_BUTTON_A, "return"},
  {VPAD_BUTTON_PLUS, "return"},
  {VPAD_BUTTON_B, "escape"},
  {VPAD_BUTTON_MINUS, "escape"},
  {VPAD_BUTTON_X, "i"},
  {VPAD_BUTTON_Y, "n"},
  {VPAD_BUTTON_L, "q"},
  {VPAD_BUTTON_R, "r"},
};

static void lwl_wiiu_sample_input(void) {
  VPADStatus status;
  VPADReadError error;
  VPADTouchData touch;
  int32_t count;
  size_t index;

  g_event_count = 0;
  g_event_read = 0;
  memset(&status, 0, sizeof(status));
  count = VPADRead(VPAD_CHAN_0, &status, 1, &error);
  if (count <= 0 && error == VPAD_READ_UNINITIALIZED) {
    VPADInit();
    g_vpad_initialized = true;
    count = VPADRead(VPAD_CHAN_0, &status, 1, &error);
  }
  if (count <= 0 || error != VPAD_READ_SUCCESS) {
    return;
  }

  if ((status.trigger & VPAD_BUTTON_ZL) != 0u) {
    lwl_wiiu_push_text("Player");
  }

  memset(&touch, 0, sizeof(touch));
  VPADGetTPCalibratedPointEx(
      VPAD_CHAN_0, VPAD_TP_854X480, &touch, &status.tpNormal);
  if (touch.touched != 0u) {
    const int x =
        lwl_wiiu_clamp((int) touch.x, 0, LWL_WIIU_GAMEPAD_WIDTH - 1) *
        LWL_WIIU_WIDTH / LWL_WIIU_GAMEPAD_WIDTH;
    const int y = lwl_wiiu_clamp((int) touch.y, 0, LWL_WIIU_HEIGHT - 1);
    if (x != g_pointer_x || y != g_pointer_y || !g_was_touching) {
      LwlEvent *move = lwl_wiiu_push(LWL_EVENT_MOUSE_MOVE);
      if (move) {
        move->dx = x - g_pointer_x;
        move->dy = y - g_pointer_y;
        move->x = x;
        move->y = y;
      }
      g_pointer_x = x;
      g_pointer_y = y;
    }
    if (!g_was_touching) {
      lwl_wiiu_push_pointer(LWL_EVENT_MOUSE_DOWN, 1);
      g_was_touching = true;
    }
  } else if (g_was_touching) {
    lwl_wiiu_push_pointer(LWL_EVENT_MOUSE_UP, 1);
    g_was_touching = false;
  }

  for (index = 0;
       index < sizeof(g_key_mappings) / sizeof(g_key_mappings[0]);
       ++index) {
    if ((status.trigger & g_key_mappings[index].mask) != 0u) {
      lwl_wiiu_push_key(LWL_EVENT_KEY_DOWN, g_key_mappings[index].key);
    }
    if ((status.release & g_key_mappings[index].mask) != 0u) {
      lwl_wiiu_push_key(LWL_EVENT_KEY_UP, g_key_mappings[index].key);
    }
  }
}

bool lwl_init(void) {
  WHBProcInit();
  VPADInit();
  g_event_count = 0;
  g_event_read = 0;
  g_polled_this_frame = false;
  g_vpad_initialized = true;
  g_was_touching = false;
  g_pointer_x = LWL_WIIU_WIDTH / 2;
  g_pointer_y = LWL_WIIU_HEIGHT / 2;
  return true;
}

void lwl_shutdown(void) {
  if (g_vpad_initialized) {
    VPADShutdown();
    g_vpad_initialized = false;
  }
  WHBProcShutdown();
}

LwlWindow *lwl_window_create(const char *title, int width, int height) {
  (void) title;
  (void) width;
  (void) height;
  g_window.width = LWL_WIIU_WIDTH;
  g_window.height = LWL_WIIU_HEIGHT;
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

void lwl_window_destroy(LwlWindow *window) { (void) window; }
void lwl_window_show(LwlWindow *window) { (void) window; }
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
    *width = window ? window->width : LWL_WIIU_WIDTH;
  }
  if (height) {
    *height = window ? window->height : LWL_WIIU_HEIGHT;
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
  lwl_wiiu_sample_input();
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
void lwl_free(void *ptr) { free(ptr); }

double lwl_time_seconds(void) {
  return (double) OSGetTime() / (double) OSTimerClockSpeed;
}
void lwl_sleep_seconds(double seconds) {
  if (seconds > 0.0) {
    OSSleepTicks((OSTime) (seconds * (double) OSTimerClockSpeed));
  }
}
void lwl_sleep_until_seconds(double time_seconds) {
  const double now = lwl_time_seconds();
  if (time_seconds > now) {
    lwl_sleep_seconds(time_seconds - now);
  }
}
const char *lwl_platform_name(void) { return "wiiu"; }
double lwl_display_scale(void) { return 1.0; }
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
void lwl_gl_context_destroy(LwlGlContext *context) { (void) context; }
bool lwl_gl_context_make_current(LwlGlContext *context) {
  (void) context;
  return false;
}
void lwl_gl_context_swap_buffers(LwlGlContext *context) { (void) context; }
bool lwl_gl_context_set_swap_interval(LwlGlContext *context, int interval) {
  (void) context;
  (void) interval;
  return false;
}
void *lwl_gl_get_proc_address(const char *name) {
  (void) name;
  return NULL;
}
