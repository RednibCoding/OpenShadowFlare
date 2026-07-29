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
 * You should have received a copy of the GNU General Public License along
 * with LWL. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * LWL backend for Emscripten / WebAssembly.
 *
 * The whole windowing surface collapses to a single HTML5 <canvas> plus a
 * WebGL 2.0 (OpenGL ES 3.0) context. There is no blocking event loop on the
 * web: browser input arrives asynchronously through the HTML5 event
 * callbacks below, which push LwlEvents into a small ring buffer that
 * lwl_poll_event drains once per requestAnimationFrame tick. The runtime is
 * expected to drive its frame via emscripten_set_main_loop, so the sleep and
 * swap-buffer entry points are no-ops (the browser presents the WebGL
 * drawing buffer automatically at the end of each tick).
 *
 * Everything runs on the browser main thread, so no locking is required
 * between the input callbacks and the game loop.
 */

#include "lwl.h"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <stdlib.h>
#include <string.h>

#define LWL_EM_CANVAS "#canvas"
#define LWL_EVENT_QUEUE_CAPACITY 256

EM_JS(void, lwl_em_set_document_title, (const char *title), {
  document.title = UTF8ToString(title);
})

EM_JS(void, lwl_em_set_cursor_visible, (int visible), {
  var canvas = document.querySelector('#canvas');
  if (canvas) {
    canvas.style.cursor = visible ? 'default' : 'none';
  }
})

struct LwlWindow {
  int width;
  int height;
};

struct LwlGlContext {
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE handle;
};

static LwlEvent g_event_queue[LWL_EVENT_QUEUE_CAPACITY];
static int g_event_head;
static int g_event_tail;
static LwlWindow *g_window;
static bool g_callbacks_registered;

static void push_event(const LwlEvent *event) {
  int next = (g_event_tail + 1) % LWL_EVENT_QUEUE_CAPACITY;
  if (next == g_event_head) {
    g_event_head = (g_event_head + 1) % LWL_EVENT_QUEUE_CAPACITY;
  }
  g_event_queue[g_event_tail] = *event;
  g_event_tail = next;
}

static bool pop_event(LwlEvent *event) {
  if (g_event_head == g_event_tail) {
    return false;
  }
  *event = g_event_queue[g_event_head];
  g_event_head = (g_event_head + 1) % LWL_EVENT_QUEUE_CAPACITY;
  return true;
}

static void copy_key_name(char *out, const char *name) {
  size_t index = 0;
  for (; name[index] != '\0' && index < sizeof(((LwlEvent *) 0)->key) - 1;
       ++index) {
    char character = name[index];
    if (character >= 'A' && character <= 'Z') {
      character = (char) (character - 'A' + 'a');
    }
    out[index] = character;
  }
  out[index] = '\0';
}


static void translate_dom_key(const char *dom_key, char *out) {
  if (strcmp(dom_key, "ArrowUp") == 0) {
    copy_key_name(out, "up");
  } else if (strcmp(dom_key, "ArrowDown") == 0) {
    copy_key_name(out, "down");
  } else if (strcmp(dom_key, "ArrowLeft") == 0) {
    copy_key_name(out, "left");
  } else if (strcmp(dom_key, "ArrowRight") == 0) {
    copy_key_name(out, "right");
  } else if (strcmp(dom_key, "Enter") == 0) {
    copy_key_name(out, "return");
  } else if (strcmp(dom_key, "Escape") == 0) {
    copy_key_name(out, "escape");
  } else if (strcmp(dom_key, "Backspace") == 0) {
    copy_key_name(out, "backspace");
  } else if (strcmp(dom_key, "Delete") == 0) {
    copy_key_name(out, "delete");
  } else if (strcmp(dom_key, "Tab") == 0) {
    copy_key_name(out, "tab");
  } else if (strcmp(dom_key, " ") == 0) {
    copy_key_name(out, "space");
  } else {
    copy_key_name(out, dom_key);
  }
}

static bool is_printable_ascii(const char *dom_key) {
  return dom_key[0] >= 0x20 && dom_key[0] <= 0x7e && dom_key[1] == '\0';
}

