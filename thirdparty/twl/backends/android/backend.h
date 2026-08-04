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

#ifndef TWL_ANDROID_BACKEND_H
#define TWL_ANDROID_BACKEND_H

#include "twl_internal.h"

#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

typedef struct {
  struct android_app *app;
  EGLDisplay display;
  EGLConfig config;
  EGLContext context;
  EGLSurface surface;
  GLuint program;
  GLuint texture;
  GLuint vbo;
  GLint attr_pos;
  GLint attr_uv;
  GLint uniform_tex;
  uint16_t *staging;
  uint32_t frame_width;
  uint32_t frame_height;
  int32_t surface_width;
  int32_t surface_height;
  int32_t view_x;
  int32_t view_y;
  int32_t view_w;
  int32_t view_h;
  int32_t pointer_x;
  int32_t pointer_y;
  bool gl_ready;
  bool window_ready;
  bool touching;
  bool quit_pushed;
} TwlAndroid;

void twl_android_on_command(struct android_app *app, int32_t command);
int32_t twl_android_on_input(struct android_app *app, AInputEvent *event);
void twl_android_pump_once(TwlAndroid *android, int timeout_ms);

#endif
