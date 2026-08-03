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

#ifndef TWL_WEB_BACKEND_H
#define TWL_WEB_BACKEND_H

#include "twl_internal.h"

#include <emscripten.h>
#include <emscripten/html5.h>

typedef struct {
  Twl *twl;
  const char *target;
  int presenter;
} TwlWeb;

int twl_web_prepare_canvas(
  const char *target, const char *title, int width, int height);
void twl_web_release_canvas(int presenter_handle);
EM_BOOL twl_web_mouse(
  int event_type, const EmscriptenMouseEvent *native_event, void *user_data);
EM_BOOL twl_web_wheel(
  int event_type, const EmscriptenWheelEvent *native_event, void *user_data);
EM_BOOL twl_web_keyboard(
  int event_type, const EmscriptenKeyboardEvent *native_event,
  void *user_data);

#endif
