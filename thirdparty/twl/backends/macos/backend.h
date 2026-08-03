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

#ifndef TWL_MACOS_BACKEND_H
#define TWL_MACOS_BACKEND_H

#include "twl_internal.h"

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <objc/message.h>
#include <objc/runtime.h>

extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *pool);

typedef double TwlCGFloat;
typedef struct { TwlCGFloat x; TwlCGFloat y; } TwlPoint;
typedef struct { TwlCGFloat width; TwlCGFloat height; } TwlSize;
typedef struct { TwlPoint origin; TwlSize size; } TwlRect;
typedef unsigned long TwlNSUInteger;
typedef long TwlNSInteger;

enum {
  TWL_NS_ACTIVATION_POLICY_REGULAR = 0,
  TWL_NS_BACKING_STORE_BUFFERED = 2,
  TWL_NS_STYLE_TITLED = 1u << 0u,
  TWL_NS_STYLE_CLOSABLE = 1u << 1u,
  TWL_NS_STYLE_MINIATURIZABLE = 1u << 2u,
  TWL_NS_STYLE_RESIZABLE = 1u << 3u,
  TWL_NS_EVENT_LEFT_DOWN = 1,
  TWL_NS_EVENT_LEFT_UP = 2,
  TWL_NS_EVENT_RIGHT_DOWN = 3,
  TWL_NS_EVENT_RIGHT_UP = 4,
  TWL_NS_EVENT_MOUSE_MOVED = 5,
  TWL_NS_EVENT_LEFT_DRAGGED = 6,
  TWL_NS_EVENT_RIGHT_DRAGGED = 7,
  TWL_NS_EVENT_KEY_DOWN = 10,
  TWL_NS_EVENT_KEY_UP = 11,
  TWL_NS_EVENT_SCROLL = 22,
  TWL_NS_EVENT_OTHER_DOWN = 25,
  TWL_NS_EVENT_OTHER_UP = 26,
  TWL_NS_EVENT_OTHER_DRAGGED = 27,
  TWL_NS_OPENGL_DOUBLE_BUFFER = 5,
  TWL_NS_OPENGL_COLOR_SIZE = 8,
  TWL_NS_OPENGL_ACCELERATED = 73
};

#define TWL_NS_EVENT_MASK_ANY (~(TwlNSUInteger) 0)

typedef struct {
  Twl *twl;
  id window;
  id view;
  id delegate;
  id context;
  id run_loop_mode;
  GLuint program;
  GLuint texture;
  GLint texture_uniform;
  GLint format_uniform;
  uint32_t texture_width;
  uint32_t texture_height;
  TwlPixelFormat texture_format;
  int32_t mouse_x;
  int32_t mouse_y;
  double backing_scale;
  id *controllers;
  uint32_t controller_count;
} TwlMacos;

TwlRect twl_rect(
  TwlCGFloat x, TwlCGFloat y, TwlCGFloat width, TwlCGFloat height);
id twl_msg_id(id object, const char *selector);
id twl_msg_id_id(id object, const char *selector, id argument);
void twl_msg_void(id object, const char *selector);
void twl_msg_void_id(id object, const char *selector, id argument);
void twl_msg_void_bool(id object, const char *selector, bool value);
void twl_msg_void_integer(
  id object, const char *selector, TwlNSInteger value);
bool twl_msg_bool(id object, const char *selector);
TwlNSInteger twl_msg_integer(id object, const char *selector);
TwlNSUInteger twl_msg_count(id object);
id twl_msg_object_at(id object, TwlNSUInteger index);
double twl_msg_double(id object, const char *selector);
float twl_msg_float(id object, const char *selector);
TwlPoint twl_msg_point(id object, const char *selector);
TwlRect twl_msg_rect(id object, const char *selector);
id twl_ns_string(const char *text);
const char *twl_ns_utf8(id string);
id twl_optional_id(id object, const char *selector);
GLuint twl_macos_create_program(void);
TwlResult twl_macos_presentation_init(TwlMacos *macos);
void twl_macos_presentation_shutdown(TwlMacos *macos);

#endif
