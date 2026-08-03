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

#include <X11/Xutil.h>

#include <time.h>

static size_t twl_x11_controller_offset(void) {
  return twl_internal_align_up(
    sizeof(TwlX11), _Alignof(TwlX11Controller));
}

size_t twl_backend_memory_alignment(void) {
  return _Alignof(max_align_t);
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  const size_t offset = twl_x11_controller_offset();
  if (!config || config->controller_capacity >
      (SIZE_MAX - offset) / sizeof(TwlX11Controller)) {
    return 0u;
  }
  return offset +
    (size_t) config->controller_capacity * sizeof(TwlX11Controller);
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  static int visual_attributes[] = {
    GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None
  };
  TwlX11 *x11;
  XVisualInfo *visual;
  XSetWindowAttributes attributes;
  XSizeHints hints;
  int screen;

  if (!twl || !memory ||
      memory_size < twl_backend_memory_required(config)) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  x11 = (TwlX11 *) memory;
  x11->controllers = (TwlX11Controller *)
    ((uint8_t *) memory + twl_x11_controller_offset());
  x11->controller_count = config->controller_capacity;
  twl_x11_input_init(x11);

  x11->display = XOpenDisplay(NULL);
  if (!x11->display) {
    return TWL_RESULT_BACKEND_UNAVAILABLE;
  }
  screen = DefaultScreen(x11->display);
  visual = glXChooseVisual(x11->display, screen, visual_attributes);
  if (!visual) {
    XCloseDisplay(x11->display);
    x11->display = NULL;
    return TWL_RESULT_BACKEND_FAILURE;
  }
  x11->colormap = XCreateColormap(
    x11->display, RootWindow(x11->display, screen), visual->visual, AllocNone);
  attributes.colormap = x11->colormap;
  attributes.event_mask =
    ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask |
    ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
  x11->window = XCreateWindow(
    x11->display, RootWindow(x11->display, screen), 0, 0,
    config->width, config->height, 0, visual->depth, InputOutput,
    visual->visual, CWColormap | CWEventMask, &attributes);
  if (!x11->window) {
    XFree(visual);
    XFreeColormap(x11->display, x11->colormap);
    XCloseDisplay(x11->display);
    x11->display = NULL;
    return TWL_RESULT_BACKEND_FAILURE;
  }
  if (config->title) XStoreName(x11->display, x11->window, config->title);
  x11->wm_delete = XInternAtom(x11->display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(x11->display, x11->window, &x11->wm_delete, 1);
  if (!config->resizable) {
    hints.flags = PMinSize | PMaxSize;
    hints.min_width = hints.max_width = (int) config->width;
    hints.min_height = hints.max_height = (int) config->height;
    XSetWMNormalHints(x11->display, x11->window, &hints);
  }
  x11->context = glXCreateContext(x11->display, visual, NULL, True);
  XFree(visual);
  if (!x11->context ||
      !glXMakeCurrent(x11->display, x11->window, x11->context)) {
    if (x11->context) glXDestroyContext(x11->display, x11->context);
    XDestroyWindow(x11->display, x11->window);
    XFreeColormap(x11->display, x11->colormap);
    XCloseDisplay(x11->display);
    x11->display = NULL;
    return TWL_RESULT_BACKEND_FAILURE;
  }
  if (twl_x11_presentation_init(x11) != TWL_RESULT_OK) {
    twl->backend = x11;
    twl_backend_shutdown(twl);
    return TWL_RESULT_BACKEND_FAILURE;
  }
  XMapWindow(x11->display, x11->window);
  XFlush(x11->display);
  twl_internal_set_display_size(twl, config->width, config->height);
  return TWL_RESULT_OK;
}

void twl_backend_shutdown(Twl *twl) {
  TwlX11 *x11 = twl ? (TwlX11 *) twl->backend : NULL;
  if (!x11) return;
  twl_x11_input_shutdown(x11);
  if (!x11->display) return;
  if (x11->context) {
    twl_x11_presentation_shutdown(x11);
    glXMakeCurrent(x11->display, None, NULL);
    glXDestroyContext(x11->display, x11->context);
  }
  if (x11->window) XDestroyWindow(x11->display, x11->window);
  if (x11->colormap) XFreeColormap(x11->display, x11->colormap);
  XCloseDisplay(x11->display);
  x11->display = NULL;
}

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  struct timespec now;
  (void) twl;
  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 0u;
  return (uint64_t) now.tv_sec * UINT64_C(1000000) +
         (uint64_t) now.tv_nsec / UINT64_C(1000);
}
