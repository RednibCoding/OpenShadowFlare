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

#if defined(__APPLE__)

#include "lwl.h"
#include <mach-o/dyld.h>
#include <mach/mach_time.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Autorelease-pool runtime entry points (stable ObjC runtime ABI). */
extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *pool);

/*
 * Keep the backend buildable with a standard macOS C toolchain without making
 * CoreGraphics headers part of LWL's public/build interface. These are the
 * stable 64-bit CoreGraphics C ABI types and entry points used by this file;
 * symbols are still provided by the system CoreGraphics framework at link and
 * runtime. CGFloat is double on every macOS architecture supported here.
 */
typedef double CGFloat;
typedef struct { CGFloat x, y; } CGPoint;
typedef struct { CGFloat width, height; } CGSize;
typedef struct { CGPoint origin; CGSize size; } CGRect;
typedef struct CGColorSpace *CGColorSpaceRef;
typedef struct CGDataProvider *CGDataProviderRef;
typedef struct CGImage *CGImageRef;
typedef struct CGContext *CGContextRef;
typedef unsigned int CGBitmapInfo;
typedef int CGColorRenderingIntent;
typedef void (*CGDataProviderReleaseDataCallback)(
  void *info, const void *data, size_t size);

enum {
  kCGImageAlphaPremultipliedLast = 1,
  kCGImageAlphaPremultipliedFirst = 2,
  kCGImageAlphaLast = 3,
  kCGImageAlphaNoneSkipLast = 5,
  kCGBitmapByteOrder32Big = 4 << 12,
  kCGBitmapByteOrder32Little = 2 << 12,
  kCGRenderingIntentDefault = 0
};

extern CGColorSpaceRef CGColorSpaceCreateDeviceRGB(void);
extern void CGColorSpaceRelease(CGColorSpaceRef space);
extern CGDataProviderRef CGDataProviderCreateWithData(
  void *info, const void *data, size_t size,
  CGDataProviderReleaseDataCallback release_data);
extern void CGDataProviderRelease(CGDataProviderRef provider);
extern CGImageRef CGImageCreate(
  size_t width, size_t height, size_t bits_per_component,
  size_t bits_per_pixel, size_t bytes_per_row,
  CGColorSpaceRef color_space, CGBitmapInfo bitmap_info,
  CGDataProviderRef provider, const CGFloat *decode,
  bool should_interpolate, CGColorRenderingIntent intent);
extern void CGImageRelease(CGImageRef image);
extern void CGContextDrawImage(
  CGContextRef context, CGRect rect, CGImageRef image);

static CGSize CGSizeMake(CGFloat width, CGFloat height) {
  CGSize result = {width, height};
  return result;
}

static CGRect CGRectMake(
    CGFloat x, CGFloat y, CGFloat width, CGFloat height) {
  CGRect result = {{x, y}, {width, height}};
  return result;
}

#ifndef nil
#define nil ((id) 0)
#endif

typedef unsigned long LwlNSUInteger;
typedef long LwlNSInteger;

enum {
  LWL_NS_APPLICATION_ACTIVATION_POLICY_REGULAR = 0,
  LWL_NS_BACKING_STORE_BUFFERED = 2,
  LWL_NS_MODAL_RESPONSE_OK = 1,
  LWL_NS_WINDOW_STYLE_MASK_TITLED = 1 << 0,
  LWL_NS_WINDOW_STYLE_MASK_CLOSABLE = 1 << 1,
  LWL_NS_WINDOW_STYLE_MASK_MINIATURIZABLE = 1 << 2,
  LWL_NS_WINDOW_STYLE_MASK_RESIZABLE = 1 << 3,
  LWL_NS_EVENT_LEFT_MOUSE_DOWN = 1,
  LWL_NS_EVENT_LEFT_MOUSE_UP = 2,
  LWL_NS_EVENT_RIGHT_MOUSE_DOWN = 3,
  LWL_NS_EVENT_RIGHT_MOUSE_UP = 4,
  LWL_NS_EVENT_MOUSE_MOVED = 5,
  LWL_NS_EVENT_LEFT_MOUSE_DRAGGED = 6,
  LWL_NS_EVENT_RIGHT_MOUSE_DRAGGED = 7,
  LWL_NS_EVENT_KEY_DOWN = 10,
  LWL_NS_EVENT_KEY_UP = 11,
  LWL_NS_EVENT_SCROLL_WHEEL = 22,
  LWL_NS_EVENT_OTHER_MOUSE_DOWN = 25,
  LWL_NS_EVENT_OTHER_MOUSE_UP = 26,
  LWL_NS_EVENT_OTHER_MOUSE_DRAGGED = 27
};

#define LWL_NSEVENT_MASK_ANY (~(LwlNSUInteger) 0)
#define LWL_NSEVENT_MODIFIER_FLAG_CONTROL ((LwlNSUInteger) 1 << 18)
#define LWL_NSEVENT_MODIFIER_FLAG_COMMAND ((LwlNSUInteger) 1 << 20)

#define LWL_QUEUE_MAX 64

struct LwlWindow {
  id ns_window;
  id view;
  id delegate;
  LwlColor *pixels;
  int width, height;
  int mouse_x, mouse_y;
  bool cursor_visible;
  id custom_cursor;
  bool use_custom_cursor;
  LwlGlContext *gl_context;
  LwlEvent queue[LWL_QUEUE_MAX];
  int queue_head, queue_tail;
};

