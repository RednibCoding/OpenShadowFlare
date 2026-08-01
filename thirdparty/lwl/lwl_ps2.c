/*
 * LWL backend for the Sony PlayStation 2.
 *
 * There is no window system on the PS2. The "window" collapses to a virtual
 * 640x480 screen that the presentation backend fills each frame, so the
 * game sees a fixed-size surface with no resizing or focus events.
 *
 * Input comes from up to two controllers (ports 0 and 1). The D-pad and
 * face buttons are converted into LWL key events using the same key names
 * the other backends use ("up", "down", "return", "escape", and so on).
 * The left analog stick drives a virtual pointer, because the game relies
 * on mouse-style pointing even in menus: the stick moves an accumulated
 * pointer position and the cross button maps to the primary mouse button.
 *
 * The pads are polled on each lwl_poll_event call; everything runs on the
 * game thread, so no locking is required.
 */

#include "lwl.h"

#include <delaythread.h>
#include <libpad.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <tamtypes.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LWL_PS2_PORT_COUNT 2
#define LWL_PS2_EVENT_QUEUE_CAPACITY 64
#define LWL_PS2_POINTER_SPEED 720.0
#define LWL_PS2_STICK_DEADZONE 32
#define LWL_PS2_VIRTUAL_WIDTH 640
#define LWL_PS2_VIRTUAL_HEIGHT 480

struct LwlWindow {
  int width;
  int height;
};

struct LwlGlContext {
  int unused;
};

struct button_key {
  unsigned short bit;
  const char *key;
};

static const struct button_key k_button_keys[] = {
  { PAD_UP, "up" },
  { PAD_DOWN, "down" },
  { PAD_LEFT, "left" },
  { PAD_RIGHT, "right" },
  { PAD_CROSS, "return" },
  { PAD_CIRCLE, "escape" },
  { PAD_TRIANGLE, "m" },
  { PAD_SQUARE, "i" },
  { PAD_L1, "n" },
  { PAD_L2, "h" },
  { PAD_R1, "q" },
  { PAD_R2, "r" },
  { PAD_SELECT, "f12" },
  { PAD_START, "escape" },
};

static LwlEvent g_event_queue[LWL_PS2_EVENT_QUEUE_CAPACITY];
static int g_event_head;
static int g_event_tail;
static LwlWindow *g_window;
static bool g_pad_initialized;
static bool g_iop_modules_loaded;
static unsigned char g_pad_buffer[LWL_PS2_PORT_COUNT][256]
  __attribute__((aligned(64)));
static unsigned short g_prev_buttons[LWL_PS2_PORT_COUNT];
static double g_pointer_x = LWL_PS2_VIRTUAL_WIDTH / 2.0;
static double g_pointer_y = LWL_PS2_VIRTUAL_HEIGHT / 2.0;
static int g_last_emit_x = -1;
static int g_last_emit_y = -1;
static double g_last_poll_time;

static void push_event(const LwlEvent *event) {
  int next = (g_event_tail + 1) % LWL_PS2_EVENT_QUEUE_CAPACITY;
  if (next == g_event_head) {
    g_event_head = (g_event_head + 1) % LWL_PS2_EVENT_QUEUE_CAPACITY;
  }
  g_event_queue[g_event_tail] = *event;
  g_event_tail = next;
}

static bool pop_event(LwlEvent *event) {
  if (g_event_head == g_event_tail) {
    return false;
  }
  *event = g_event_queue[g_event_head];
  g_event_head = (g_event_head + 1) % LWL_PS2_EVENT_QUEUE_CAPACITY;
  return true;
}

static void emit_key(const char *key, bool pressed) {
  LwlEvent event;
  memset(&event, 0, sizeof(event));
  event.type = pressed ? LWL_EVENT_KEY_DOWN : LWL_EVENT_KEY_UP;
  snprintf(event.key, sizeof(event.key), "%s", key);
  push_event(&event);
}

