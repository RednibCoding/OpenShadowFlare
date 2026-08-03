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

#include <windowsx.h>

static const char twl_win32_class_name[] = "TwlWindow";
static ATOM twl_win32_class;

static LRESULT CALLBACK twl_win32_window_proc(
    HWND window, UINT message, WPARAM word_parameter, LPARAM long_parameter) {
  Twl *twl = (Twl *) GetWindowLongPtrA(window, GWLP_USERDATA);
  TwlEvent event;
  if (message == WM_NCCREATE) {
    const CREATESTRUCTA *create = (const CREATESTRUCTA *) long_parameter;
    twl = (Twl *) create->lpCreateParams;
    SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR) twl);
  }
  if (!twl) return DefWindowProcA(
    window, message, word_parameter, long_parameter);

  twl_internal_zero(&event, sizeof(event));
  event.timestamp_us = twl_backend_time_microseconds(twl);
  switch (message) {
    case WM_CLOSE:
      event.type = TWL_EVENT_QUIT;
      twl_internal_push_event(twl, &event);
      return 0;
    case WM_SIZE:
      if (word_parameter != SIZE_MINIMIZED) {
        event.type = TWL_EVENT_RESIZED;
        event.width = LOWORD(long_parameter);
        event.height = HIWORD(long_parameter);
        twl_internal_set_display_size(
          twl, (uint32_t) event.width, (uint32_t) event.height);
        twl_internal_push_event(twl, &event);
      }
      return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      event.type = TWL_EVENT_KEY_DOWN;
      event.key = twl_win32_key(word_parameter);
      event.repeat = (long_parameter & ((LPARAM) 1 << 30u)) != 0;
      twl_internal_push_event(twl, &event);
      return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
      event.type = TWL_EVENT_KEY_UP;
      event.key = twl_win32_key(word_parameter);
      twl_internal_push_event(twl, &event);
      return 0;
    case WM_CHAR:
      twl_win32_push_text(twl, (uint16_t) word_parameter);
      return 0;
    case WM_LBUTTONDOWN:
      SetCapture(window);
      twl_win32_pointer_event(twl, TWL_EVENT_POINTER_DOWN, long_parameter, 1u);
      return 0;
    case WM_MBUTTONDOWN:
      SetCapture(window);
      twl_win32_pointer_event(twl, TWL_EVENT_POINTER_DOWN, long_parameter, 2u);
      return 0;
    case WM_RBUTTONDOWN:
      SetCapture(window);
      twl_win32_pointer_event(twl, TWL_EVENT_POINTER_DOWN, long_parameter, 3u);
      return 0;
    case WM_LBUTTONUP:
      ReleaseCapture();
      twl_win32_pointer_event(twl, TWL_EVENT_POINTER_UP, long_parameter, 1u);
      return 0;
    case WM_MBUTTONUP:
      ReleaseCapture();
      twl_win32_pointer_event(twl, TWL_EVENT_POINTER_UP, long_parameter, 2u);
      return 0;
    case WM_RBUTTONUP:
      ReleaseCapture();
      twl_win32_pointer_event(twl, TWL_EVENT_POINTER_UP, long_parameter, 3u);
      return 0;
    case WM_MOUSEMOVE:
      twl_win32_pointer_event(twl, TWL_EVENT_POINTER_MOVE, long_parameter, 0u);
      return 0;
    case WM_MOUSEWHEEL: {
      POINT position;
      position.x = GET_X_LPARAM(long_parameter);
      position.y = GET_Y_LPARAM(long_parameter);
      ScreenToClient(window, &position);
      event.type = TWL_EVENT_POINTER_WHEEL;
      event.x = position.x;
      event.y = position.y;
      event.dy = GET_WHEEL_DELTA_WPARAM(word_parameter) / WHEEL_DELTA;
      twl_internal_push_event(twl, &event);
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT: {
      PAINTSTRUCT paint;
      BeginPaint(window, &paint);
      EndPaint(window, &paint);
      return 0;
    }
    default:
      break;
  }
  return DefWindowProcA(window, message, word_parameter, long_parameter);
}

static bool twl_win32_register_class(void) {
  WNDCLASSA window_class;
  if (twl_win32_class != 0) return true;
  twl_internal_zero(&window_class, sizeof(window_class));
  window_class.style = CS_OWNDC;
  window_class.lpfnWndProc = twl_win32_window_proc;
  window_class.hInstance = GetModuleHandleA(NULL);
  window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
  window_class.lpszClassName = twl_win32_class_name;
  twl_win32_class = RegisterClassA(&window_class);
  return twl_win32_class != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

size_t twl_backend_memory_alignment(void) {
  return _Alignof(TwlWin32);
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  (void) config;
  return sizeof(TwlWin32);
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  TwlWin32 *win32;
  RECT rectangle;
  DWORD style;
  if (!twl || !memory || memory_size < sizeof(TwlWin32) || !config)
    return TWL_RESULT_INVALID_ARGUMENT;
  if (!twl_win32_register_class()) return TWL_RESULT_BACKEND_FAILURE;
  win32 = (TwlWin32 *) memory;
  QueryPerformanceFrequency(&win32->counter_frequency);
  style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  if (config->resizable) style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
  rectangle.left = 0;
  rectangle.top = 0;
  rectangle.right = (LONG) config->width;
  rectangle.bottom = (LONG) config->height;
  AdjustWindowRect(&rectangle, style, FALSE);
  win32->window = CreateWindowExA(
    0, twl_win32_class_name,
    config->title ? config->title : "TWL application", style,
    CW_USEDEFAULT, CW_USEDEFAULT,
    rectangle.right - rectangle.left, rectangle.bottom - rectangle.top,
    NULL, NULL, GetModuleHandleA(NULL), twl);
  if (!win32->window) return TWL_RESULT_BACKEND_FAILURE;
  win32->device_context = GetDC(win32->window);
  if (!win32->device_context) {
    DestroyWindow(win32->window);
    win32->window = NULL;
    return TWL_RESULT_BACKEND_FAILURE;
  }
  SetStretchBltMode(win32->device_context, COLORONCOLOR);
  ShowWindow(win32->window, SW_SHOW);
  UpdateWindow(win32->window);
  twl_internal_set_display_size(twl, config->width, config->height);
  return TWL_RESULT_OK;
}

void twl_backend_shutdown(Twl *twl) {
  TwlWin32 *win32 = twl ? (TwlWin32 *) twl->backend : NULL;
  if (!win32) return;
  if (win32->device_context && win32->window)
    ReleaseDC(win32->window, win32->device_context);
  if (win32->window) DestroyWindow(win32->window);
  win32->device_context = NULL;
  win32->window = NULL;
}

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  const TwlWin32 *win32 = twl ? (const TwlWin32 *) twl->backend : NULL;
  LARGE_INTEGER counter;
  uint64_t frequency;
  uint64_t ticks;
  if (!win32 || win32->counter_frequency.QuadPart <= 0 ||
      !QueryPerformanceCounter(&counter)) return 0u;
  frequency = (uint64_t) win32->counter_frequency.QuadPart;
  ticks = (uint64_t) counter.QuadPart;
  return ticks / frequency * UINT64_C(1000000) +
    ticks % frequency * UINT64_C(1000000) / frequency;
}