static void sync_canvas_size(int *width_out, int *height_out) {
  double css_width = 0.0;
  double css_height = 0.0;
  int width;
  int height;

  if (emscripten_get_element_css_size(LWL_EM_CANVAS, &css_width, &css_height) !=
        EMSCRIPTEN_RESULT_SUCCESS ||
      css_width < 1.0 || css_height < 1.0) {
    if (g_window) {
      if (width_out) {
        *width_out = g_window->width;
      }
      if (height_out) {
        *height_out = g_window->height;
      }
    }
    return;
  }

  width = (int) (css_width + 0.5);
  height = (int) (css_height + 0.5);

  if (g_window && (g_window->width != width || g_window->height != height)) {
    LwlEvent event;
    emscripten_set_canvas_element_size(LWL_EM_CANVAS, width, height);
    g_window->width = width;
    g_window->height = height;
    memset(&event, 0, sizeof(event));
    event.type = LWL_EVENT_RESIZED;
    event.x = width;
    event.y = height;
    push_event(&event);
  }

  if (width_out) {
    *width_out = width;
  }
  if (height_out) {
    *height_out = height;
  }
}

static EM_BOOL on_mouse(int event_type, const EmscriptenMouseEvent *mouse,
                        void *user_data) {
  LwlEvent event;
  (void) user_data;

  memset(&event, 0, sizeof(event));
  event.x = (int) mouse->targetX;
  event.y = (int) mouse->targetY;
  event.button = (int) mouse->button + 1;

  switch (event_type) {
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
      event.type = LWL_EVENT_MOUSE_DOWN;
      event.clicks = 1;
      break;
    case EMSCRIPTEN_EVENT_MOUSEUP:
      event.type = LWL_EVENT_MOUSE_UP;
      event.clicks = 1;
      break;
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
      event.type = LWL_EVENT_MOUSE_MOVE;
      event.dx = (int) mouse->movementX;
      event.dy = (int) mouse->movementY;
      break;
    default:
      return EM_FALSE;
  }

  push_event(&event);
  return EM_TRUE;
}

static EM_BOOL on_wheel(int event_type, const EmscriptenWheelEvent *wheel,
                        void *user_data) {
  LwlEvent event;
  (void) event_type;
  (void) user_data;

  memset(&event, 0, sizeof(event));
  event.type = LWL_EVENT_MOUSE_WHEEL;
  event.x = (int) wheel->mouse.targetX;
  event.y = (int) wheel->mouse.targetY;
  event.dy = wheel->deltaY > 0.0 ? -1 : (wheel->deltaY < 0.0 ? 1 : 0);
  push_event(&event);
  return EM_TRUE;
}

static EM_BOOL on_key(int event_type, const EmscriptenKeyboardEvent *key,
                      void *user_data) {
  LwlEvent event;
  (void) user_data;

  if (key->ctrlKey || key->metaKey || key->altKey) {
    return EM_FALSE;
  }

  memset(&event, 0, sizeof(event));

  if (event_type == EMSCRIPTEN_EVENT_KEYDOWN) {
    event.type = LWL_EVENT_KEY_DOWN;
    translate_dom_key(key->key, event.key);
    push_event(&event);

    if (is_printable_ascii(key->key)) {
      LwlEvent text_event;
      memset(&text_event, 0, sizeof(text_event));
      text_event.type = LWL_EVENT_TEXT_INPUT;
      text_event.text[0] = key->key[0];
      text_event.text[1] = '\0';
      push_event(&text_event);
    }
    return EM_TRUE;
  }

  if (event_type == EMSCRIPTEN_EVENT_KEYUP) {
    event.type = LWL_EVENT_KEY_UP;
    translate_dom_key(key->key, event.key);
    push_event(&event);
    return EM_TRUE;
  }

  return EM_FALSE;
}

static EM_BOOL on_resize(int event_type, const EmscriptenUiEvent *ui,
                         void *user_data) {
  (void) event_type;
  (void) ui;
  (void) user_data;
  sync_canvas_size(NULL, NULL);
  return EM_FALSE;
}

static void register_callbacks(void) {
  if (g_callbacks_registered) {
    return;
  }
  emscripten_set_mousedown_callback(LWL_EM_CANVAS, NULL, EM_FALSE, on_mouse);
  emscripten_set_mouseup_callback(LWL_EM_CANVAS, NULL, EM_FALSE, on_mouse);
  emscripten_set_mousemove_callback(LWL_EM_CANVAS, NULL, EM_FALSE, on_mouse);
  emscripten_set_wheel_callback(LWL_EM_CANVAS, NULL, EM_FALSE, on_wheel);
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_FALSE,
                                  on_key);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_FALSE,
                                on_key);
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_FALSE,
                                 on_resize);
  g_callbacks_registered = true;
}

