#include "lwl.h"

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspthreadman.h>
#include <psputils.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define LWL_PSP_DEFAULT_WIDTH 640
#define LWL_PSP_DEFAULT_HEIGHT 480

struct LwlWindow {
  int width;
  int height;
};

static struct LwlWindow g_window;

enum { LWL_PSP_EVENT_CAPACITY = 32 };

static LwlEvent g_events[LWL_PSP_EVENT_CAPACITY];
static int g_event_count;
static int g_event_read;
static bool g_polled_this_frame;

static int g_cursor_x = LWL_PSP_DEFAULT_WIDTH / 2;
static int g_cursor_y = LWL_PSP_DEFAULT_HEIGHT / 2;
static unsigned int g_prev_buttons;
static bool g_controller_ready;

static int lwl_psp_clamp_int(int value, int low, int high) {
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

static int lwl_psp_axis_delta(unsigned char axis) {
  enum { DEAD_ZONE = 32, AXIS_RANGE = 127, MAX_SPEED = 10 };
  const int offset = (int) axis - 128;
  const int magnitude = offset < 0 ? -offset : offset;
  float normalized;
  int speed;

  if (magnitude <= DEAD_ZONE) {
    return 0;
  }
  normalized =
      (float) (magnitude - DEAD_ZONE) / (float) (AXIS_RANGE - DEAD_ZONE);
  if (normalized > 1.0f) {
    normalized = 1.0f;
  }
  speed = (int) (normalized * normalized * (float) MAX_SPEED + 0.5f);
  return offset < 0 ? -speed : speed;
}

static LwlEvent *lwl_psp_push_event(LwlEventType type) {
  LwlEvent *event;
  if (g_event_count >= LWL_PSP_EVENT_CAPACITY) {
    return NULL;
  }
  event = &g_events[g_event_count++];
  memset(event, 0, sizeof(*event));
  event->type = type;
  return event;
}

static void lwl_psp_push_pointer(LwlEventType type, int button) {
  LwlEvent *event = lwl_psp_push_event(type);
  if (event) {
    event->x = g_cursor_x;
    event->y = g_cursor_y;
    event->button = button;
    event->clicks = 1;
  }
}

static void lwl_psp_push_key(LwlEventType type, const char *key) {
  LwlEvent *event = lwl_psp_push_event(type);
  if (event) {
    strncpy(event->key, key, sizeof(event->key) - 1);
  }
}

struct LwlPspKeyMapping {
  unsigned int mask;
  const char *key;
};

static const struct LwlPspKeyMapping g_key_mappings[] = {
  {PSP_CTRL_UP, "up"},
  {PSP_CTRL_DOWN, "down"},
  {PSP_CTRL_LEFT, "left"},
  {PSP_CTRL_RIGHT, "right"},
  {PSP_CTRL_START, "return"},
  {PSP_CTRL_SELECT, "escape"},
  {PSP_CTRL_TRIANGLE, "i"},
  {PSP_CTRL_SQUARE, "n"},
  {PSP_CTRL_LTRIGGER, "q"},
  {PSP_CTRL_RTRIGGER, "r"},
};

static void lwl_psp_sample_controller(void) {
  SceCtrlData pad;
  unsigned int buttons;
  unsigned int pressed;
  unsigned int released;
  int delta_x;
  int delta_y;
  size_t index;

  g_event_count = 0;
  g_event_read = 0;

  if (!g_controller_ready || sceCtrlReadBufferPositive(&pad, 1) <= 0) {
    return;
  }
  buttons = pad.Buttons;

  delta_x = lwl_psp_axis_delta(pad.Lx);
  delta_y = lwl_psp_axis_delta(pad.Ly);
  if (delta_x != 0 || delta_y != 0) {
    const int previous_x = g_cursor_x;
    const int previous_y = g_cursor_y;
    LwlEvent *move;
    g_cursor_x = lwl_psp_clamp_int(
        g_cursor_x + delta_x, 0, LWL_PSP_DEFAULT_WIDTH - 1);
    g_cursor_y = lwl_psp_clamp_int(
        g_cursor_y + delta_y, 0, LWL_PSP_DEFAULT_HEIGHT - 1);
    move = lwl_psp_push_event(LWL_EVENT_MOUSE_MOVE);
    if (move) {
      move->x = g_cursor_x;
      move->y = g_cursor_y;
      move->dx = g_cursor_x - previous_x;
      move->dy = g_cursor_y - previous_y;
    }
  }

  pressed = buttons & ~g_prev_buttons;
  released = ~buttons & g_prev_buttons;

  if (pressed & PSP_CTRL_CROSS) {
    lwl_psp_push_pointer(LWL_EVENT_MOUSE_DOWN, 1);
  }
  if (released & PSP_CTRL_CROSS) {
    lwl_psp_push_pointer(LWL_EVENT_MOUSE_UP, 1);
  }
  if (pressed & PSP_CTRL_CIRCLE) {
    lwl_psp_push_pointer(LWL_EVENT_MOUSE_DOWN, 3);
  }
  if (released & PSP_CTRL_CIRCLE) {
    lwl_psp_push_pointer(LWL_EVENT_MOUSE_UP, 3);
  }

  for (index = 0;
       index < sizeof(g_key_mappings) / sizeof(g_key_mappings[0]);
       ++index) {
    if (pressed & g_key_mappings[index].mask) {
      lwl_psp_push_key(LWL_EVENT_KEY_DOWN, g_key_mappings[index].key);
    }
    if (released & g_key_mappings[index].mask) {
      lwl_psp_push_key(LWL_EVENT_KEY_UP, g_key_mappings[index].key);
    }
  }

  g_prev_buttons = buttons;
}

bool lwl_init(void) {
  sceCtrlSetSamplingCycle(0);
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
  g_controller_ready = true;
  g_prev_buttons = 0;
  g_cursor_x = LWL_PSP_DEFAULT_WIDTH / 2;
  g_cursor_y = LWL_PSP_DEFAULT_HEIGHT / 2;
  g_event_count = 0;
  g_event_read = 0;
  g_polled_this_frame = false;
  return true;
}

void lwl_shutdown(void) {
}

LwlWindow *lwl_window_create(const char *title, int width, int height) {
  (void) title;
  g_window.width = width > 0 ? width : LWL_PSP_DEFAULT_WIDTH;
  g_window.height = height > 0 ? height : LWL_PSP_DEFAULT_HEIGHT;
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
  if (!window) {
    return false;
  }
  window->width = width;
  window->height = height;
  return true;
}

void lwl_window_get_size(LwlWindow *window, int *width, int *height) {
  if (width) {
    *width = window ? window->width : LWL_PSP_DEFAULT_WIDTH;
  }
  if (height) {
    *height = window ? window->height : LWL_PSP_DEFAULT_HEIGHT;
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
  lwl_psp_sample_controller();
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
  return (double) sceKernelGetSystemTimeWide() / 1000000.0;
}

void lwl_sleep_seconds(double seconds) {
  if (seconds <= 0.0) {
    return;
  }
  sceKernelDelayThread((SceUInt) (seconds * 1000000.0));
}

void lwl_sleep_until_seconds(double time_seconds) {
  const double now = lwl_time_seconds();
  if (time_seconds > now) {
    lwl_sleep_seconds(time_seconds - now);
  }
}

const char *lwl_platform_name(void) {
  return "psp";
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
