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

#ifndef TWL_LINUX_BACKEND_H
#define TWL_LINUX_BACKEND_H

#define _POSIX_C_SOURCE 200809L
#define GL_GLEXT_PROTOTYPES

#include "twl_internal.h"

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

typedef struct {
  int descriptor;
  uint64_t next_open_attempt_us;
} TwlX11Controller;

typedef struct {
  Display *display;
  Window window;
  Colormap colormap;
  GLXContext context;
  Atom wm_delete;
  GLuint program;
  GLuint texture;
  GLint texture_uniform;
  GLint format_uniform;
  uint32_t texture_width;
  uint32_t texture_height;
  TwlPixelFormat texture_format;
  TwlX11Controller *controllers;
  uint32_t controller_count;
} TwlX11;

GLuint twl_x11_create_program(void);
TwlResult twl_x11_presentation_init(TwlX11 *x11);
void twl_x11_presentation_shutdown(TwlX11 *x11);
void twl_x11_input_init(TwlX11 *x11);
void twl_x11_input_shutdown(TwlX11 *x11);

#endif
