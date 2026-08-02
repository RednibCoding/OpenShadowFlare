/*
 * Copyright (C) 2026 Michael Binder
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

#if defined(__linux__) && !defined(_WIN32)

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "lwl.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <dlfcn.h>
#ifdef LWL_HAVE_XSHM
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#ifdef LWL_HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#else
typedef int lwl_x11_backend_not_selected;
#endif
#include <errno.h>
#include <locale.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define LWL_QUEUE_MAX 32

struct LwlWindow {
  Display *display;
  int screen;
  Window handle;
  GC gc;
  XImage *image;
#ifdef LWL_HAVE_XSHM
  XShmSegmentInfo shm;
  bool use_shm;
#endif
  XIC ic;
  XIM im;
  Atom wm_delete;
  Atom clipboard_atom;
  Atom utf8_atom;
  Atom targets_atom;
  Atom lwl_selection_atom;
  Cursor cursors[5];
  Cursor custom_cursor;
  Cursor invisible_cursor;
  LwlCursor current_cursor;
  bool use_custom_cursor;
  bool cursor_visible;
  LwlColor *pixels;
  char *clipboard_text;
  int width, height;
  int mouse_x, mouse_y;
  int last_button;
  int last_clicks;
  double last_click_time;
  LwlEvent queue[LWL_QUEUE_MAX];
  int queue_head, queue_tail;
};

struct LwlGlContext {
  LwlWindow *window;
  GLXContext handle;
  bool double_buffer;
};

static Display *g_display;

static int g_x11_error_seen;
static int (*g_prev_x11_error_handler)(Display*, XErrorEvent*);

static int lwl_x11_error_handler(Display *display, XErrorEvent *event) {
  (void) display;
  (void) event;
  g_x11_error_seen = 1;
  return 0;
}

static void begin_x11_error_trap(void) {
  XSync(g_display, False);
  g_x11_error_seen = 0;
  g_prev_x11_error_handler = XSetErrorHandler(lwl_x11_error_handler);
}

static bool end_x11_error_trap(void) {
  XSync(g_display, False);
  XSetErrorHandler(g_prev_x11_error_handler);
  g_prev_x11_error_handler = NULL;
  return g_x11_error_seen == 0;
}

static void destroy_framebuffer(LwlWindow *window) {
  free(window->pixels);
  window->pixels = NULL;
  if (!window->image) {
    return;
  }

#ifdef LWL_HAVE_XSHM
  if (window->use_shm) {
    XSync(window->display, False);
    XShmDetach(window->display, &window->shm);
    XSync(window->display, False);
    window->image->data = NULL;
    XDestroyImage(window->image);
    if (window->shm.shmaddr && window->shm.shmaddr != (char*) -1) {
      shmdt(window->shm.shmaddr);
    }
    if (window->shm.shmid >= 0) {
      shmctl(window->shm.shmid, IPC_RMID, NULL);
    }
    memset(&window->shm, 0, sizeof(window->shm));
    window->shm.shmid = -1;
    window->use_shm = false;
    window->image = NULL;
    return;
  }
#endif

  {
    char *native_pixels = window->image->data;
    window->image->data = NULL;
    XDestroyImage(window->image);
    window->image = NULL;
    free(native_pixels);
  }
}

#ifdef LWL_HAVE_XSHM
static bool create_shm_framebuffer(LwlWindow *window, int width, int height) {
  if (!XShmQueryExtension(window->display)) { return false; }
  memset(&window->shm, 0, sizeof(window->shm));
  window->shm.shmid = -1;
  window->image = XShmCreateImage(
    window->display,
    DefaultVisual(window->display, window->screen),
    DefaultDepth(window->display, window->screen),
    ZPixmap, NULL, &window->shm,
    (unsigned) width, (unsigned) height);
  if (!window->image) { return false; }

  size_t bytes = (size_t) window->image->bytes_per_line * (size_t) window->image->height;
  window->shm.shmid = shmget(IPC_PRIVATE, bytes, IPC_CREAT | 0600);
  if (window->shm.shmid < 0) {
    window->image->data = NULL;
    XDestroyImage(window->image);
    window->image = NULL;
    return false;
  }

  window->shm.shmaddr = (char*) shmat(window->shm.shmid, NULL, 0);
  if (window->shm.shmaddr == (char*) -1) {
    shmctl(window->shm.shmid, IPC_RMID, NULL);
    window->image->data = NULL;
    XDestroyImage(window->image);
    window->image = NULL;
    memset(&window->shm, 0, sizeof(window->shm));
    window->shm.shmid = -1;
    return false;
  }

  window->shm.readOnly = False;
  window->image->data = window->shm.shmaddr;
  begin_x11_error_trap();
  if (!XShmAttach(window->display, &window->shm)) {
    end_x11_error_trap();
    shmdt(window->shm.shmaddr);
    shmctl(window->shm.shmid, IPC_RMID, NULL);
    window->image->data = NULL;
    XDestroyImage(window->image);
    window->image = NULL;
    memset(&window->shm, 0, sizeof(window->shm));
    window->shm.shmid = -1;
    return false;
  }
  if (!end_x11_error_trap()) {
    XShmDetach(window->display, &window->shm);
    XSync(window->display, False);
    shmdt(window->shm.shmaddr);
    shmctl(window->shm.shmid, IPC_RMID, NULL);
    window->image->data = NULL;
    XDestroyImage(window->image);
    window->image = NULL;
    memset(&window->shm, 0, sizeof(window->shm));
    window->shm.shmid = -1;
    return false;
  }
  shmctl(window->shm.shmid, IPC_RMID, NULL);
  memset(window->image->data, 0, bytes);
  window->use_shm = true;
  return true;
}
#endif

static char* lwl_strdup(const char *s) {
  size_t n = strlen(s) + 1;
  char *res = (char*) malloc(n);
  if (res) { memcpy(res, s, n); }
  return res;
}

static char* shell_quote(const char *s) {
  size_t len = 2;
  for (const char *p = s; *p; p++) {
    len += *p == '\'' ? 4 : 1;
  }

  char *res = (char*) malloc(len + 1);
  if (!res) { return NULL; }

  char *out = res;
  *out++ = '\'';
  for (const char *p = s; *p; p++) {
    if (*p == '\'') {
      memcpy(out, "'\\''", 4);
      out += 4;
    } else {
      *out++ = *p;
    }
  }
  *out++ = '\'';
  *out = '\0';
  return res;
}

static bool queue_push(LwlWindow *window, const LwlEvent *event) {
  int next = (window->queue_tail + 1) % LWL_QUEUE_MAX;
  if (next == window->queue_head) { return false; }
  window->queue[window->queue_tail] = *event;
  window->queue_tail = next;
  return true;
}

static bool queue_pop(LwlWindow *window, LwlEvent *event) {
  if (window->queue_head == window->queue_tail) { return false; }
  *event = window->queue[window->queue_head];
  window->queue_head = (window->queue_head + 1) % LWL_QUEUE_MAX;
  return true;
}

bool lwl_init(void) {
  setlocale(LC_CTYPE, "");
  XInitThreads();
  g_display = XOpenDisplay(NULL);
  return g_display != NULL;
}

void lwl_shutdown(void) {
  if (g_display) {
    XCloseDisplay(g_display);
    g_display = NULL;
  }
}

static bool resize_framebuffer(LwlWindow *window, int width, int height) {
  if (width < 1) { width = 1; }
  if (height < 1) { height = 1; }
  if (width == window->width && height == window->height && window->image) {
    return true;
  }

  destroy_framebuffer(window);

#ifdef LWL_HAVE_XSHM
  if (!create_shm_framebuffer(window, width, height))
#endif
  {
    char *native_pixels = (char*) calloc(
      (size_t) width * (size_t) height, sizeof(LwlColor));
    if (!native_pixels) { return false; }

    window->image = XCreateImage(
      window->display,
      DefaultVisual(window->display, window->screen),
      DefaultDepth(window->display, window->screen),
      ZPixmap, 0, native_pixels,
      width, height, 32, width * (int) sizeof(LwlColor));
    if (!window->image) {
      free(native_pixels);
      return false;
    }
  }

  window->pixels = (LwlColor*) calloc(
    (size_t) width * (size_t) height, sizeof(*window->pixels));
  if (!window->pixels) {
    destroy_framebuffer(window);
    return false;
  }
  window->width = width;
  window->height = height;
  return true;
}

static Cursor create_invisible_cursor(Display *display, Window handle) {
  char empty[] = { 0 };
  Pixmap bitmap = XCreateBitmapFromData(display, handle, empty, 1, 1);
  if (!bitmap) { return None; }

  XColor black;
  memset(&black, 0, sizeof(black));
  Cursor cursor = XCreatePixmapCursor(display, bitmap, bitmap, &black, &black, 0, 0);
  XFreePixmap(display, bitmap);
  return cursor;
}

static void apply_cursor(LwlWindow *window) {
  Cursor cursor = window->invisible_cursor;
  if (window->cursor_visible) {
    cursor = window->use_custom_cursor
      ? window->custom_cursor
      : window->cursors[window->current_cursor];
  }
  if (!cursor) {
    cursor = window->cursors[LWL_CURSOR_ARROW];
  }
  XDefineCursor(window->display, window->handle, cursor);
  XFlush(window->display);
}

static void get_default_window_area(Display *display, int screen, int *x, int *y, int *w, int *h) {
  *x = 0;
  *y = 0;
  *w = DisplayWidth(display, screen);
  *h = DisplayHeight(display, screen);

#ifdef LWL_HAVE_XINERAMA
  if (!XineramaIsActive(display)) { return; }

  int count = 0;
  XineramaScreenInfo *screens = XineramaQueryScreens(display, &count);
  if (!screens || count < 1) {
    if (screens) { XFree(screens); }
    return;
  }

  int px = screens[0].x_org;
  int py = screens[0].y_org;
  Window root = RootWindow(display, screen);
  Window root_return, child_return;
  int root_x, root_y, win_x, win_y;
  unsigned int mask;
  if (XQueryPointer(display, root, &root_return, &child_return, &root_x, &root_y, &win_x, &win_y, &mask)) {
    px = root_x;
    py = root_y;
  }

  int selected = 0;
  for (int i = 0; i < count; i++) {
    int sx = screens[i].x_org;
    int sy = screens[i].y_org;
    int sw = screens[i].width;
    int sh = screens[i].height;
    if (px >= sx && px < sx + sw && py >= sy && py < sy + sh) {
      selected = i;
      break;
    }
  }

  *x = screens[selected].x_org;
  *y = screens[selected].y_org;
  *w = screens[selected].width;
  *h = screens[selected].height;
  XFree(screens);
#endif
}

LwlWindow* lwl_window_create(const char *title, int width, int height) {
  if (!g_display && !lwl_init()) { return NULL; }

  LwlWindow *window = (LwlWindow*) calloc(1, sizeof(*window));
  if (!window) { return NULL; }

  window->display = g_display;
  window->screen = DefaultScreen(window->display);
  int area_x, area_y, area_w, area_h;
  get_default_window_area(window->display, window->screen, &area_x, &area_y, &area_w, &area_h);
  if (width <= 0) { width = area_w * 8 / 10; }
  if (height <= 0) { height = area_h * 8 / 10; }
  int window_x = area_x + (area_w - width) / 2;
  int window_y = area_y + (area_h - height) / 2;

  XSetWindowAttributes attrs;
  attrs.event_mask =
    ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask |
    ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask |
    PropertyChangeMask;

  window->handle = XCreateWindow(
    window->display, RootWindow(window->display, window->screen),
    window_x, window_y, (unsigned) width, (unsigned) height, 0,
    CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &attrs);
  if (!window->handle) {
    free(window);
    return NULL;
  }

  XSizeHints size_hints;
  memset(&size_hints, 0, sizeof(size_hints));
  size_hints.flags = USPosition | USSize | PPosition | PSize;
  size_hints.x = window_x;
  size_hints.y = window_y;
  size_hints.width = width;
  size_hints.height = height;
  XSetWMNormalHints(window->display, window->handle, &size_hints);

  window->gc = XCreateGC(window->display, window->handle, 0, NULL);
  window->wm_delete = XInternAtom(window->display, "WM_DELETE_WINDOW", False);
  window->clipboard_atom = XInternAtom(window->display, "CLIPBOARD", False);
  window->utf8_atom = XInternAtom(window->display, "UTF8_STRING", False);
  window->targets_atom = XInternAtom(window->display, "TARGETS", False);
  window->lwl_selection_atom = XInternAtom(window->display, "LWL_SELECTION", False);
  XSetWMProtocols(window->display, window->handle, &window->wm_delete, 1);
  lwl_window_set_title(window, title);

  window->cursors[LWL_CURSOR_ARROW] = XCreateFontCursor(window->display, XC_left_ptr);
  window->cursors[LWL_CURSOR_IBEAM] = XCreateFontCursor(window->display, XC_xterm);
  window->cursors[LWL_CURSOR_SIZEH] = XCreateFontCursor(window->display, XC_sb_h_double_arrow);
  window->cursors[LWL_CURSOR_SIZEV] = XCreateFontCursor(window->display, XC_sb_v_double_arrow);
  window->cursors[LWL_CURSOR_HAND] = XCreateFontCursor(window->display, XC_hand2);
  window->invisible_cursor = create_invisible_cursor(window->display, window->handle);
  window->current_cursor = LWL_CURSOR_ARROW;
  window->cursor_visible = true;
  lwl_window_set_cursor(window, LWL_CURSOR_ARROW);

  window->im = XOpenIM(window->display, NULL, NULL, NULL);
  if (window->im) {
    window->ic = XCreateIC(
      window->im,
      XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
      XNClientWindow, window->handle,
      XNFocusWindow, window->handle,
      NULL);
  }

  if (!resize_framebuffer(window, width, height)) {
    lwl_window_destroy(window);
    return NULL;
  }

  return window;
}

LwlWindow* lwl_window_create_with_native_message_handler(
    const char *title, int width, int height,
    LwlNativeMessageHandler handler, void *user_data) {
  (void) handler;
  (void) user_data;
  return lwl_window_create(title, width, height);
}

LwlWindow* lwl_window_attach_native(void *native_window, int width, int height) {
  (void) native_window;
  (void) width;
  (void) height;
  return NULL;
}

void* lwl_window_get_native_handle(LwlWindow *window) {
  return window ? (void*) (uintptr_t) window->handle : NULL;
}

void lwl_window_destroy(LwlWindow *window) {
  if (!window) { return; }
  if (window->ic) { XDestroyIC(window->ic); }
  if (window->im) { XCloseIM(window->im); }
  for (int i = 0; i < 5; i++) {
    if (window->cursors[i]) { XFreeCursor(window->display, window->cursors[i]); }
  }
  if (window->custom_cursor) {
    XFreeCursor(window->display, window->custom_cursor);
  }
  if (window->invisible_cursor) { XFreeCursor(window->display, window->invisible_cursor); }
  destroy_framebuffer(window);
  free(window->clipboard_text);
  if (window->gc) { XFreeGC(window->display, window->gc); }
  if (window->handle) { XDestroyWindow(window->display, window->handle); }
  free(window);
}

void lwl_window_show(LwlWindow *window) {
  XMapRaised(window->display, window->handle);
  XSync(window->display, False);
}

void lwl_window_set_title(LwlWindow *window, const char *title) {
  XStoreName(window->display, window->handle, title);
}

void lwl_window_set_mode(LwlWindow *window, LwlWindowMode mode) {
  Atom state = XInternAtom(window->display, "_NET_WM_STATE", False);
  Atom fullscreen = XInternAtom(window->display, "_NET_WM_STATE_FULLSCREEN", False);
  Atom maximized_h = XInternAtom(window->display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  Atom maximized_v = XInternAtom(window->display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

  XEvent e;
  memset(&e, 0, sizeof(e));
  e.xclient.type = ClientMessage;
  e.xclient.window = window->handle;
  e.xclient.message_type = state;
  e.xclient.format = 32;
  e.xclient.data.l[0] = (mode == LWL_WINDOW_NORMAL) ? 0 : 1;
  e.xclient.data.l[1] = (mode == LWL_WINDOW_FULLSCREEN) ? fullscreen : maximized_h;
  e.xclient.data.l[2] = (mode == LWL_WINDOW_MAXIMIZED) ? maximized_v : 0;

  XSendEvent(window->display, DefaultRootWindow(window->display), False,
    SubstructureNotifyMask | SubstructureRedirectMask, &e);
}

bool lwl_window_has_focus(LwlWindow *window) {
  Window focus;
  int revert;
  XGetInputFocus(window->display, &focus, &revert);
  return focus == window->handle;
}

void lwl_window_set_cursor(LwlWindow *window, LwlCursor cursor) {
  if (cursor < LWL_CURSOR_ARROW || cursor > LWL_CURSOR_HAND) {
    cursor = LWL_CURSOR_ARROW;
  }
  window->current_cursor = cursor;
  window->use_custom_cursor = false;
  apply_cursor(window);
}

bool lwl_window_set_cursor_image(
    LwlWindow *window, const LwlColor *pixels,
    int width, int height, int hotspot_x, int hotspot_y) {
  if (!window || !pixels || width < 1 || height < 1) { return false; }
  XcursorImage *image = XcursorImageCreate(width, height);
  if (!image) { return false; }
  image->xhot = (XcursorDim) (hotspot_x < 0 ? 0 :
    hotspot_x >= width ? width - 1 : hotspot_x);
  image->yhot = (XcursorDim) (hotspot_y < 0 ? 0 :
    hotspot_y >= height ? height - 1 : hotspot_y);
  for (int index = 0; index < width * height; ++index) {
    const LwlColor pixel = pixels[index];
    image->pixels[index] =
      ((XcursorPixel) pixel.a << 24) |
      ((XcursorPixel) pixel.r << 16) |
      ((XcursorPixel) pixel.g << 8) |
      (XcursorPixel) pixel.b;
  }
  const Cursor cursor = XcursorImageLoadCursor(window->display, image);
  XcursorImageDestroy(image);
  if (!cursor) { return false; }
  if (window->custom_cursor) {
    XFreeCursor(window->display, window->custom_cursor);
  }
  window->custom_cursor = cursor;
  window->use_custom_cursor = true;
  apply_cursor(window);
  return true;
}

void lwl_window_set_cursor_visible(LwlWindow *window, bool visible) {
  if (!window || window->cursor_visible == visible) { return; }
  window->cursor_visible = visible;
  apply_cursor(window);
}

bool lwl_window_set_size(LwlWindow *window, int width, int height) {
  if (!window || width < 1 || height < 1) { return false; }
  XResizeWindow(window->display, window->handle,
                (unsigned) width, (unsigned) height);
  XFlush(window->display);
  return resize_framebuffer(window, width, height);
}

void lwl_window_get_size(LwlWindow *window, int *width, int *height) {
  *width = window->width;
  *height = window->height;
}

LwlColor* lwl_window_get_framebuffer(LwlWindow *window, int *width, int *height) {
  if (width) { *width = window->width; }
  if (height) { *height = window->height; }
  return window->pixels;
}

bool lwl_window_resize_framebuffer(LwlWindow *window, int width, int height) {
  return window != NULL && resize_framebuffer(window, width, height);
}

static unsigned long scale_color_to_mask(uint8_t value, unsigned long mask) {
  unsigned int shift = 0;
  unsigned long maximum;
  if (!mask) { return 0; }
  while (((mask >> shift) & 1ul) == 0ul) { ++shift; }
  maximum = mask >> shift;
  return ((((unsigned long) value * maximum + 127ul) / 255ul) << shift) & mask;
}

static void copy_rgba_to_native(
    LwlWindow *window, const LwlRect *source_rect) {
  LwlRect rect = *source_rect;
  int y;
  if (rect.x < 0) { rect.width += rect.x; rect.x = 0; }
  if (rect.y < 0) { rect.height += rect.y; rect.y = 0; }
  if (rect.x + rect.width > window->width) {
    rect.width = window->width - rect.x;
  }
  if (rect.y + rect.height > window->height) {
    rect.height = window->height - rect.y;
  }
  if (rect.width <= 0 || rect.height <= 0) { return; }

  for (y = rect.y; y < rect.y + rect.height; ++y) {
    int x;
    for (x = rect.x; x < rect.x + rect.width; ++x) {
      const LwlColor color =
        window->pixels[(size_t) y * (size_t) window->width + (size_t) x];
      const unsigned long pixel =
        scale_color_to_mask(color.r, window->image->red_mask) |
        scale_color_to_mask(color.g, window->image->green_mask) |
        scale_color_to_mask(color.b, window->image->blue_mask);
      if (window->image->bits_per_pixel == 32) {
        unsigned char *destination = (unsigned char*) window->image->data +
          (size_t) y * (size_t) window->image->bytes_per_line +
          (size_t) x * 4u;
        if (window->image->byte_order == LSBFirst) {
          destination[0] = (unsigned char) pixel;
          destination[1] = (unsigned char) (pixel >> 8);
          destination[2] = (unsigned char) (pixel >> 16);
          destination[3] = (unsigned char) (pixel >> 24);
        } else {
          destination[0] = (unsigned char) (pixel >> 24);
          destination[1] = (unsigned char) (pixel >> 16);
          destination[2] = (unsigned char) (pixel >> 8);
          destination[3] = (unsigned char) pixel;
        }
      } else {
        XPutPixel(window->image, x, y, pixel);
      }
    }
  }
}

void lwl_window_update_rects(LwlWindow *window, const LwlRect *rects, int count) {
  for (int i = 0; i < count; i++) {
    LwlRect r = rects[i];
    if (r.x < 0) { r.width += r.x; r.x = 0; }
    if (r.y < 0) { r.height += r.y; r.y = 0; }
    if (r.x + r.width > window->width) { r.width = window->width - r.x; }
    if (r.y + r.height > window->height) { r.height = window->height - r.y; }
    if (r.width <= 0 || r.height <= 0) { continue; }
    copy_rgba_to_native(window, &r);
#ifdef LWL_HAVE_XSHM
    if (window->use_shm) {
      XShmPutImage(window->display, window->handle, window->gc, window->image,
        r.x, r.y, r.x, r.y, (unsigned) r.width, (unsigned) r.height, False);
      continue;
    }
#endif
    XPutImage(window->display, window->handle, window->gc, window->image,
      r.x, r.y, r.x, r.y, (unsigned) r.width, (unsigned) r.height);
  }
  XFlush(window->display);
}

static void button_name(int button, LwlEvent *event) {
  event->button = button == Button1 ? 1 : button == Button2 ? 2 : button == Button3 ? 3 : 0;
}

static void set_key_name(KeySym sym, char *dst, int size) {
  const char *name = NULL;
  if (sym >= XK_a && sym <= XK_z) {
    dst[0] = (char) sym;
    dst[1] = '\0';
    return;
  }
  if (sym >= XK_A && sym <= XK_Z) {
    dst[0] = (char) (sym - XK_A + 'a');
    dst[1] = '\0';
    return;
  }
  if (sym >= XK_0 && sym <= XK_9) {
    dst[0] = (char) sym;
    dst[1] = '\0';
    return;
  }
  if (sym >= XK_F1 && sym <= XK_F24) {
    snprintf(dst, size, "f%d", (int) (sym - XK_F1 + 1));
    return;
  }

  switch (sym) {
    case XK_Return: name = "return"; break;
    case XK_KP_Enter: name = "keypad enter"; break;
    case XK_Escape: name = "escape"; break;
    case XK_BackSpace: name = "backspace"; break;
    case XK_Tab: case XK_ISO_Left_Tab: name = "tab"; break;
    case XK_Delete: name = "delete"; break;
    case XK_Left: name = "left"; break;
    case XK_Right: name = "right"; break;
    case XK_Up: name = "up"; break;
    case XK_Down: name = "down"; break;
    case XK_Home: name = "home"; break;
    case XK_End: name = "end"; break;
    case XK_Page_Up: name = "pageup"; break;
    case XK_Page_Down: name = "pagedown"; break;
    case XK_Control_L: name = "left ctrl"; break;
    case XK_Control_R: name = "right ctrl"; break;
    case XK_Shift_L: name = "left shift"; break;
    case XK_Shift_R: name = "right shift"; break;
    case XK_Alt_L: case XK_Meta_L: name = "left alt"; break;
    case XK_Alt_R: case XK_Meta_R: name = "right alt"; break;
    case XK_space: name = "space"; break;
    case XK_slash: name = "/"; break;
    case XK_backslash: name = "\\"; break;
    case XK_bracketleft: name = "["; break;
    case XK_bracketright: name = "]"; break;
    case XK_minus: name = "-"; break;
    case XK_equal: name = "="; break;
    case XK_semicolon: name = ";"; break;
    case XK_apostrophe: name = "'"; break;
    case XK_comma: name = ","; break;
    case XK_period: name = "."; break;
    case XK_grave: name = "`"; break;
  }

  if (name) {
    snprintf(dst, size, "%s", name);
  } else {
    const char *xname = XKeysymToString(sym);
    snprintf(dst, size, "%s", xname ? xname : "?");
    for (char *p = dst; *p; p++) {
      if (*p >= 'A' && *p <= 'Z') { *p = (char) (*p - 'A' + 'a'); }
    }
  }
}

static void handle_selection_request(LwlWindow *window, XSelectionRequestEvent *req) {
  XSelectionEvent reply;
  memset(&reply, 0, sizeof(reply));
  reply.type = SelectionNotify;
  reply.display = req->display;
  reply.requestor = req->requestor;
  reply.selection = req->selection;
  reply.target = req->target;
  reply.time = req->time;
  reply.property = None;

  if (req->target == window->targets_atom) {
    Atom targets[] = { window->targets_atom, window->utf8_atom, XA_STRING };
    XChangeProperty(window->display, req->requestor, req->property,
      XA_ATOM, 32, PropModeReplace, (unsigned char*) targets, 3);
    reply.property = req->property;
  } else if (req->target == window->utf8_atom || req->target == XA_STRING) {
    const char *text = window->clipboard_text ? window->clipboard_text : "";
    XChangeProperty(window->display, req->requestor, req->property,
      req->target, 8, PropModeReplace, (const unsigned char*) text, strlen(text));
    reply.property = req->property;
  }

  XSendEvent(window->display, req->requestor, False, 0, (XEvent*) &reply);
  XFlush(window->display);
}

bool lwl_poll_event(LwlWindow *window, LwlEvent *event) {
  if (queue_pop(window, event)) { return true; }

  while (XPending(window->display)) {
    XEvent xevent;
    XNextEvent(window->display, &xevent);

    if (XFilterEvent(&xevent, window->handle)) { continue; }

    memset(event, 0, sizeof(*event));
    switch (xevent.type) {
      case ClientMessage:
        if ((Atom) xevent.xclient.data.l[0] == window->wm_delete) {
          event->type = LWL_EVENT_QUIT;
          return true;
        }
        break;

      case ConfigureNotify:
        if (xevent.xconfigure.width != window->width ||
            xevent.xconfigure.height != window->height) {
          resize_framebuffer(window, xevent.xconfigure.width, xevent.xconfigure.height);
          event->type = LWL_EVENT_RESIZED;
          event->x = window->width;
          event->y = window->height;
          return true;
        }
        break;

      case Expose:
        event->type = LWL_EVENT_EXPOSED;
        return true;

      case FocusIn:
        if (window->ic) { XSetICFocus(window->ic); }
        break;

      case FocusOut:
        if (window->ic) { XUnsetICFocus(window->ic); }
        break;

      case KeyPress: {
        KeySym sym = NoSymbol;
        char text[64] = {0};
        Status status = 0;
        int len = window->ic
          ? Xutf8LookupString(window->ic, &xevent.xkey, text, sizeof(text) - 1, &sym, &status)
          : XLookupString(&xevent.xkey, text, sizeof(text) - 1, &sym, NULL);
        text[len > 0 ? len : 0] = '\0';

        event->type = LWL_EVENT_KEY_DOWN;
        set_key_name(sym, event->key, sizeof(event->key));

        if (len > 0 && (unsigned char) text[0] >= 32 && (unsigned char) text[0] != 127 && !(xevent.xkey.state & ControlMask)) {
          LwlEvent text_event;
          memset(&text_event, 0, sizeof(text_event));
          text_event.type = LWL_EVENT_TEXT_INPUT;
          snprintf(text_event.text, sizeof(text_event.text), "%s", text);
          queue_push(window, &text_event);
        }
        return true;
      }

      case KeyRelease: {
        KeySym sym = XLookupKeysym(&xevent.xkey, 0);
        event->type = LWL_EVENT_KEY_UP;
        set_key_name(sym, event->key, sizeof(event->key));
        return true;
      }

      case ButtonPress:
        if (xevent.xbutton.button == Button4 || xevent.xbutton.button == Button5) {
          event->type = LWL_EVENT_MOUSE_WHEEL;
          event->x = xevent.xbutton.x;
          event->y = xevent.xbutton.y;
          event->dy = xevent.xbutton.button == Button4 ? 1 : -1;
          window->mouse_x = event->x;
          window->mouse_y = event->y;
          return true;
        }
        event->type = LWL_EVENT_MOUSE_DOWN;
        event->x = xevent.xbutton.x;
        event->y = xevent.xbutton.y;
        button_name(xevent.xbutton.button, event);
        if (event->button == window->last_button &&
            lwl_time_seconds() - window->last_click_time < 0.35) {
          window->last_clicks++;
        } else {
          window->last_clicks = 1;
        }
        window->last_button = event->button;
        window->last_click_time = lwl_time_seconds();
        event->clicks = window->last_clicks;
        return true;

      case ButtonRelease:
        if (xevent.xbutton.button == Button4 || xevent.xbutton.button == Button5) {
          break;
        }
        event->type = LWL_EVENT_MOUSE_UP;
        event->x = xevent.xbutton.x;
        event->y = xevent.xbutton.y;
        button_name(xevent.xbutton.button, event);
        return true;

      case MotionNotify:
        event->type = LWL_EVENT_MOUSE_MOVE;
        event->x = xevent.xmotion.x;
        event->y = xevent.xmotion.y;
        event->dx = event->x - window->mouse_x;
        event->dy = event->y - window->mouse_y;
        window->mouse_x = event->x;
        window->mouse_y = event->y;
        return true;

      case SelectionRequest:
        handle_selection_request(window, &xevent.xselectionrequest);
        break;
    }
  }

  return false;
}

bool lwl_wait_event(LwlWindow *window, double timeout_seconds) {
  if (XPending(window->display)) { return true; }
  struct pollfd pfd;
  pfd.fd = ConnectionNumber(window->display);
  pfd.events = POLLIN;
  pfd.revents = 0;
  int timeout_ms = timeout_seconds < 0 ? -1 : (int) (timeout_seconds * 1000.0);
  return poll(&pfd, 1, timeout_ms) > 0;
}

char* lwl_clipboard_get(LwlWindow *window) {
  if (XGetSelectionOwner(window->display, window->clipboard_atom) == window->handle) {
    return lwl_strdup(window->clipboard_text ? window->clipboard_text : "");
  }

  XConvertSelection(window->display, window->clipboard_atom, window->utf8_atom,
    window->lwl_selection_atom, window->handle, CurrentTime);
  XFlush(window->display);

  double deadline = lwl_time_seconds() + 1.0;
  while (lwl_time_seconds() < deadline) {
    XEvent event;
    if (!XCheckTypedWindowEvent(window->display, window->handle, SelectionNotify, &event)) {
      lwl_wait_event(window, 0.05);
      continue;
    }
    if (event.xselection.property == None) { return lwl_strdup(""); }

    Atom type;
    int format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;
    XGetWindowProperty(window->display, window->handle, event.xselection.property,
      0, 1 << 20, True, AnyPropertyType, &type, &format, &nitems, &bytes_after, &data);
    char *res = data ? lwl_strdup((char*) data) : lwl_strdup("");
    if (data) { XFree(data); }
    return res;
  }

  return lwl_strdup("");
}

void lwl_clipboard_set(LwlWindow *window, const char *text) {
  free(window->clipboard_text);
  window->clipboard_text = lwl_strdup(text ? text : "");
  XSetSelectionOwner(window->display, window->clipboard_atom, window->handle, CurrentTime);
}

char* lwl_select_folder(LwlWindow *window, const char *title) {
  (void) window;
  char *quoted_title = shell_quote(title ? title : "Select Folder");
  if (!quoted_title) { return NULL; }

  const char *command_template =
    "if command -v zenity >/dev/null 2>&1; then "
      "zenity --file-selection --directory --title=%s 2>/dev/null; "
    "elif command -v kdialog >/dev/null 2>&1; then "
      "kdialog --title %s --getexistingdirectory . 2>/dev/null; "
    "elif command -v yad >/dev/null 2>&1; then "
      "yad --file-selection --directory --title=%s 2>/dev/null; "
    "else exit 127; fi";

  int command_len = snprintf(NULL, 0, command_template, quoted_title, quoted_title, quoted_title);
  char *command = (char*) malloc((size_t) command_len + 1);
  if (!command) {
    free(quoted_title);
    return NULL;
  }
  snprintf(command, (size_t) command_len + 1, command_template, quoted_title, quoted_title, quoted_title);
  free(quoted_title);

  FILE *pipe = popen(command, "r");
  free(command);
  if (!pipe) { return NULL; }

  char buf[4096];
  if (!fgets(buf, sizeof(buf), pipe)) {
    pclose(pipe);
    return NULL;
  }
  int status = pclose(pipe);
  if (status != 0) { return NULL; }

  size_t len = strlen(buf);
  while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
    buf[--len] = '\0';
  }
  if (len == 0) { return NULL; }
  return lwl_strdup(buf);
}

void lwl_free(void *ptr) {
  free(ptr);
}

double lwl_time_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

void lwl_sleep_seconds(double seconds) {
  if (seconds <= 0) { return; }
  struct timespec ts;
  ts.tv_sec = (time_t) seconds;
  ts.tv_nsec = (long) ((seconds - ts.tv_sec) * 1000000000.0);
  while (nanosleep(&ts, &ts) < 0 && errno == EINTR) {}
}

void lwl_sleep_until_seconds(double time_seconds) {
  struct timespec ts;
  if (time_seconds <= 0.0) { return; }
  ts.tv_sec = (time_t) time_seconds;
  ts.tv_nsec = (long) ((time_seconds - (double)ts.tv_sec) * 1000000000.0);
  if (ts.tv_nsec < 0) {
    ts.tv_nsec = 0;
  } else if (ts.tv_nsec >= 1000000000L) {
    ts.tv_sec += 1;
    ts.tv_nsec -= 1000000000L;
  }
  while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) < 0 && errno == EINTR) {}
}

const char* lwl_platform_name(void) {
  return "Linux";
}

double lwl_display_scale(void) {
  return 1.0;
}

bool lwl_exe_path(char *buf, int size) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/exe", getpid());
  int len = readlink(path, buf, size - 1);
  if (len < 0) { return false; }
  buf[len] = '\0';
  return true;
}

bool lwl_data_path(char *buf, int size) {
  (void) buf;
  (void) size;
  return false;
}

LwlGlConfig lwl_gl_config_default(void) {
  LwlGlConfig config;
  config.api = LWL_GL_API_DESKTOP;
  config.major_version = 3;
  config.minor_version = 3;
  config.depth_bits = 24;
  config.stencil_bits = 8;
  config.core_profile = true;
  config.debug = false;
  config.double_buffer = true;
  return config;
}

void* lwl_gl_get_proc_address(const char *name) {
  __GLXextFuncPtr function;
  void *address;
  if (!name || !name[0]) { return NULL; }
  function = glXGetProcAddressARB((const GLubyte*) name);
  address = NULL;
  if (function && sizeof(function) == sizeof(address)) {
    memcpy(&address, &function, sizeof(address));
  }
  return address ? address : dlsym(RTLD_DEFAULT, name);
}

LwlGlContext* lwl_gl_context_create(
    LwlWindow *window, const LwlGlConfig *requested_config) {
  typedef GLXContext (*LwlGlxCreateContextAttribs)(
    Display *display, GLXFBConfig config, GLXContext shared,
    Bool direct, const int *attributes);
  enum {
    LWL_GLX_CONTEXT_MAJOR_VERSION = 0x2091,
    LWL_GLX_CONTEXT_MINOR_VERSION = 0x2092,
    LWL_GLX_CONTEXT_FLAGS = 0x2094,
    LWL_GLX_CONTEXT_PROFILE_MASK = 0x9126,
    LWL_GLX_CONTEXT_DEBUG_BIT = 0x0001,
    LWL_GLX_CONTEXT_CORE_PROFILE_BIT = 0x00000001,
    LWL_GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT = 0x00000002
  };
  LwlGlConfig config;
  int framebuffer_attributes[24];
  int framebuffer_attribute_count;
  int context_attributes[9];
  int context_attribute_count;
  int framebuffer_count;
  GLXFBConfig *framebuffers;
  GLXFBConfig selected;
  VisualID window_visual;
  int index;
  LwlGlxCreateContextAttribs create_context;
  GLXContext handle;
  LwlGlContext *context;

  if (!window || !window->display || !window->handle) { return NULL; }
  config = requested_config ? *requested_config : lwl_gl_config_default();
  if (config.api != LWL_GL_API_DESKTOP ||
      config.major_version < 1 || config.minor_version < 0 ||
      config.depth_bits < 0 || config.stencil_bits < 0 ||
      (config.core_profile &&
       (config.major_version < 3 ||
        (config.major_version == 3 && config.minor_version < 2)))) {
    return NULL;
  }

  framebuffer_attribute_count = 0;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_X_RENDERABLE;
  framebuffer_attributes[framebuffer_attribute_count++] = True;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_DRAWABLE_TYPE;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_WINDOW_BIT;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_RENDER_TYPE;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_RGBA_BIT;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_X_VISUAL_TYPE;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_TRUE_COLOR;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_RED_SIZE;
  framebuffer_attributes[framebuffer_attribute_count++] = 8;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_GREEN_SIZE;
  framebuffer_attributes[framebuffer_attribute_count++] = 8;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_BLUE_SIZE;
  framebuffer_attributes[framebuffer_attribute_count++] = 8;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_ALPHA_SIZE;
  framebuffer_attributes[framebuffer_attribute_count++] = 8;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_DEPTH_SIZE;
  framebuffer_attributes[framebuffer_attribute_count++] = config.depth_bits;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_STENCIL_SIZE;
  framebuffer_attributes[framebuffer_attribute_count++] = config.stencil_bits;
  framebuffer_attributes[framebuffer_attribute_count++] = GLX_DOUBLEBUFFER;
  framebuffer_attributes[framebuffer_attribute_count++] =
    config.double_buffer ? True : False;
  framebuffer_attributes[framebuffer_attribute_count] = None;

  framebuffers = glXChooseFBConfig(
    window->display, window->screen,
    framebuffer_attributes, &framebuffer_count);
  if (!framebuffers || framebuffer_count < 1) {
    if (framebuffers) { XFree(framebuffers); }
    return NULL;
  }

  selected = NULL;
  window_visual = XVisualIDFromVisual(
    DefaultVisual(window->display, window->screen));
  for (index = 0; index < framebuffer_count; ++index) {
    XVisualInfo *visual =
      glXGetVisualFromFBConfig(window->display, framebuffers[index]);
    if (visual && visual->visualid == window_visual) {
      selected = framebuffers[index];
      XFree(visual);
      break;
    }
    if (visual) { XFree(visual); }
  }
  if (!selected) {
    XFree(framebuffers);
    return NULL;
  }

  {
    void *address =
      lwl_gl_get_proc_address("glXCreateContextAttribsARB");
    create_context = NULL;
    if (address && sizeof(address) == sizeof(create_context)) {
      memcpy(&create_context, &address, sizeof(create_context));
    }
  }
  if (!create_context) {
    XFree(framebuffers);
    return NULL;
  }

  context_attribute_count = 0;
  context_attributes[context_attribute_count++] =
    LWL_GLX_CONTEXT_MAJOR_VERSION;
  context_attributes[context_attribute_count++] = config.major_version;
  context_attributes[context_attribute_count++] =
    LWL_GLX_CONTEXT_MINOR_VERSION;
  context_attributes[context_attribute_count++] = config.minor_version;
  if (config.major_version > 3 ||
      (config.major_version == 3 && config.minor_version >= 2)) {
    context_attributes[context_attribute_count++] =
      LWL_GLX_CONTEXT_PROFILE_MASK;
    context_attributes[context_attribute_count++] = config.core_profile
      ? LWL_GLX_CONTEXT_CORE_PROFILE_BIT
      : LWL_GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT;
  }
  if (config.debug) {
    context_attributes[context_attribute_count++] = LWL_GLX_CONTEXT_FLAGS;
    context_attributes[context_attribute_count++] =
      LWL_GLX_CONTEXT_DEBUG_BIT;
  }
  context_attributes[context_attribute_count] = None;

  begin_x11_error_trap();
  handle = create_context(
    window->display, selected, NULL, True, context_attributes);
  if (!end_x11_error_trap()) {
    handle = NULL;
  }
  XFree(framebuffers);
  if (!handle) { return NULL; }

  context = (LwlGlContext*) calloc(1, sizeof(*context));
  if (!context) {
    glXDestroyContext(window->display, handle);
    return NULL;
  }
  context->window = window;
  context->handle = handle;
  context->double_buffer = config.double_buffer;
  return context;
}

void lwl_gl_context_destroy(LwlGlContext *context) {
  if (!context) { return; }
  if (glXGetCurrentContext() == context->handle) {
    glXMakeCurrent(context->window->display, None, NULL);
  }
  if (context->handle) {
    glXDestroyContext(context->window->display, context->handle);
  }
  free(context);
}

bool lwl_gl_context_make_current(LwlGlContext *context) {
  if (!context) {
    return g_display
      ? glXMakeCurrent(g_display, None, NULL) == True
      : false;
  }
  return glXMakeCurrent(
    context->window->display,
    context->window->handle,
    context->handle) == True;
}

void lwl_gl_context_swap_buffers(LwlGlContext *context) {
  if (!context || !context->double_buffer) { return; }
  glXSwapBuffers(context->window->display, context->window->handle);
}

bool lwl_gl_context_set_swap_interval(
    LwlGlContext *context, int interval) {
  typedef void (*LwlGlxSwapIntervalExt)(
    Display *display, GLXDrawable drawable, int interval);
  typedef int (*LwlGlxSwapIntervalMesa)(unsigned int interval);
  typedef int (*LwlGlxSwapIntervalSgi)(int interval);
  LwlGlxSwapIntervalExt swap_ext;
  LwlGlxSwapIntervalMesa swap_mesa;
  LwlGlxSwapIntervalSgi swap_sgi;
  if (!context || !lwl_gl_context_make_current(context)) { return false; }

  {
    void *address = lwl_gl_get_proc_address("glXSwapIntervalEXT");
    swap_ext = NULL;
    if (address && sizeof(address) == sizeof(swap_ext)) {
      memcpy(&swap_ext, &address, sizeof(swap_ext));
    }
  }
  if (swap_ext) {
    swap_ext(
      context->window->display, context->window->handle, interval);
    return true;
  }
  {
    void *address = lwl_gl_get_proc_address("glXSwapIntervalMESA");
    swap_mesa = NULL;
    if (address && sizeof(address) == sizeof(swap_mesa)) {
      memcpy(&swap_mesa, &address, sizeof(swap_mesa));
    }
  }
  if (swap_mesa && interval >= 0) {
    return swap_mesa((unsigned int) interval) == 0;
  }
  {
    void *address = lwl_gl_get_proc_address("glXSwapIntervalSGI");
    swap_sgi = NULL;
    if (address && sizeof(address) == sizeof(swap_sgi)) {
      memcpy(&swap_sgi, &address, sizeof(swap_sgi));
    }
  }
  return swap_sgi && interval > 0 && swap_sgi(interval) == 0;
}

#endif
