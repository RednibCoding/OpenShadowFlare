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

#include <3ds.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum {
  LWL_N3DS_EVENT_QUEUE_CAPACITY = 32,
  LWL_N3DS_CIRCLE_THRESHOLD = 40,
  LWL_N3DS_CIRCLE_UP = BIT(28),
  LWL_N3DS_CIRCLE_DOWN = BIT(29),
  LWL_N3DS_CIRCLE_LEFT = BIT(30),
  LWL_N3DS_CIRCLE_RIGHT = BIT(31)
};

struct LwlWindow {
  int width;
  int height;
};

static LwlEvent g_events[LWL_N3DS_EVENT_QUEUE_CAPACITY];
static int g_event_head;
static int g_event_tail;
static u32 g_previous_keys;
static bool g_touch_down;
static bool g_input_pumped;

static void push_event(const LwlEvent *event) {
  int next = (g_event_tail + 1) % LWL_N3DS_EVENT_QUEUE_CAPACITY;
  if (next == g_event_head) {
    g_event_head = (g_event_head + 1) % LWL_N3DS_EVENT_QUEUE_CAPACITY;
  }
  g_events[g_event_tail] = *event;
  g_event_tail = next;
}

static bool pop_event(LwlEvent *event) {
  if (g_event_head == g_event_tail || !event) {
    return false;
  }
  *event = g_events[g_event_head];
  g_event_head = (g_event_head + 1) % LWL_N3DS_EVENT_QUEUE_CAPACITY;
  return true;
}

static void push_key_event(LwlEventType type, const char *key) {
  LwlEvent event;
  memset(&event, 0, sizeof(event));
  event.type = type;
  strncpy(event.key, key, sizeof(event.key) - 1);
  push_event(&event);
}

static void emit_key_transitions(u32 previous, u32 current) {
  static const struct {
    u32 key_mask;
    const char *key_name;
  } keys[] = {
    {KEY_DUP | LWL_N3DS_CIRCLE_UP, "up"},
    {KEY_DDOWN | LWL_N3DS_CIRCLE_DOWN, "down"},
    {KEY_DLEFT | LWL_N3DS_CIRCLE_LEFT, "left"},
    {KEY_DRIGHT | LWL_N3DS_CIRCLE_RIGHT, "right"},
    {KEY_A, "return"},
    {KEY_B, "escape"},
    {KEY_X, "i"},
    {KEY_Y, "n"},
    {KEY_L, "q"},
    {KEY_R, "r"},
    {KEY_ZR, "m"},
    {KEY_SELECT, "h"},
  };
  size_t index;

  for (index = 0; index < sizeof(keys) / sizeof(keys[0]); ++index) {
    bool was_down = (previous & keys[index].key_mask) != 0;
    bool is_down = (current & keys[index].key_mask) != 0;
    if (!was_down && is_down) {
      push_key_event(LWL_EVENT_KEY_DOWN, keys[index].key_name);
    } else if (was_down && !is_down) {
      push_key_event(LWL_EVENT_KEY_UP, keys[index].key_name);
    }
  }
}

static void emit_touch_event(LwlEventType type, const touchPosition *touch) {
  LwlEvent event;
  memset(&event, 0, sizeof(event));
  event.type = type;
  event.x = touch->px * 2;
  event.y = touch->py * 2;
  event.button = 1;
  event.clicks = type == LWL_EVENT_MOUSE_DOWN ? 1 : 0;
  push_event(&event);
}

static void open_keyboard(void) {
  SwkbdState keyboard;
  char text[sizeof(((LwlEvent *) 0)->text)] = {};

  swkbdInit(&keyboard, SWKBD_TYPE_QWERTY, 2, (int) sizeof(text) - 1);
  swkbdSetFeatures(&keyboard, SWKBD_DEFAULT_QWERTY);
  swkbdSetHintText(&keyboard, "Character name");
  if (swkbdInputText(&keyboard, text, sizeof(text)) == SWKBD_BUTTON_CONFIRM &&
      text[0] != '\0') {
    LwlEvent event;
    memset(&event, 0, sizeof(event));
    event.type = LWL_EVENT_TEXT_INPUT;
    memcpy(event.text, text, sizeof(event.text));
    push_event(&event);
  }
}