bool lwl_init(void) {
  g_event_head = 0;
  g_event_tail = 0;
  return true;
}

void lwl_shutdown(void) {
  g_event_head = 0;
  g_event_tail = 0;
}

LwlWindow *lwl_window_create(const char *title, int width, int height) {
  LwlWindow *window = (LwlWindow *) calloc(1, sizeof(*window));
  if (!window) {
    return NULL;
  }
  window->width = width;
  window->height = height;
  g_window = window;

  emscripten_set_canvas_element_size(LWL_EM_CANVAS, width, height);
  lwl_window_set_title(window, title);
  register_callbacks();
  sync_canvas_size(NULL, NULL);
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
  if (title) {
    lwl_em_set_document_title(title);
  }
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
  lwl_em_set_cursor_visible(visible ? 1 : 0);
}

bool lwl_window_set_size(LwlWindow *window, int width, int height) {
  if (!window) {
    return false;
  }
  window->width = width;
  window->height = height;
  return emscripten_set_canvas_element_size(LWL_EM_CANVAS, width, height) ==
         EMSCRIPTEN_RESULT_SUCCESS;
}

void lwl_window_get_size(LwlWindow *window, int *width, int *height) {
  int synced_width = window ? window->width : 0;
  int synced_height = window ? window->height : 0;
  sync_canvas_size(&synced_width, &synced_height);
  if (width) {
    *width = synced_width;
  }
  if (height) {
    *height = synced_height;
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
  if (!event) {
    return false;
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

double lwl_time_seconds(void) { return emscripten_get_now() * 0.001; }

void lwl_sleep_seconds(double seconds) { (void) seconds; }

void lwl_sleep_until_seconds(double time_seconds) { (void) time_seconds; }

const char *lwl_platform_name(void) { return "emscripten"; }

double lwl_display_scale(void) { return 1.0; }

bool lwl_exe_path(char *buf, int size) {
  (void) buf;
  (void) size;
  return false;
}

/* --- OpenGL / WebGL2 ----------------------------------------------------- */

LwlGlConfig lwl_gl_config_default(void) {
  LwlGlConfig config;
  memset(&config, 0, sizeof(config));
  config.major_version = 3;
  config.minor_version = 0;
  config.depth_bits = 24;
  config.stencil_bits = 0;
  config.core_profile = true;
  config.debug = false;
  config.double_buffer = true;
  return config;
}

LwlGlContext *lwl_gl_context_create(LwlWindow *window,
                                    const LwlGlConfig *config) {
  EmscriptenWebGLContextAttributes attributes;
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE handle;
  LwlGlContext *context;
  (void) window;

  emscripten_webgl_init_context_attributes(&attributes);
  attributes.majorVersion = 2;
  attributes.minorVersion = 0;
  attributes.alpha = EM_FALSE;
  attributes.depth = config && config->depth_bits > 0 ? EM_TRUE : EM_FALSE;
  attributes.stencil = config && config->stencil_bits > 0 ? EM_TRUE : EM_FALSE;
  attributes.antialias = EM_FALSE;
  attributes.premultipliedAlpha = EM_TRUE;
  attributes.preserveDrawingBuffer = EM_FALSE;
  attributes.failIfMajorPerformanceCaveat = EM_FALSE;
  attributes.enableExtensionsByDefault = EM_TRUE;

  handle = emscripten_webgl_create_context(LWL_EM_CANVAS, &attributes);
  if (handle <= 0) {
    return NULL;
  }

  context = (LwlGlContext *) calloc(1, sizeof(*context));
  if (!context) {
    emscripten_webgl_destroy_context(handle);
    return NULL;
  }
  context->handle = handle;
  return context;
}

void lwl_gl_context_destroy(LwlGlContext *context) {
  if (!context) {
    return;
  }
  emscripten_webgl_destroy_context(context->handle);
  free(context);
}

bool lwl_gl_context_make_current(LwlGlContext *context) {
  if (!context) {
    return false;
  }
  return emscripten_webgl_make_context_current(context->handle) ==
         EMSCRIPTEN_RESULT_SUCCESS;
}

void lwl_gl_context_swap_buffers(LwlGlContext *context) {
  (void) context;
}

bool lwl_gl_context_set_swap_interval(LwlGlContext *context, int interval) {
  (void) context;
  (void) interval;
  return true;
}

void *lwl_gl_get_proc_address(const char *name) {
  return emscripten_webgl_get_proc_address(name);
}
