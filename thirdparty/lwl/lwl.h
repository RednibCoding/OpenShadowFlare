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

#ifndef LWL_H
#define LWL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LwlWindow LwlWindow;
typedef struct LwlGlContext LwlGlContext;

typedef intptr_t (*LwlNativeMessageHandler)(
  void *native_window, uint32_t message, uintptr_t wparam,
  intptr_t lparam, void *user_data);

typedef struct {
  int x, y, width, height;
} LwlRect;

typedef struct {
  uint8_t r, g, b, a;
} LwlColor;

typedef enum {
  LWL_CURSOR_ARROW,
  LWL_CURSOR_IBEAM,
  LWL_CURSOR_SIZEH,
  LWL_CURSOR_SIZEV,
  LWL_CURSOR_HAND,
} LwlCursor;

typedef enum {
  LWL_WINDOW_NORMAL,
  LWL_WINDOW_MAXIMIZED,
  LWL_WINDOW_FULLSCREEN,
} LwlWindowMode;

typedef enum {
  LWL_EVENT_NONE,
  LWL_EVENT_QUIT,
  LWL_EVENT_RESIZED,
  LWL_EVENT_EXPOSED,
  LWL_EVENT_TEXT_INPUT,
  LWL_EVENT_KEY_DOWN,
  LWL_EVENT_KEY_UP,
  LWL_EVENT_MOUSE_DOWN,
  LWL_EVENT_MOUSE_UP,
  LWL_EVENT_MOUSE_MOVE,
  LWL_EVENT_MOUSE_WHEEL,
} LwlEventType;

typedef struct {
  LwlEventType type;
  int x, y;
  int dx, dy;
  int button;
  int clicks;
  char text[64];
  char key[32];
} LwlEvent;

typedef struct {
  int major_version;
  int minor_version;
  int depth_bits;
  int stencil_bits;
  bool core_profile;
  bool debug;
  bool double_buffer;
} LwlGlConfig;

bool lwl_init(void);
void lwl_shutdown(void);

LwlWindow* lwl_window_create(const char *title, int width, int height);
LwlWindow* lwl_window_create_with_native_message_handler(
  const char *title, int width, int height,
  LwlNativeMessageHandler handler, void *user_data);
LwlWindow* lwl_window_attach_native(void *native_window, int width, int height);
void* lwl_window_get_native_handle(LwlWindow *window);
void lwl_window_destroy(LwlWindow *window);
void lwl_window_show(LwlWindow *window);
void lwl_window_set_title(LwlWindow *window, const char *title);
void lwl_window_set_mode(LwlWindow *window, LwlWindowMode mode);
bool lwl_window_has_focus(LwlWindow *window);
void lwl_window_set_cursor(LwlWindow *window, LwlCursor cursor);
bool lwl_window_set_cursor_image(
  LwlWindow *window, const LwlColor *pixels,
  int width, int height, int hotspot_x, int hotspot_y);
void lwl_window_set_cursor_visible(LwlWindow *window, bool visible);
bool lwl_window_set_size(LwlWindow *window, int width, int height);
void lwl_window_get_size(LwlWindow *window, int *width, int *height);
LwlColor* lwl_window_get_framebuffer(LwlWindow *window, int *width, int *height);
bool lwl_window_resize_framebuffer(LwlWindow *window, int width, int height);
void lwl_window_update_rects(LwlWindow *window, const LwlRect *rects, int count);

bool lwl_poll_event(LwlWindow *window, LwlEvent *event);
bool lwl_wait_event(LwlWindow *window, double timeout_seconds);

char* lwl_clipboard_get(LwlWindow *window);
void lwl_clipboard_set(LwlWindow *window, const char *text);
char* lwl_select_folder(LwlWindow *window, const char *title);
void lwl_free(void *ptr);

double lwl_time_seconds(void);
void lwl_sleep_seconds(double seconds);
void lwl_sleep_until_seconds(double time_seconds);
const char* lwl_platform_name(void);
double lwl_display_scale(void);
bool lwl_exe_path(char *buf, int size);

/*
 * OpenGL contexts are optional. Software-framebuffer users do not need to
 * link OpenGL unless they call these functions.
 */
LwlGlConfig lwl_gl_config_default(void);
LwlGlContext* lwl_gl_context_create(
  LwlWindow *window, const LwlGlConfig *config);
void lwl_gl_context_destroy(LwlGlContext *context);
bool lwl_gl_context_make_current(LwlGlContext *context);
void lwl_gl_context_swap_buffers(LwlGlContext *context);
bool lwl_gl_context_set_swap_interval(
  LwlGlContext *context, int interval);
void* lwl_gl_get_proc_address(const char *name);

#ifdef __cplusplus
}
#endif

#endif
