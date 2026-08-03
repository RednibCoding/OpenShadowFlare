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

size_t twl_backend_memory_alignment(void) {
  return _Alignof(TwlWeb);
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  (void) config;
  return sizeof(TwlWeb);
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  TwlWeb *web;
  if (!twl || !memory || memory_size < sizeof(TwlWeb) ||
      !config->display_target) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  web = (TwlWeb *) memory;
  web->twl = twl;
  web->target = config->display_target;
  web->presenter = twl_web_prepare_canvas(
    web->target, config->title,
    (int) config->width, (int) config->height);
  if (web->presenter == 0) {
    return TWL_RESULT_BACKEND_UNAVAILABLE;
  }
  emscripten_set_mousedown_callback(web->target, web, false, twl_web_mouse);
  emscripten_set_mouseup_callback(web->target, web, false, twl_web_mouse);
  emscripten_set_mousemove_callback(web->target, web, false, twl_web_mouse);
  emscripten_set_wheel_callback(web->target, web, false, twl_web_wheel);
  emscripten_set_keydown_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, web, false, twl_web_keyboard);
  emscripten_set_keyup_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, web, false, twl_web_keyboard);
  twl_internal_set_display_size(twl, config->width, config->height);
  return TWL_RESULT_OK;
}

void twl_backend_shutdown(Twl *twl) {
  TwlWeb *web = twl ? (TwlWeb *) twl->backend : NULL;
  if (!web) {
    return;
  }
  emscripten_set_mousedown_callback(web->target, NULL, false, NULL);
  emscripten_set_mouseup_callback(web->target, NULL, false, NULL);
  emscripten_set_mousemove_callback(web->target, NULL, false, NULL);
  emscripten_set_wheel_callback(web->target, NULL, false, NULL);
  emscripten_set_keydown_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, NULL);
  emscripten_set_keyup_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, NULL);
  twl_web_release_canvas(web->presenter);
  web->presenter = 0;
  web->twl = NULL;
}

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  (void) twl;
  return (uint64_t) (emscripten_get_now() * 1000.0);
}