struct LwlGlContext {
  LwlWindow *window;
  id handle;
  bool double_buffer;
};

static bool g_initialized;
static Class g_view_class;
static Class g_delegate_class;
static mach_timebase_info_data_t g_timebase;

static id msg_id(id obj, const char *name) {
  return ((id (*)(id, SEL)) objc_msgSend)(obj, sel_registerName(name));
}

static id msg_id_id(id obj, const char *name, id arg) {
  return ((id (*)(id, SEL, id)) objc_msgSend)(obj, sel_registerName(name), arg);
}

static id msg_id_cstr(id obj, const char *name, const char *arg) {
  return ((id (*)(id, SEL, const char*)) objc_msgSend)(obj, sel_registerName(name), arg);
}

static void msg_void(id obj, const char *name) {
  ((void (*)(id, SEL)) objc_msgSend)(obj, sel_registerName(name));
}

static void msg_void_id(id obj, const char *name, id arg) {
  ((void (*)(id, SEL, id)) objc_msgSend)(obj, sel_registerName(name), arg);
}

static void msg_void_bool(id obj, const char *name, bool arg) {
  ((void (*)(id, SEL, BOOL)) objc_msgSend)(obj, sel_registerName(name), arg ? YES : NO);
}

static void msg_void_int(id obj, const char *name, LwlNSInteger arg) {
  ((void (*)(id, SEL, LwlNSInteger)) objc_msgSend)(obj, sel_registerName(name), arg);
}

static bool msg_bool(id obj, const char *name) {
  return ((BOOL (*)(id, SEL)) objc_msgSend)(obj, sel_registerName(name)) != NO;
}

static double msg_double(id obj, const char *name) {
  return ((double (*)(id, SEL)) objc_msgSend)(obj, sel_registerName(name));
}

static LwlNSInteger msg_integer(id obj, const char *name) {
  return ((LwlNSInteger (*)(id, SEL)) objc_msgSend)(obj, sel_registerName(name));
}

static LwlNSUInteger msg_ulong(id obj, const char *name) {
  return ((LwlNSUInteger (*)(id, SEL)) objc_msgSend)(obj, sel_registerName(name));
}

static CGPoint msg_point(id obj, const char *name) {
  return ((CGPoint (*)(id, SEL)) objc_msgSend)(obj, sel_registerName(name));
}

static CGRect msg_rect(id obj, const char *name) {
#if defined(__x86_64__)
  CGRect result;
  ((void (*)(CGRect *, id, SEL)) objc_msgSend_stret)(
    &result, obj, sel_registerName(name));
  return result;
#else
  return ((CGRect (*)(id, SEL)) objc_msgSend)(obj, sel_registerName(name));
#endif
}

static id msg_id_ulong(id obj, const char *name, LwlNSUInteger arg) {
  return ((id (*)(id, SEL, LwlNSUInteger)) objc_msgSend)(obj, sel_registerName(name), arg);
}

static id ns_string(const char *text) {
  return msg_id_cstr((id) objc_getClass("NSString"), "stringWithUTF8String:", text ? text : "");
}

static const char* ns_utf8(id string) {
  if (!string) { return ""; }
  return ((const char* (*)(id, SEL)) objc_msgSend)(string, sel_registerName("UTF8String"));
}

static CGRect active_screen_visible_frame(void) {
  id ns_screen_class = (id) objc_getClass("NSScreen");
  id main_screen = msg_id(ns_screen_class, "mainScreen");
  CGRect selected = main_screen ? msg_rect(main_screen, "visibleFrame") : CGRectMake(0.0, 0.0, 1280.0, 960.0);
  id screens = msg_id(ns_screen_class, "screens");
  CGPoint mouse = msg_point((id) objc_getClass("NSEvent"), "mouseLocation");
  if (!screens) { return selected; }

  LwlNSUInteger count = msg_ulong(screens, "count");
  for (LwlNSUInteger i = 0; i < count; ++i) {
    id screen = msg_id_ulong(screens, "objectAtIndex:", i);
    CGRect frame = msg_rect(screen, "frame");
    if (mouse.x >= frame.origin.x &&
        mouse.x < frame.origin.x + frame.size.width &&
        mouse.y >= frame.origin.y &&
        mouse.y < frame.origin.y + frame.size.height) {
      return msg_rect(screen, "visibleFrame");
    }
  }
  return selected;
}