static void pump_input(void) {
  circlePosition circle;
  touchPosition touch;
  u32 keys;
  u32 held;
  bool touch_is_down;

  if (g_input_pumped) {
    return;
  }
  g_input_pumped = true;
  hidScanInput();
  held = hidKeysHeld();
  keys = held & (KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT | KEY_A | KEY_B |
                 KEY_X | KEY_Y | KEY_L | KEY_R | KEY_ZR | KEY_SELECT);
  hidCircleRead(&circle);
  if (circle.dy > (s16) LWL_N3DS_CIRCLE_THRESHOLD) {
    keys |= LWL_N3DS_CIRCLE_UP;
  }
  if (circle.dy < (s16) -LWL_N3DS_CIRCLE_THRESHOLD) {
    keys |= LWL_N3DS_CIRCLE_DOWN;
  }
  if (circle.dx < (s16) -LWL_N3DS_CIRCLE_THRESHOLD) {
    keys |= LWL_N3DS_CIRCLE_LEFT;
  }
  if (circle.dx > (s16) LWL_N3DS_CIRCLE_THRESHOLD) {
    keys |= LWL_N3DS_CIRCLE_RIGHT;
  }
  emit_key_transitions(g_previous_keys, keys);
  g_previous_keys = keys;

  if ((hidKeysDown() & KEY_START) != 0) {
    LwlEvent event;
    memset(&event, 0, sizeof(event));
    event.type = LWL_EVENT_QUIT;
    push_event(&event);
  }
  if ((hidKeysDown() & KEY_ZL) != 0) {
    open_keyboard();
  }

  touch_is_down = (held & KEY_TOUCH) != 0;
  if (touch_is_down || g_touch_down) {
    hidTouchRead(&touch);
  }
  if (touch_is_down && !g_touch_down) {
    emit_touch_event(LWL_EVENT_MOUSE_DOWN, &touch);
  } else if (touch_is_down) {
    emit_touch_event(LWL_EVENT_MOUSE_MOVE, &touch);
  } else if (g_touch_down) {
    emit_touch_event(LWL_EVENT_MOUSE_UP, &touch);
  }
  g_touch_down = touch_is_down;
}

bool lwl_init(void) {
  gfxInitDefault();
  gfxSet3D(false);
  g_event_head = 0;
  g_event_tail = 0;
  g_previous_keys = 0;
  g_touch_down = false;
  g_input_pumped = false;
  return true;
}

void lwl_shutdown(void) {
  gfxExit();
  g_event_head = 0;
  g_event_tail = 0;
}

LwlWindow *lwl_window_create(const char *title, int width, int height) {
  LwlWindow *window = (LwlWindow *) calloc(1, sizeof(*window));
  (void) title;
  if (window) {
    window->width = width;
    window->height = height;
  }
  return window;
}

LwlWindow *lwl_window_create_with_native_message_handler(
    const char *title, int width, int height,
    LwlNativeMessageHandler handler, void *user_data) {
  (void) handler;
  (void) user_data;
  return lwl_window_create(title, width, height);
}

LwlWindow *lwl_window_attach_native(void *native_window, int width, int height) {
  (void) native_window;
  (void) width;
  (void) height;
  return NULL;
}

void *lwl_window_get_native_handle(LwlWindow *window) {
  (void) window;
  return NULL;
}

void lwl_window_destroy(LwlWindow *window) { free(window); }
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
bool lwl_window_set_cursor_image(LwlWindow *window, const LwlColor *pixels,
                                 int width, int height, int hotspot_x,
                                 int hotspot_y) {
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
  if (!window) {
    return false;
  }
  window->width = width;
  window->height = height;
  return true;
}
void lwl_window_get_size(LwlWindow *window, int *width, int *height) {
  if (width) {
    *width = window ? window->width : 0;
  }
  if (height) {
    *height = window ? window->height : 0;
  }
}
LwlColor *lwl_window_get_framebuffer(LwlWindow *window, int *width, int *height) {
  (void) window;
  (void) width;
  (void) height;
  return NULL;
}
bool lwl_window_resize_framebuffer(LwlWindow *window, int width, int height) {
  (void) window;
  (void) width;
  (void) height;
  return false;
}
void lwl_window_update_rects(LwlWindow *window, const LwlRect *rects, int count) {
  (void) window;
  (void) rects;
  (void) count;
}

bool lwl_poll_event(LwlWindow *window, LwlEvent *event) {
  (void) window;
  if (g_event_head == g_event_tail) {
    pump_input();
  }
  return pop_event(event);
}

bool lwl_wait_event(LwlWindow *window, double timeout_seconds) {
  (void) window;
  if (g_event_head == g_event_tail) {
    pump_input();
  }
  if (g_event_head != g_event_tail) {
    return true;
  }
  lwl_sleep_seconds(timeout_seconds);
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

double lwl_time_seconds(void) { return (double) osGetTime() / 1000.0; }
void lwl_sleep_seconds(double seconds) {
  if (seconds > 0.0) {
    svcSleepThread((s64) (seconds * 1000000000.0));
  }
  g_input_pumped = false;
}
void lwl_sleep_until_seconds(double time_seconds) {
  double remaining = time_seconds - lwl_time_seconds();
  lwl_sleep_seconds(remaining);
}
const char *lwl_platform_name(void) { return "Nintendo 3DS"; }
double lwl_display_scale(void) { return 1.0; }
bool lwl_exe_path(char *buf, int size) {
  if (!buf || size <= 0 || !getcwd(buf, (size_t) size)) {
    return false;
  }
  return true;
}

LwlGlConfig lwl_gl_config_default(void) {
  LwlGlConfig config;
  memset(&config, 0, sizeof(config));
  return config;
}
LwlGlContext *lwl_gl_context_create(LwlWindow *window, const LwlGlConfig *config) {
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
