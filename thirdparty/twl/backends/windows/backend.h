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

#ifndef TWL_WINDOWS_BACKEND_H
#define TWL_WINDOWS_BACKEND_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "twl_internal.h"

#include <windows.h>

typedef struct {
  HWND window;
  HDC device_context;
  LARGE_INTEGER counter_frequency;
  int32_t mouse_x;
  int32_t mouse_y;
  uint16_t pending_high_surrogate;
} TwlWin32;

TwlKey twl_win32_key(WPARAM key);
void twl_win32_push_text(Twl *twl, uint16_t unit);
void twl_win32_pointer_event(
  Twl *twl, TwlEventType type, LPARAM parameter, uint8_t button);

#endif
