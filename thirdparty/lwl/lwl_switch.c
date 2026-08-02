/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of LWL.
 *
 * LWL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * LWL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along with
 * LWL. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Nintendo Switch backend. The console has no windowing: the "window" is a fixed
 * 1280x720 surface (the handheld touchscreen resolution; the system upscales it
 * when docked). The nxfb presenter owns the libnx framebuffer; this file provides
 * input and timing.
 *
 * Input maps the touchscreen to the pointer (touch = move + primary click) and
 * the gamepad to keys, so the game's mouse/keyboard-driven UI works unchanged.
 * ZL opens the system keyboard and submits its text to the focused text field.
 * Pointer coordinates are reported in the 1280x720 surface space; the game's
 * InputAdapter maps them onto the 640x480 virtual surface with the same
 * fitViewport the presenter uses.
 *
 * Compiled only for Switch (selected in CMakeLists.txt), so it is not wrapped in
 * a platform #if.
 */

#include "lwl.h"

#include <switch.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LWL_SWITCH_WIDTH 1280
#define LWL_SWITCH_HEIGHT 720

struct LwlWindow {
  int width;
  int height;
};

static struct LwlWindow g_window;

/* --- Input: touchscreen -> pointer, gamepad -> keys --- */

enum { LWL_SWITCH_EVENT_CAPACITY = 32 };

static LwlEvent g_events[LWL_SWITCH_EVENT_CAPACITY];
static int g_event_count;
static int g_event_read;
static bool g_polled_this_frame;

static PadState g_pad;
static bool g_input_ready;
static bool g_was_touching;
static int g_pointer_x = LWL_SWITCH_WIDTH / 2;
static int g_pointer_y = LWL_SWITCH_HEIGHT / 2;

