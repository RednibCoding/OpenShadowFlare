/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of TWL.
 *
 * TWL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TWL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with TWL. If not, see <https://www.gnu.org/licenses/>.
 */

#include "backend.h"

#include <errno.h>
#include <mach/mach_time.h>
#include <time.h>

static Class twl_macos_delegate_class;
static mach_timebase_info_data_t twl_macos_timebase;

static TwlMacos *twl_macos_from_delegate(id delegate) {
  TwlMacos *macos = NULL;
  object_getInstanceVariable(delegate, "_twlMacos", (void **) &macos);
  return macos;
}

static BOOL twl_macos_window_should_close(
    id delegate, SEL selector, id sender) {
  TwlMacos *macos = twl_macos_from_delegate(delegate);
  TwlEvent event;
  (void) selector;
  (void) sender;
  if (macos && macos->twl) {
    twl_internal_zero(&event, sizeof(event));
    event.type = TWL_EVENT_QUIT;
    event.timestamp_us = twl_backend_time_microseconds(macos->twl);
    twl_internal_push_event(macos->twl, &event);
  }
  return NO;
}

static bool twl_macos_prepare_delegate(void) {
  twl_macos_delegate_class = (Class) objc_getClass("TwlWindowDelegate");
  if (twl_macos_delegate_class) return true;
  twl_macos_delegate_class = objc_allocateClassPair(
    (Class) objc_getClass("NSObject"), "TwlWindowDelegate", 0u);
  if (!twl_macos_delegate_class) return false;
  if (!class_addIvar(
        twl_macos_delegate_class, "_twlMacos", sizeof(void *), 3u, "^v") ||
      !class_addMethod(
        twl_macos_delegate_class, sel_registerName("windowShouldClose:"),
        (IMP) twl_macos_window_should_close, "c@:@")) {
    return false;
  }
  objc_registerClassPair(twl_macos_delegate_class);
  return true;
}

static id twl_macos_create_context(id view) {
  const uint32_t attributes[] = {
    TWL_NS_OPENGL_ACCELERATED,
    TWL_NS_OPENGL_DOUBLE_BUFFER,
    TWL_NS_OPENGL_COLOR_SIZE, 24u,
    0u
  };
  id pixel_format = twl_msg_id(
    (id) objc_getClass("NSOpenGLPixelFormat"), "alloc");
  id context;
  pixel_format = ((id (*)(id, SEL, const uint32_t *)) objc_msgSend)(
    pixel_format, sel_registerName("initWithAttributes:"), attributes);
  if (!pixel_format) return nil;
  context = twl_msg_id((id) objc_getClass("NSOpenGLContext"), "alloc");
  context = ((id (*)(id, SEL, id, id)) objc_msgSend)(
    context, sel_registerName("initWithFormat:shareContext:"),
    pixel_format, nil);
  twl_msg_void(pixel_format, "release");
  if (context) {
    int interval = 1;
    twl_msg_void_id(context, "setView:", view);
    ((void (*)(id, SEL, const int *, TwlNSInteger)) objc_msgSend)(
      context, sel_registerName("setValues:forParameter:"),
      &interval, 222);
    twl_msg_void(context, "makeCurrentContext");
  }
  return context;
}

static size_t twl_macos_controller_offset(void) {
  return twl_internal_align_up(sizeof(TwlMacos), _Alignof(id));
}