static void emit_pointer_button(bool pressed) {
  LwlEvent event;
  memset(&event, 0, sizeof(event));
  event.type = pressed ? LWL_EVENT_MOUSE_DOWN : LWL_EVENT_MOUSE_UP;
  event.x = (int) g_pointer_x;
  event.y = (int) g_pointer_y;
  event.button = 1;
  event.clicks = 1;
  push_event(&event);
}

static void emit_pointer_move(void) {
  int x = (int) g_pointer_x;
  int y = (int) g_pointer_y;
  if (x == g_last_emit_x && y == g_last_emit_y) {
    return;
  }
  g_last_emit_x = x;
  g_last_emit_y = y;
  LwlEvent event;
  memset(&event, 0, sizeof(event));
  event.type = LWL_EVENT_MOUSE_MOVE;
  event.x = x;
  event.y = y;
  push_event(&event);
}

static void open_pad_port(int port) {
  padPortOpen(port, 0, g_pad_buffer[port]);
  padSetMainMode(port, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
}

static void poll_pad(int port) {
  struct padButtonStatus status;
  int state = padGetState(port, 0);
  unsigned short buttons;
  unsigned short pressed;
  unsigned short released;
  int horizontal;
  int vertical;
  double elapsed;
  double speed;

  if (state != PAD_STATE_STABLE) {
    if (state == PAD_STATE_DISCONN) {
      g_prev_buttons[port] = 0;
    }
    return;
  }
  if (!padRead(port, 0, &status)) {
    return;
  }

  buttons = (unsigned short) (0xffffu ^ status.btns);
  pressed = (unsigned short) (buttons & ~g_prev_buttons[port]);
  released = (unsigned short) (~buttons & g_prev_buttons[port]);
  g_prev_buttons[port] = buttons;

  {
    size_t index;
    for (index = 0; index < sizeof(k_button_keys) / sizeof(k_button_keys[0]);
         ++index) {
      if (pressed & k_button_keys[index].bit) {
        emit_key(k_button_keys[index].key, true);
      }
      if (released & k_button_keys[index].bit) {
        emit_key(k_button_keys[index].key, false);
      }
    }
  }

  if (pressed & PAD_CROSS) {
    emit_pointer_button(true);
  }
  if (released & PAD_CROSS) {
    emit_pointer_button(false);
  }

  horizontal = (int) status.ljoy_h - 128;
  vertical = (int) status.ljoy_v - 128;
  if (horizontal > -LWL_PS2_STICK_DEADZONE &&
      horizontal < LWL_PS2_STICK_DEADZONE) {
    horizontal = 0;
  }
  if (vertical > -LWL_PS2_STICK_DEADZONE &&
      vertical < LWL_PS2_STICK_DEADZONE) {
    vertical = 0;
  }
  if (horizontal == 0 && vertical == 0) {
    return;
  }

  elapsed = lwl_time_seconds() - g_last_poll_time;
  if (elapsed <= 0.0) {
    return;
  }
  g_last_poll_time += elapsed;
  speed = LWL_PS2_POINTER_SPEED * elapsed;

  g_pointer_x += (double) horizontal / 127.0 * speed;
  g_pointer_y += (double) vertical / 127.0 * speed;
  if (g_pointer_x < 0.0) {
    g_pointer_x = 0.0;
  } else if (g_pointer_x > LWL_PS2_VIRTUAL_WIDTH - 1) {
    g_pointer_x = LWL_PS2_VIRTUAL_WIDTH - 1;
  }
  if (g_pointer_y < 0.0) {
    g_pointer_y = 0.0;
  } else if (g_pointer_y > LWL_PS2_VIRTUAL_HEIGHT - 1) {
    g_pointer_y = LWL_PS2_VIRTUAL_HEIGHT - 1;
  }
  emit_pointer_move();
}

bool lwl_init(void) {
  int port;
  g_event_head = 0;
  g_event_tail = 0;
  g_last_poll_time = 0.0;
  if (!g_pad_initialized) {
    if (!g_iop_modules_loaded) {
      static const char *const module_paths[] = {
        "cdrom0:\\IOMANX.IRX;1",
        "cdrom0:\\FILEXIO.IRX;1",
        "cdrom0:\\SIO2MAN.IRX;1",
        "cdrom0:\\PADMAN.IRX;1",
      };
      size_t index;
      for (index = 0; index < sizeof(module_paths) / sizeof(module_paths[0]);
           ++index) {
        SifLoadModule(module_paths[index], 0, NULL);
      }
      g_iop_modules_loaded = true;
    }
    if (padInit(0) == 1) {
      for (port = 0; port < LWL_PS2_PORT_COUNT; ++port) {
        open_pad_port(port);
        g_prev_buttons[port] = 0;
      }
      g_pad_initialized = true;
    }
  }
  return true;
}

void lwl_shutdown(void) {
  g_event_head = 0;
  g_event_tail = 0;
  if (g_pad_initialized) {
    padEnd();
    g_pad_initialized = false;
  }
}

LwlWindow *lwl_window_create(const char *title, int width, int height) {
  LwlWindow *window = (LwlWindow *) calloc(1, sizeof(*window));
  if (!window) {
    return NULL;
  }
  window->width = width;
  window->height = height;
  g_window = window;
  lwl_window_set_title(window, title);
  return window;
}

LwlWindow *lwl_window_create_with_native_message_handler(
    const char *title, int width, int height,
    LwlNativeMessageHandler handler, void *user_data) {
  (void) handler;
  (void) user_data;
  return lwl_window_create(title, width, height);
}

LwlWindow *lwl_window_attach_native(void *native_window, int width,
                                    int height) {
  (void) native_window;
  (void) width;
  (void) height;
  return NULL;
}

void *lwl_window_get_native_handle(LwlWindow *window) {
  (void) window;
  return NULL;
}

void lwl_window_destroy(LwlWindow *window) {
  if (window == g_window) {
    g_window = NULL;
  }
  free(window);
}

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
    *width = window ? window->width : LWL_PS2_VIRTUAL_WIDTH;
  }
  if (height) {
    *height = window ? window->height : LWL_PS2_VIRTUAL_HEIGHT;
  }
}