static int lwl_switch_clamp(int value, int low, int high) {
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

static LwlEvent *lwl_switch_push(LwlEventType type) {
  LwlEvent *event;
  if (g_event_count >= LWL_SWITCH_EVENT_CAPACITY) {
    return NULL;
  }
  event = &g_events[g_event_count++];
  memset(event, 0, sizeof(*event));
  event->type = type;
  return event;
}

static void lwl_switch_push_pointer(LwlEventType type, int button) {
  LwlEvent *event = lwl_switch_push(type);
  if (event) {
    event->x = g_pointer_x;
    event->y = g_pointer_y;
    event->button = button;
    event->clicks = 1;
  }
}

static void lwl_switch_push_key(LwlEventType type, const char *key) {
  LwlEvent *event = lwl_switch_push(type);
  if (event) {
    strncpy(event->key, key, sizeof(event->key) - 1);
  }
}

static void lwl_switch_show_keyboard(void) {
  SwkbdConfig keyboard;
  char text[64] = {0};

  if (R_FAILED(swkbdCreate(&keyboard, 0))) {
    return;
  }
  swkbdConfigMakePresetUserName(&keyboard);
  swkbdConfigSetHeaderText(&keyboard, "Character name");
  swkbdConfigSetGuideText(&keyboard, "Enter a name for your character.");
  swkbdConfigSetStringLenMax(&keyboard, 15);

  if (R_SUCCEEDED(swkbdShow(&keyboard, text, sizeof(text))) &&
      text[0] != '\0') {
    LwlEvent *event = lwl_switch_push(LWL_EVENT_TEXT_INPUT);
    if (event) {
      strncpy(event->text, text, sizeof(event->text) - 1);
    }
  }
  swkbdClose(&keyboard);
}

struct LwlSwitchKeyMapping {
  u64 mask;
  const char *key;
};

static const struct LwlSwitchKeyMapping g_key_mappings[] = {
  {HidNpadButton_Up, "up"},
  {HidNpadButton_Down, "down"},
  {HidNpadButton_Left, "left"},
  {HidNpadButton_Right, "right"},
  {HidNpadButton_A, "return"},
  {HidNpadButton_Plus, "return"},
  {HidNpadButton_B, "escape"},
  {HidNpadButton_Minus, "escape"},
  {HidNpadButton_X, "i"},
  {HidNpadButton_Y, "n"},
  {HidNpadButton_L, "q"},
  {HidNpadButton_R, "r"},
};

static void lwl_switch_sample_input(void) {
  HidTouchScreenState touch;
  u64 pressed;
  u64 released;
  size_t index;

  g_event_count = 0;
  g_event_read = 0;
  if (!g_input_ready) {
    return;
  }

  padUpdate(&g_pad);
  pressed = padGetButtonsDown(&g_pad);
  released = padGetButtonsUp(&g_pad);

  /* ZL is otherwise unused by the game's native controller mapping. */
  if (pressed & HidNpadButton_ZL) {
    lwl_switch_show_keyboard();
  }

  /* Touchscreen -> pointer + primary button. */
  memset(&touch, 0, sizeof(touch));
  if (hidGetTouchScreenStates(&touch, 1) && touch.count > 0) {
    const int x = lwl_switch_clamp(
        (int) touch.touches[0].x, 0, LWL_SWITCH_WIDTH - 1);
    const int y = lwl_switch_clamp(
        (int) touch.touches[0].y, 0, LWL_SWITCH_HEIGHT - 1);
    if (x != g_pointer_x || y != g_pointer_y || !g_was_touching) {
      LwlEvent *move = lwl_switch_push(LWL_EVENT_MOUSE_MOVE);
      if (move) {
        move->dx = x - g_pointer_x;
        move->dy = y - g_pointer_y;
        g_pointer_x = x;
        g_pointer_y = y;
        move->x = g_pointer_x;
        move->y = g_pointer_y;
      } else {
        g_pointer_x = x;
        g_pointer_y = y;
      }
    }
    if (!g_was_touching) {
      lwl_switch_push_pointer(LWL_EVENT_MOUSE_DOWN, 1);
      g_was_touching = true;
    }
  } else if (g_was_touching) {
    lwl_switch_push_pointer(LWL_EVENT_MOUSE_UP, 1);
    g_was_touching = false;
  }

  for (index = 0;
       index < sizeof(g_key_mappings) / sizeof(g_key_mappings[0]);
       ++index) {
    if (pressed & g_key_mappings[index].mask) {
      lwl_switch_push_key(LWL_EVENT_KEY_DOWN, g_key_mappings[index].key);
    }
    if (released & g_key_mappings[index].mask) {
      lwl_switch_push_key(LWL_EVENT_KEY_UP, g_key_mappings[index].key);
    }
  }
}

bool lwl_init(void) {
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&g_pad);
  hidInitializeTouchScreen();
  g_input_ready = true;
  g_was_touching = false;
  g_pointer_x = LWL_SWITCH_WIDTH / 2;
  g_pointer_y = LWL_SWITCH_HEIGHT / 2;
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
  g_window.width = LWL_SWITCH_WIDTH;
  g_window.height = LWL_SWITCH_HEIGHT;
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
    *width = window ? window->width : LWL_SWITCH_WIDTH;
  }
  if (height) {
    *height = window ? window->height : LWL_SWITCH_HEIGHT;
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
  lwl_switch_sample_input();
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
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double) ts.tv_sec + (double) ts.tv_nsec / 1000000000.0;
}

void lwl_sleep_seconds(double seconds) {
  if (seconds <= 0.0) {
    return;
  }
  svcSleepThread((s64) (seconds * 1000000000.0));
}

void lwl_sleep_until_seconds(double time_seconds) {
  const double now = lwl_time_seconds();
  if (time_seconds > now) {
    lwl_sleep_seconds(time_seconds - now);
  }
}

const char *lwl_platform_name(void) {
  return "switch";
}

double lwl_display_scale(void) {
  return 1.0;
}

bool lwl_exe_path(char *buf, int size) {
  (void) buf;
  (void) size;
  return false;
}

bool lwl_data_path(char *buf, int size) {
  (void) buf;
  (void) size;
  return false;
}

LwlGlConfig lwl_gl_config_default(void) {
  LwlGlConfig config;
  memset(&config, 0, sizeof(config));
  config.api = LWL_GL_API_ES;
  config.major_version = 3;
  config.double_buffer = true;
  return config;
}

/* The Switch presents through the libnx framebuffer; no LWL GL context exists. */
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