size_t twl_backend_memory_alignment(void) {
  return _Alignof(TwlMacos);
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  const size_t offset = twl_macos_controller_offset();
  if (!config || config->controller_capacity >
      (SIZE_MAX - offset) / sizeof(id)) return 0u;
  return offset + (size_t) config->controller_capacity * sizeof(id);
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  TwlMacos *macos;
  void *pool;
  id application;
  TwlNSUInteger style;
  TwlRect rectangle;
  if (!twl || !memory || !config ||
      memory_size < twl_backend_memory_required(config))
    return TWL_RESULT_INVALID_ARGUMENT;
  if (!twl_macos_prepare_delegate()) return TWL_RESULT_BACKEND_FAILURE;
  pool = objc_autoreleasePoolPush();
  macos = (TwlMacos *) memory;
  macos->twl = twl;
  macos->controllers = (id *)
    ((uint8_t *) memory + twl_macos_controller_offset());
  macos->controller_count = config->controller_capacity;
  application = twl_msg_id(
    (id) objc_getClass("NSApplication"), "sharedApplication");
  twl_msg_void_integer(
    application, "setActivationPolicy:", TWL_NS_ACTIVATION_POLICY_REGULAR);
  twl_msg_void(application, "finishLaunching");
  rectangle = twl_rect(100.0, 100.0, config->width, config->height);
  style = TWL_NS_STYLE_TITLED | TWL_NS_STYLE_CLOSABLE |
    TWL_NS_STYLE_MINIATURIZABLE;
  if (config->resizable) style |= TWL_NS_STYLE_RESIZABLE;
  macos->window = twl_msg_id((id) objc_getClass("NSWindow"), "alloc");
  macos->window = ((id (*)(
    id, SEL, TwlRect, TwlNSUInteger, TwlNSUInteger, BOOL)) objc_msgSend)(
      macos->window,
      sel_registerName("initWithContentRect:styleMask:backing:defer:"),
      rectangle, style, TWL_NS_BACKING_STORE_BUFFERED, NO);
  macos->view = twl_msg_id((id) objc_getClass("NSView"), "alloc");
  macos->view = ((id (*)(id, SEL, TwlRect)) objc_msgSend)(
    macos->view, sel_registerName("initWithFrame:"),
    twl_rect(0.0, 0.0, config->width, config->height));
  macos->delegate = twl_msg_id((id) twl_macos_delegate_class, "alloc");
  macos->delegate = twl_msg_id(macos->delegate, "init");
  if (!macos->window || !macos->view || !macos->delegate) {
    objc_autoreleasePoolPop(pool);
    twl_backend_shutdown(twl);
    return TWL_RESULT_BACKEND_FAILURE;
  }
  object_setInstanceVariable(macos->delegate, "_twlMacos", macos);
  twl_msg_void_id(macos->window, "setTitle:", twl_ns_string(config->title));
  twl_msg_void_id(macos->window, "setContentView:", macos->view);
  twl_msg_void_id(macos->window, "setDelegate:", macos->delegate);
  twl_msg_void_bool(macos->window, "setAcceptsMouseMovedEvents:", true);
  twl_msg_void_bool(macos->window, "setReleasedWhenClosed:", false);
  twl_msg_void_id(macos->window, "makeFirstResponder:", macos->view);
  macos->run_loop_mode = twl_msg_id(
    twl_ns_string("kCFRunLoopDefaultMode"), "retain");
  macos->context = twl_macos_create_context(macos->view);
  if (!macos->context) {
    objc_autoreleasePoolPop(pool);
    twl_backend_shutdown(twl);
    return TWL_RESULT_BACKEND_FAILURE;
  }
  if (twl_macos_presentation_init(macos) != TWL_RESULT_OK) {
    objc_autoreleasePoolPop(pool);
    twl_backend_shutdown(twl);
    return TWL_RESULT_BACKEND_FAILURE;
  }
  macos->backing_scale = twl_msg_double(macos->window, "backingScaleFactor");
  if (macos->backing_scale <= 0.0) macos->backing_scale = 1.0;
  twl_msg_void_id(macos->window, "makeKeyAndOrderFront:", nil);
  twl_msg_void_bool(application, "activateIgnoringOtherApps:", true);
  twl_internal_set_display_size(twl, config->width, config->height);
  objc_autoreleasePoolPop(pool);
  return TWL_RESULT_OK;
}

void twl_backend_shutdown(Twl *twl) {
  TwlMacos *macos = twl ? (TwlMacos *) twl->backend : NULL;
  void *pool;
  if (!macos) return;
  pool = objc_autoreleasePoolPush();
  twl_macos_presentation_shutdown(macos);
  if (macos->window) twl_msg_void_id(macos->window, "setDelegate:", nil);
  if (macos->context) {
    twl_msg_void(macos->context, "clearDrawable");
    twl_msg_void(macos->context, "release");
  }
  if (macos->window) {
    twl_msg_void(macos->window, "close");
    twl_msg_void(macos->window, "release");
  }
  if (macos->view) twl_msg_void(macos->view, "release");
  if (macos->delegate) twl_msg_void(macos->delegate, "release");
  if (macos->run_loop_mode)
    twl_msg_void(macos->run_loop_mode, "release");
  macos->context = nil;
  macos->window = nil;
  macos->view = nil;
  macos->delegate = nil;
  macos->run_loop_mode = nil;
  macos->twl = NULL;
  objc_autoreleasePoolPop(pool);
}

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  const uint64_t ticks = mach_absolute_time();
  (void) twl;
  if (twl_macos_timebase.denom == 0u)
    mach_timebase_info(&twl_macos_timebase);
  return ticks * twl_macos_timebase.numer /
    twl_macos_timebase.denom / UINT64_C(1000);
}

void twl_backend_sleep_microseconds(Twl *twl, uint64_t duration) {
  struct timespec delay;
  (void) twl;
  delay.tv_sec = (time_t) (duration / UINT64_C(1000000));
  delay.tv_nsec = (long) ((duration % UINT64_C(1000000)) * UINT64_C(1000));
  while (nanosleep(&delay, &delay) != 0 && errno == EINTR) {}
}