static char* lwl_strdup(const char *s) {
  size_t n = strlen(s) + 1;
  char *res = (char*) malloc(n);
  if (res) { memcpy(res, s, n); }
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

static LwlWindow* object_window(id object) {
  LwlWindow *window = NULL;
  object_getInstanceVariable(object, "_lwlWindow", (void**) &window);
  return window;
}

static BOOL view_accepts_first_responder(id self, SEL cmd) {
  (void) self;
  (void) cmd;
  return YES;
}

static BOOL view_is_flipped(id self, SEL cmd) {
  (void) self;
  (void) cmd;
  return YES;
}

static void view_draw_rect(id self, SEL cmd, CGRect dirty_rect) {
  (void) cmd;
  (void) dirty_rect;

  LwlWindow *window = object_window(self);
  if (!window || !window->pixels) { return; }

  id graphics_context = msg_id((id) objc_getClass("NSGraphicsContext"), "currentContext");
  CGContextRef context = ((CGContextRef (*)(id, SEL)) objc_msgSend)(
    graphics_context, sel_registerName("CGContext"));
  if (!context) { return; }

  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  if (!color_space) { return; }

  CGDataProviderRef provider = CGDataProviderCreateWithData(
    NULL, window->pixels,
    (size_t) window->width * (size_t) window->height * sizeof(LwlColor),
    NULL);
  if (!provider) {
    CGColorSpaceRelease(color_space);
    return;
  }

  CGImageRef image = CGImageCreate(
    (size_t) window->width,
    (size_t) window->height,
    8,
    32,
    (size_t) window->width * sizeof(LwlColor),
    color_space,
    kCGBitmapByteOrder32Big | kCGImageAlphaNoneSkipLast,
    provider,
    NULL,
    false,
    kCGRenderingIntentDefault);
  if (image) {
    CGContextDrawImage(context, CGRectMake(0.0, 0.0, window->width, window->height), image);
    CGImageRelease(image);
  }

  CGDataProviderRelease(provider);
  CGColorSpaceRelease(color_space);
}

static BOOL delegate_window_should_close(id self, SEL cmd, id sender) {
  (void) cmd;
  (void) sender;
  LwlWindow *window = object_window(self);
  if (window) {
    LwlEvent event;
    memset(&event, 0, sizeof(event));
    event.type = LWL_EVENT_QUIT;
    queue_push(window, &event);
  }
  return NO;
}

static void delegate_window_did_resize(id self, SEL cmd, id notification) {
  (void) cmd;
  (void) notification;
  LwlWindow *window = object_window(self);
  if (!window) { return; }

  CGRect frame = msg_rect(window->view, "bounds");
  int width = (int) frame.size.width;
  int height = (int) frame.size.height;
  if (width < 1) { width = 1; }
  if (height < 1) { height = 1; }
  if (width == window->width && height == window->height) { return; }

  LwlColor *pixels = (LwlColor*) calloc((size_t) width * (size_t) height, sizeof(LwlColor));
  if (!pixels) { return; }
  free(window->pixels);
  window->pixels = pixels;
  window->width = width;
  window->height = height;
  if (window->gl_context && window->gl_context->handle) {
    msg_void(window->gl_context->handle, "update");
  }

  LwlEvent event;
  memset(&event, 0, sizeof(event));
  event.type = LWL_EVENT_RESIZED;
  event.x = width;
  event.y = height;
  queue_push(window, &event);
}

static void ensure_runtime_classes(void) {
  if (!g_view_class) {
    g_view_class = objc_allocateClassPair((Class) objc_getClass("NSView"), "LwlContentView", 0);
    class_addIvar(g_view_class, "_lwlWindow", sizeof(void*), 3, "^v");
    class_addMethod(g_view_class, sel_registerName("acceptsFirstResponder"),
      (IMP) view_accepts_first_responder, "c@:");
    class_addMethod(g_view_class, sel_registerName("isFlipped"), (IMP) view_is_flipped, "c@:");
    class_addMethod(g_view_class, sel_registerName("drawRect:"), (IMP) view_draw_rect, "v@:{CGRect={CGPoint=dd}{CGSize=dd}}");
    objc_registerClassPair(g_view_class);
  }

  if (!g_delegate_class) {
    g_delegate_class = objc_allocateClassPair((Class) objc_getClass("NSObject"), "LwlWindowDelegate", 0);
    class_addIvar(g_delegate_class, "_lwlWindow", sizeof(void*), 3, "^v");
    class_addMethod(g_delegate_class, sel_registerName("windowShouldClose:"),
      (IMP) delegate_window_should_close, "c@:@");
    class_addMethod(g_delegate_class, sel_registerName("windowDidResize:"),
      (IMP) delegate_window_did_resize, "v@:@");
    objc_registerClassPair(g_delegate_class);
  }
}

bool lwl_init(void) {
  if (g_initialized) { return true; }
  ensure_runtime_classes();
  mach_timebase_info(&g_timebase);

  id app = msg_id((id) objc_getClass("NSApplication"), "sharedApplication");
  msg_void_int(app, "setActivationPolicy:", LWL_NS_APPLICATION_ACTIVATION_POLICY_REGULAR);
  msg_void(app, "finishLaunching");
  g_initialized = true;
  return true;
}

void lwl_shutdown(void) {
}

static bool resize_framebuffer(LwlWindow *window, int width, int height) {
  if (width < 1) { width = 1; }
  if (height < 1) { height = 1; }
  if (width == window->width && height == window->height && window->pixels) { return true; }

  LwlColor *pixels = (LwlColor*) calloc((size_t) width * (size_t) height, sizeof(LwlColor));
  if (!pixels) { return false; }
  free(window->pixels);
  window->pixels = pixels;
  window->width = width;
  window->height = height;
  return true;
}

LwlWindow* lwl_window_create(const char *title, int width, int height) {
  if (!lwl_init()) { return NULL; }

  CGRect screen_frame = active_screen_visible_frame();
  if (width <= 0) { width = (int) (screen_frame.size.width * 0.8); }
  if (height <= 0) { height = (int) (screen_frame.size.height * 0.8); }

  LwlWindow *window = (LwlWindow*) calloc(1, sizeof(*window));
  if (!window) { return NULL; }
  if (!resize_framebuffer(window, width, height)) {
    free(window);
    return NULL;
  }
  window->cursor_visible = true;

  CGRect rect = CGRectMake(
    screen_frame.origin.x + (screen_frame.size.width - width) * 0.5,
    screen_frame.origin.y + (screen_frame.size.height - height) * 0.5,
    width,
    height);
  LwlNSUInteger style =
    LWL_NS_WINDOW_STYLE_MASK_TITLED |
    LWL_NS_WINDOW_STYLE_MASK_CLOSABLE |
    LWL_NS_WINDOW_STYLE_MASK_MINIATURIZABLE |
    LWL_NS_WINDOW_STYLE_MASK_RESIZABLE;

  id ns_window = msg_id((id) objc_getClass("NSWindow"), "alloc");
  ns_window = ((id (*)(id, SEL, CGRect, LwlNSUInteger, LwlNSUInteger, BOOL)) objc_msgSend)(
    ns_window,
    sel_registerName("initWithContentRect:styleMask:backing:defer:"),
    rect,
    style,
    LWL_NS_BACKING_STORE_BUFFERED,
    NO);
  if (!ns_window) {
    lwl_window_destroy(window);
    return NULL;
  }
  window->ns_window = ns_window;

  id view = msg_id((id) g_view_class, "alloc");
  view = ((id (*)(id, SEL, CGRect)) objc_msgSend)(
    view, sel_registerName("initWithFrame:"), CGRectMake(0.0, 0.0, width, height));
  id delegate = msg_id((id) g_delegate_class, "alloc");
  delegate = msg_id(delegate, "init");
  window->view = view;
  window->delegate = delegate;
  if (!view || !delegate) {
    lwl_window_destroy(window);
    return NULL;
  }

  object_setInstanceVariable(view, "_lwlWindow", window);
  object_setInstanceVariable(delegate, "_lwlWindow", window);

  msg_void_id(ns_window, "setTitle:", ns_string(title ? title : ""));
  msg_void_id(ns_window, "setContentView:", view);
  msg_void_id(ns_window, "setDelegate:", delegate);
  msg_void_bool(ns_window, "setAcceptsMouseMovedEvents:", true);
  msg_void_bool(ns_window, "setReleasedWhenClosed:", false);
  msg_void_id(ns_window, "makeFirstResponder:", view);
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
  return window ? (void*) window->ns_window : NULL;
}

void lwl_window_destroy(LwlWindow *window) {
  if (!window) { return; }
  if (window->custom_cursor) { msg_void(window->custom_cursor, "release"); }
  if (window->ns_window) {
    msg_void_id(window->ns_window, "setDelegate:", nil);
    msg_void(window->ns_window, "close");
    msg_void(window->ns_window, "release");
  }
  if (window->view) { msg_void(window->view, "release"); }
  if (window->delegate) { msg_void(window->delegate, "release"); }
  free(window->pixels);
  free(window);
}

void lwl_window_show(LwlWindow *window) {
  msg_void_id(window->ns_window, "makeKeyAndOrderFront:", nil);
  msg_void_bool(msg_id((id) objc_getClass("NSApplication"), "sharedApplication"),
    "activateIgnoringOtherApps:", true);
}

void lwl_window_set_title(LwlWindow *window, const char *title) {
  msg_void_id(window->ns_window, "setTitle:", ns_string(title ? title : ""));
}

void lwl_window_set_mode(LwlWindow *window, LwlWindowMode mode) {
  if (mode == LWL_WINDOW_FULLSCREEN) {
    msg_void_id(window->ns_window, "toggleFullScreen:", nil);
  } else if (mode == LWL_WINDOW_MAXIMIZED) {
    if (!msg_bool(window->ns_window, "isZoomed")) {
      msg_void_id(window->ns_window, "zoom:", nil);
    }
  } else {
    if (msg_bool(window->ns_window, "isZoomed")) {
      msg_void_id(window->ns_window, "zoom:", nil);
    }
  }
}

bool lwl_window_has_focus(LwlWindow *window) {
  return msg_bool(window->ns_window, "isKeyWindow");
}

void lwl_window_set_cursor(LwlWindow *window, LwlCursor cursor) {
  const char *selector = "arrowCursor";
  if (cursor == LWL_CURSOR_IBEAM) { selector = "IBeamCursor"; }
  if (cursor == LWL_CURSOR_SIZEH) { selector = "resizeLeftRightCursor"; }
  if (cursor == LWL_CURSOR_SIZEV) { selector = "resizeUpDownCursor"; }
  if (cursor == LWL_CURSOR_HAND) { selector = "pointingHandCursor"; }
  window->use_custom_cursor = false;
  if (window->cursor_visible) {
    msg_void(msg_id((id) objc_getClass("NSCursor"), selector), "set");
  }
}

bool lwl_window_set_cursor_image(
    LwlWindow *window, const LwlColor *pixels,
    int width, int height, int hotspot_x, int hotspot_y) {
  if (!window || !pixels || width < 1 || height < 1) { return false; }

  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  if (!color_space) { return false; }
  CGDataProviderRef provider = CGDataProviderCreateWithData(
    NULL, pixels, (size_t) width * (size_t) height * sizeof(*pixels), NULL);
  if (!provider) {
    CGColorSpaceRelease(color_space);
    return false;
  }
  CGImageRef image = CGImageCreate(
    (size_t) width, (size_t) height, 8, 32,
    (size_t) width * sizeof(*pixels), color_space,
    kCGBitmapByteOrder32Big | kCGImageAlphaLast,
    provider, NULL, false, kCGRenderingIntentDefault);
  CGDataProviderRelease(provider);
  CGColorSpaceRelease(color_space);
  if (!image) { return false; }

  id ns_image = msg_id((id) objc_getClass("NSImage"), "alloc");
  ns_image = ((id (*)(id, SEL, CGImageRef, CGSize)) objc_msgSend)(
    ns_image, sel_registerName("initWithCGImage:size:"),
    image, CGSizeMake(width, height));
  CGImageRelease(image);
  if (!ns_image) { return false; }

  id cursor = msg_id((id) objc_getClass("NSCursor"), "alloc");
  const CGPoint hotspot = {
    hotspot_x < 0 ? 0 : hotspot_x >= width ? width - 1 : hotspot_x,
    hotspot_y < 0 ? 0 : hotspot_y >= height ? height - 1 : hotspot_y
  };
  cursor = ((id (*)(id, SEL, id, CGPoint)) objc_msgSend)(
    cursor, sel_registerName("initWithImage:hotSpot:"), ns_image, hotspot);
  msg_void(ns_image, "release");
  if (!cursor) { return false; }

  if (window->custom_cursor) { msg_void(window->custom_cursor, "release"); }
  window->custom_cursor = cursor;
  window->use_custom_cursor = true;
  if (window->cursor_visible) { msg_void(cursor, "set"); }
  return true;
}

void lwl_window_set_cursor_visible(LwlWindow *window, bool visible) {
  if (!window || window->cursor_visible == visible) { return; }
  window->cursor_visible = visible;
  if (visible && window->use_custom_cursor && window->custom_cursor) {
    msg_void(window->custom_cursor, "set");
  }
  msg_void((id) objc_getClass("NSCursor"), visible ? "unhide" : "hide");
}

bool lwl_window_set_size(LwlWindow *window, int width, int height) {
  if (!window || width < 1 || height < 1) { return false; }
  CGSize size = CGSizeMake((double) width, (double) height);
  ((void (*)(id, SEL, CGSize)) objc_msgSend)(
    window->ns_window, sel_registerName("setContentSize:"), size);
  return resize_framebuffer(window, width, height);
}

void lwl_window_get_size(LwlWindow *window, int *width, int *height) {
  if (width) { *width = window->width; }
  if (height) { *height = window->height; }
}

LwlColor* lwl_window_get_framebuffer(LwlWindow *window, int *width, int *height) {
  if (width) { *width = window->width; }
  if (height) { *height = window->height; }
  return window->pixels;
}

bool lwl_window_resize_framebuffer(LwlWindow *window, int width, int height) {
  return window != NULL && resize_framebuffer(window, width, height);
}

void lwl_window_update_rects(LwlWindow *window, const LwlRect *rects, int count) {
  int i;
  for (i = 0; i < count; i++) {
    const LwlRect *r = &rects[i];
    if (r->width <= 0 || r->height <= 0) { continue; }
    ((void (*)(id, SEL, CGRect)) objc_msgSend)(
      window->view,
      sel_registerName("setNeedsDisplayInRect:"),
      CGRectMake(r->x, r->y, r->width, r->height));
  }
  msg_void(window->view, "displayIfNeeded");
}

static void set_key_name(unsigned short code, char *dst, int size) {
  const char *name = NULL;
  switch (code) {
    case 36: name = "return"; break;
    case 53: name = "escape"; break;
    case 51: name = "backspace"; break;
    case 48: name = "tab"; break;
    case 117: name = "delete"; break;
    case 123: name = "left"; break;
    case 124: name = "right"; break;
    case 126: name = "up"; break;
    case 125: name = "down"; break;
    case 115: name = "home"; break;
    case 119: name = "end"; break;
    case 116: name = "pageup"; break;
    case 121: name = "pagedown"; break;
    case 59: name = "left ctrl"; break;
    case 62: name = "right ctrl"; break;
    case 56: name = "left shift"; break;
    case 60: name = "right shift"; break;
    case 58: name = "left alt"; break;
    case 61: name = "right alt"; break;
    case 49: name = "space"; break;
    case 122: name = "f1"; break;
    case 120: name = "f2"; break;
    case 99: name = "f3"; break;
    case 118: name = "f4"; break;
    case 96: name = "f5"; break;
    case 97: name = "f6"; break;
    case 98: name = "f7"; break;
    case 100: name = "f8"; break;
    case 101: name = "f9"; break;
    case 109: name = "f10"; break;
    case 103: name = "f11"; break;
    case 111: name = "f12"; break;
  }
  snprintf(dst, (size_t) size, "%s", name ? name : "?");
}

static void set_text_key_name(id event, char *dst, int size) {
  id chars = msg_id(event, "charactersIgnoringModifiers");
  const char *text = ns_utf8(chars);
  if (!text[0] || (unsigned char) text[0] < 32) { return; }
  snprintf(dst, (size_t) size, "%s", text);
  if (dst[0] >= 'A' && dst[0] <= 'Z' && dst[1] == '\0') {
    dst[0] = (char) (dst[0] - 'A' + 'a');
  }
}

static void set_mouse_event(LwlWindow *window, id event, LwlEvent *out, LwlEventType type, int button) {
  CGPoint p = msg_point(event, "locationInWindow");
  out->type = type;
  out->x = (int) p.x;
  out->y = window->height - (int) p.y;
  out->dx = out->x - window->mouse_x;
  out->dy = out->y - window->mouse_y;
  out->button = button;
  out->clicks = (int) msg_integer(event, "clickCount");
  window->mouse_x = out->x;
  window->mouse_y = out->y;
}

static bool translate_event(LwlWindow *window, id event, LwlEvent *out) {
  LwlNSInteger type = msg_integer(event, "type");
  memset(out, 0, sizeof(*out));

  if (type == LWL_NS_EVENT_KEY_DOWN || type == LWL_NS_EVENT_KEY_UP) {
    unsigned short code = ((unsigned short (*)(id, SEL)) objc_msgSend)(event, sel_registerName("keyCode"));
    out->type = type == LWL_NS_EVENT_KEY_DOWN ? LWL_EVENT_KEY_DOWN : LWL_EVENT_KEY_UP;
    set_key_name(code, out->key, sizeof(out->key));
    if (strcmp(out->key, "?") == 0) {
      set_text_key_name(event, out->key, sizeof(out->key));
    }
    if (type == LWL_NS_EVENT_KEY_DOWN) {
      id chars = msg_id(event, "characters");
      const char *text = ns_utf8(chars);
      LwlNSUInteger modifiers = msg_ulong(event, "modifierFlags");
      if (text[0] && (unsigned char) text[0] >= 32 &&
          !(modifiers & (LWL_NSEVENT_MODIFIER_FLAG_CONTROL | LWL_NSEVENT_MODIFIER_FLAG_COMMAND))) {
        LwlEvent text_event;
        memset(&text_event, 0, sizeof(text_event));
        text_event.type = LWL_EVENT_TEXT_INPUT;
        snprintf(text_event.text, sizeof(text_event.text), "%s", text);
        queue_push(window, &text_event);
      }
    }
    return true;
  }

  switch (type) {
    case LWL_NS_EVENT_LEFT_MOUSE_DOWN:
      set_mouse_event(window, event, out, LWL_EVENT_MOUSE_DOWN, 1);
      return true;
    case LWL_NS_EVENT_LEFT_MOUSE_UP:
      set_mouse_event(window, event, out, LWL_EVENT_MOUSE_UP, 1);
      return true;
    case LWL_NS_EVENT_RIGHT_MOUSE_DOWN:
      set_mouse_event(window, event, out, LWL_EVENT_MOUSE_DOWN, 3);
      return true;
    case LWL_NS_EVENT_RIGHT_MOUSE_UP:
      set_mouse_event(window, event, out, LWL_EVENT_MOUSE_UP, 3);
      return true;
    case LWL_NS_EVENT_OTHER_MOUSE_DOWN:
      set_mouse_event(window, event, out, LWL_EVENT_MOUSE_DOWN, 2);
      return true;
    case LWL_NS_EVENT_OTHER_MOUSE_UP:
      set_mouse_event(window, event, out, LWL_EVENT_MOUSE_UP, 2);
      return true;
    case LWL_NS_EVENT_MOUSE_MOVED:
    case LWL_NS_EVENT_LEFT_MOUSE_DRAGGED:
    case LWL_NS_EVENT_RIGHT_MOUSE_DRAGGED:
    case LWL_NS_EVENT_OTHER_MOUSE_DRAGGED:
      set_mouse_event(window, event, out, LWL_EVENT_MOUSE_MOVE, 0);
      return true;
    case LWL_NS_EVENT_SCROLL_WHEEL:
      set_mouse_event(window, event, out, LWL_EVENT_MOUSE_WHEEL, 0);
      out->dx = (int) msg_double(event, "scrollingDeltaX");
      out->dy = (int) msg_double(event, "scrollingDeltaY");
      return true;
  }

  return false;
}

bool lwl_poll_event(LwlWindow *window, LwlEvent *event) {
  if (queue_pop(window, event)) { return true; }

  void *pool = objc_autoreleasePoolPush();
  bool got = false;
  id app = msg_id((id) objc_getClass("NSApplication"), "sharedApplication");
  for (;;) {
    id ns_event = ((id (*)(id, SEL, LwlNSUInteger, id, id, BOOL)) objc_msgSend)(
      app,
      sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"),
      LWL_NSEVENT_MASK_ANY,
      nil,
      ns_string("kCFRunLoopDefaultMode"),
      YES);
    if (!ns_event) { break; }
    if (msg_id(ns_event, "window") != window->ns_window) {
      msg_void_id(app, "sendEvent:", ns_event);
      continue;
    }
    if (translate_event(window, ns_event, event)) { got = true; break; }
    msg_void_id(app, "sendEvent:", ns_event);
  }
  objc_autoreleasePoolPop(pool);
  return got;
}

bool lwl_wait_event(LwlWindow *window, double timeout_seconds) {
  if (window->queue_head != window->queue_tail) { return true; }

  void *pool = objc_autoreleasePoolPush();
  id date = nil;
  if (timeout_seconds < 0.0) {
    date = msg_id((id) objc_getClass("NSDate"), "distantFuture");
  } else {
    date = ((id (*)(id, SEL, double)) objc_msgSend)(
      (id) objc_getClass("NSDate"),
      sel_registerName("dateWithTimeIntervalSinceNow:"),
      timeout_seconds);
  }

  id app = msg_id((id) objc_getClass("NSApplication"), "sharedApplication");
  id event = ((id (*)(id, SEL, LwlNSUInteger, id, id, BOOL)) objc_msgSend)(
    app,
    sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"),
    LWL_NSEVENT_MASK_ANY,
    date,
    ns_string("kCFRunLoopDefaultMode"),
    NO);
  bool has_event = event != nil;
  objc_autoreleasePoolPop(pool);
  return has_event;
}

char* lwl_clipboard_get(LwlWindow *window) {
  (void) window;
  id pasteboard = msg_id((id) objc_getClass("NSPasteboard"), "generalPasteboard");
  id type = ns_string("public.utf8-plain-text");
  id string = msg_id_id(pasteboard, "stringForType:", type);
  return lwl_strdup(ns_utf8(string));
}

void lwl_clipboard_set(LwlWindow *window, const char *text) {
  (void) window;
  id pasteboard = msg_id((id) objc_getClass("NSPasteboard"), "generalPasteboard");
  msg_void(pasteboard, "clearContents");
  ((BOOL (*)(id, SEL, id, id)) objc_msgSend)(
    pasteboard,
    sel_registerName("setString:forType:"),
    ns_string(text ? text : ""),
    ns_string("public.utf8-plain-text"));
}

char* lwl_select_folder(LwlWindow *window, const char *title) {
  id panel = msg_id((id) objc_getClass("NSOpenPanel"), "openPanel");
  msg_void_bool(panel, "setCanChooseDirectories:", true);
  msg_void_bool(panel, "setCanChooseFiles:", false);
  msg_void_bool(panel, "setAllowsMultipleSelection:", false);
  msg_void_id(panel, "setTitle:", ns_string(title ? title : "Select Folder"));

  LwlNSInteger result = msg_integer(panel, "runModal");
  if (result != LWL_NS_MODAL_RESPONSE_OK) { return NULL; }

  id url = msg_id(panel, "URL");
  id path = msg_id(url, "path");
  (void) window;
  return lwl_strdup(ns_utf8(path));
}

void lwl_free(void *ptr) {
  free(ptr);
}

double lwl_time_seconds(void) {
  if (g_timebase.denom == 0) { mach_timebase_info(&g_timebase); }
  uint64_t now = mach_absolute_time();
  return (double) now * (double) g_timebase.numer / (double) g_timebase.denom / 1000000000.0;
}

void lwl_sleep_seconds(double seconds) {
  if (seconds <= 0.0) { return; }
  {
    struct timespec ts;
    ts.tv_sec = (time_t) seconds;
    ts.tv_nsec = (long) ((seconds - (double) ts.tv_sec) * 1000000000.0);
    while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {}
  }
}

void lwl_sleep_until_seconds(double time_seconds) {
  double now = lwl_time_seconds();
  if (time_seconds > now) {
    lwl_sleep_seconds(time_seconds - now);
  }
}

const char* lwl_platform_name(void) {
  return "macOS";
}

double lwl_display_scale(void) {
  id screen = msg_id((id) objc_getClass("NSScreen"), "mainScreen");
  return screen ? msg_double(screen, "backingScaleFactor") : 1.0;
}

bool lwl_exe_path(char *buf, int size) {
  uint32_t n = (uint32_t) size;
  return _NSGetExecutablePath(buf, &n) == 0;
}

bool lwl_data_path(char *buf, int size) {
  (void) buf;
  (void) size;
  return false;
}

LwlGlConfig lwl_gl_config_default(void) {
  LwlGlConfig config;
  config.api = LWL_GL_API_DESKTOP;
  config.major_version = 4;
  config.minor_version = 1;
  config.depth_bits = 24;
  config.stencil_bits = 8;
  config.core_profile = true;
  config.debug = false;
  config.double_buffer = true;
  return config;
}

void* lwl_gl_get_proc_address(const char *name) {
  return name && name[0] ? dlsym(RTLD_DEFAULT, name) : NULL;
}

LwlGlContext* lwl_gl_context_create(
    LwlWindow *window, const LwlGlConfig *requested_config) {
  enum {
    LWL_NS_OPENGL_PFA_DOUBLE_BUFFER = 5,
    LWL_NS_OPENGL_PFA_COLOR_SIZE = 8,
    LWL_NS_OPENGL_PFA_ALPHA_SIZE = 11,
    LWL_NS_OPENGL_PFA_DEPTH_SIZE = 12,
    LWL_NS_OPENGL_PFA_STENCIL_SIZE = 13,
    LWL_NS_OPENGL_PFA_ACCELERATED = 73,
    LWL_NS_OPENGL_PFA_OPENGL_PROFILE = 99,
    LWL_NS_OPENGL_PROFILE_VERSION_3_2_CORE = 0x3200,
    LWL_NS_OPENGL_PROFILE_VERSION_4_1_CORE = 0x4100
  };
  LwlGlConfig config;
  uint32_t attributes[20];
  int attribute_count;
  id pixel_format;
  id handle;
  LwlGlContext *context;

  if (!window || !window->view || window->gl_context) { return NULL; }
  config = requested_config ? *requested_config : lwl_gl_config_default();
  if (config.api != LWL_GL_API_DESKTOP ||
      config.major_version < 1 || config.minor_version < 0 ||
      config.depth_bits < 0 || config.stencil_bits < 0 ||
      config.debug) {
    return NULL;
  }
  if (config.core_profile) {
    if (config.major_version < 3 ||
        config.major_version > 4 ||
        (config.major_version == 4 && config.minor_version > 1)) {
      return NULL;
    }
  } else if (config.major_version > 2 ||
             (config.major_version == 2 && config.minor_version > 1)) {
    return NULL;
  }

  attribute_count = 0;
  attributes[attribute_count++] = LWL_NS_OPENGL_PFA_ACCELERATED;
  if (config.double_buffer) {
    attributes[attribute_count++] = LWL_NS_OPENGL_PFA_DOUBLE_BUFFER;
  }
  attributes[attribute_count++] = LWL_NS_OPENGL_PFA_COLOR_SIZE;
  attributes[attribute_count++] = 24;
  attributes[attribute_count++] = LWL_NS_OPENGL_PFA_ALPHA_SIZE;
  attributes[attribute_count++] = 8;
  attributes[attribute_count++] = LWL_NS_OPENGL_PFA_DEPTH_SIZE;
  attributes[attribute_count++] = (uint32_t) config.depth_bits;
  attributes[attribute_count++] = LWL_NS_OPENGL_PFA_STENCIL_SIZE;
  attributes[attribute_count++] = (uint32_t) config.stencil_bits;
  if (config.core_profile) {
    attributes[attribute_count++] = LWL_NS_OPENGL_PFA_OPENGL_PROFILE;
    attributes[attribute_count++] =
      config.major_version > 3 ||
      (config.major_version == 3 && config.minor_version > 2)
        ? LWL_NS_OPENGL_PROFILE_VERSION_4_1_CORE
        : LWL_NS_OPENGL_PROFILE_VERSION_3_2_CORE;
  }
  attributes[attribute_count] = 0;

  pixel_format = msg_id((id) objc_getClass("NSOpenGLPixelFormat"), "alloc");
  pixel_format = ((id (*)(id, SEL, const uint32_t*)) objc_msgSend)(
    pixel_format,
    sel_registerName("initWithAttributes:"),
    attributes);
  if (!pixel_format) { return NULL; }

  handle = msg_id((id) objc_getClass("NSOpenGLContext"), "alloc");
  handle = ((id (*)(id, SEL, id, id)) objc_msgSend)(
    handle,
    sel_registerName("initWithFormat:shareContext:"),
    pixel_format,
    nil);
  msg_void(pixel_format, "release");
  if (!handle) { return NULL; }

  msg_void_id(handle, "setView:", window->view);
  context = (LwlGlContext*) calloc(1, sizeof(*context));
  if (!context) {
    msg_void(handle, "clearDrawable");
    msg_void(handle, "release");
    return NULL;
  }
  context->window = window;
  context->handle = handle;
  context->double_buffer = config.double_buffer;
  window->gl_context = context;
  return context;
}

void lwl_gl_context_destroy(LwlGlContext *context) {
  id current;
  if (!context) { return; }
  current = msg_id((id) objc_getClass("NSOpenGLContext"), "currentContext");
  if (current == context->handle) {
    msg_void((id) objc_getClass("NSOpenGLContext"), "clearCurrentContext");
  }
  if (context->handle) {
    msg_void(context->handle, "clearDrawable");
    msg_void(context->handle, "release");
  }
  if (context->window &&
      context->window->gl_context == context) {
    context->window->gl_context = NULL;
  }
  free(context);
}

bool lwl_gl_context_make_current(LwlGlContext *context) {
  if (!context) {
    msg_void((id) objc_getClass("NSOpenGLContext"), "clearCurrentContext");
    return true;
  }
  msg_void(context->handle, "makeCurrentContext");
  return msg_id(
    (id) objc_getClass("NSOpenGLContext"),
    "currentContext") == context->handle;
}

void lwl_gl_context_swap_buffers(LwlGlContext *context) {
  if (!context || !context->double_buffer) { return; }
  msg_void(context->handle, "flushBuffer");
}

bool lwl_gl_context_set_swap_interval(
    LwlGlContext *context, int interval) {
  enum { LWL_NS_OPENGL_CP_SWAP_INTERVAL = 222 };
  if (!context) { return false; }
  ((void (*)(id, SEL, const int*, LwlNSInteger)) objc_msgSend)(
    context->handle,
    sel_registerName("setValues:forParameter:"),
    &interval,
    LWL_NS_OPENGL_CP_SWAP_INTERVAL);
  return true;
}

#else
typedef int lwl_macos_backend_not_selected;
#endif