LwlColor *lwl_window_get_framebuffer(LwlWindow *window, int *width,
                                     int *height) {
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

void lwl_window_update_rects(LwlWindow *window, const LwlRect *rects,
                             int count) {
  (void) window;
  (void) rects;
  (void) count;
}

bool lwl_poll_event(LwlWindow *window, LwlEvent *event) {
  (void) window;
  int port;
  if (!event) {
    return false;
  }
  if (g_pad_initialized && g_last_poll_time == 0.0) {
    g_last_poll_time = lwl_time_seconds();
  }
  for (port = 0; port < LWL_PS2_PORT_COUNT; ++port) {
    poll_pad(port);
  }
  return pop_event(event);
}

bool lwl_wait_event(LwlWindow *window, double timeout_seconds) {
  (void) window;
  (void) timeout_seconds;
  return g_event_head != g_event_tail;
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
  struct timespec timestamp;
  if (clock_gettime(CLOCK_REALTIME, &timestamp) == 0) {
    return (double) timestamp.tv_sec + (double) timestamp.tv_nsec * 1e-9;
  }
  return 0.0;
}

void lwl_sleep_seconds(double seconds) {
  if (seconds <= 0.0) {
    return;
  }
  DelayThread((s32) (seconds * 1000000.0));
}

void lwl_sleep_until_seconds(double time_seconds) {
  double remaining = time_seconds - lwl_time_seconds();
  if (remaining <= 0.0) {
    return;
  }
  DelayThread((s32) (remaining * 1000000.0));
}

const char *lwl_platform_name(void) { return "ps2"; }

double lwl_display_scale(void) { return 1.0; }

bool lwl_exe_path(char *buf, int size) {
  (void) buf;
  (void) size;
  return false;
}

/* --- OpenGL (unavailable on PS2) ----------------------------------------- */

LwlGlConfig lwl_gl_config_default(void) {
  LwlGlConfig config;
  memset(&config, 0, sizeof(config));
  config.api = LWL_GL_API_DESKTOP;
  return config;
}

LwlGlContext *lwl_gl_context_create(LwlWindow *window,
                                    const LwlGlConfig *requested_config) {
  (void) window;
  (void) requested_config;
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
