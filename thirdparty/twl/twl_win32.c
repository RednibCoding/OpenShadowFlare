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

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "twl_internal.h"

#include <windows.h>
#include <windowsx.h>
#include <xinput.h>

typedef struct {
  BITMAPINFOHEADER header;
  DWORD masks[3];
} TwlWin32Bitmap;

typedef struct {
  HWND window;
  HDC device_context;
  LARGE_INTEGER counter_frequency;
  int32_t mouse_x;
  int32_t mouse_y;
  uint16_t pending_high_surrogate;
} TwlWin32;

static const char twl_win32_class_name[] = "TwlWindow";
static ATOM twl_win32_class;

static TwlKey twl_win32_key(WPARAM key) {
  if (key >= '0' && key <= '9')
    return (TwlKey) (TWL_KEY_0 + key - '0');
  if (key >= 'A' && key <= 'Z')
    return (TwlKey) (TWL_KEY_A + key - 'A');
  if (key >= VK_F1 && key <= VK_F12)
    return (TwlKey) (TWL_KEY_F1 + key - VK_F1);
  switch (key) {
    case VK_BACK: return TWL_KEY_BACKSPACE;
    case VK_TAB: return TWL_KEY_TAB;
    case VK_RETURN: return TWL_KEY_RETURN;
    case VK_ESCAPE: return TWL_KEY_ESCAPE;
    case VK_SPACE: return TWL_KEY_SPACE;
    case VK_DELETE: return TWL_KEY_DELETE;
    case VK_LEFT: return TWL_KEY_LEFT;
    case VK_RIGHT: return TWL_KEY_RIGHT;
    case VK_UP: return TWL_KEY_UP;
    case VK_DOWN: return TWL_KEY_DOWN;
    case VK_HOME: return TWL_KEY_HOME;
    case VK_END: return TWL_KEY_END;
    case VK_PRIOR: return TWL_KEY_PAGE_UP;
    case VK_NEXT: return TWL_KEY_PAGE_DOWN;
    case VK_INSERT: return TWL_KEY_INSERT;
    default: return TWL_KEY_UNKNOWN;
  }
}

static void twl_win32_push_text(Twl *twl, uint16_t unit) {
  TwlWin32 *win32 = (TwlWin32 *) twl->backend;
  TwlEvent event;
  uint32_t codepoint;
  if (unit >= 0xd800u && unit <= 0xdbffu) {
    win32->pending_high_surrogate = unit;
    return;
  }
  if (unit >= 0xdc00u && unit <= 0xdfffu &&
      win32->pending_high_surrogate != 0u) {
    codepoint = UINT32_C(0x10000) +
      ((uint32_t) (win32->pending_high_surrogate - 0xd800u) << 10u) +
      (uint32_t) (unit - 0xdc00u);
  } else {
    codepoint = unit;
  }
  win32->pending_high_surrogate = 0u;
  if (codepoint < 0x20u || codepoint == 0x7fu) return;
  twl_internal_zero(&event, sizeof(event));
  event.type = TWL_EVENT_TEXT;
  event.timestamp_us = twl_backend_time_microseconds(twl);
  event.codepoint = codepoint;
  twl_internal_push_event(twl, &event);
}

static void twl_win32_pointer_event(
    Twl *twl, TwlEventType type, LPARAM parameter, uint8_t button) {
  TwlWin32 *win32 = (TwlWin32 *) twl->backend;
  TwlEvent event;
  twl_internal_zero(&event, sizeof(event));
  event.type = type;
  event.timestamp_us = twl_backend_time_microseconds(twl);
  event.x = GET_X_LPARAM(parameter);
  event.y = GET_Y_LPARAM(parameter);
  event.dx = event.x - win32->mouse_x;
  event.dy = event.y - win32->mouse_y;
  event.button = button;
  win32->mouse_x = event.x;
  win32->mouse_y = event.y;
  twl_internal_push_event(twl, &event);
}

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

static void twl_win32_controller_button(
    Twl *twl, uint32_t index, WORD buttons, WORD mask,
    TwlControllerButton button) {
  twl_internal_set_controller_button(
    twl, index, button, (buttons & mask) != 0u);
}

static int16_t twl_win32_invert_axis(int16_t value) {
  return value == INT16_MIN ? INT16_MAX : (int16_t) -value;
}

static void twl_win32_pump_controllers(Twl *twl) {
  uint32_t index;
  for (index = 0u; index < twl->config.controller_capacity; ++index) {
    XINPUT_STATE state;
    DWORD result;
    twl_internal_zero(&state, sizeof(state));
    result = index < XUSER_MAX_COUNT
      ? XInputGetState(index, &state) : ERROR_DEVICE_NOT_CONNECTED;
    twl_internal_set_controller_connected(
      twl, index, result == ERROR_SUCCESS);
    if (result != ERROR_SUCCESS) continue;
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_A, TWL_CONTROLLER_BUTTON_SOUTH);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_B, TWL_CONTROLLER_BUTTON_EAST);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_X, TWL_CONTROLLER_BUTTON_WEST);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_Y, TWL_CONTROLLER_BUTTON_NORTH);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_LEFT_SHOULDER, TWL_CONTROLLER_BUTTON_LEFT_SHOULDER);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_RIGHT_SHOULDER, TWL_CONTROLLER_BUTTON_RIGHT_SHOULDER);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_BACK, TWL_CONTROLLER_BUTTON_BACK);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_START, TWL_CONTROLLER_BUTTON_START);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_LEFT_THUMB, TWL_CONTROLLER_BUTTON_LEFT_STICK);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_RIGHT_THUMB, TWL_CONTROLLER_BUTTON_RIGHT_STICK);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_DPAD_UP, TWL_CONTROLLER_BUTTON_DPAD_UP);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_DPAD_DOWN, TWL_CONTROLLER_BUTTON_DPAD_DOWN);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_DPAD_LEFT, TWL_CONTROLLER_BUTTON_DPAD_LEFT);
    twl_win32_controller_button(twl, index, state.Gamepad.wButtons,
      XINPUT_GAMEPAD_DPAD_RIGHT, TWL_CONTROLLER_BUTTON_DPAD_RIGHT);
    twl_internal_set_controller_button(
      twl, index, TWL_CONTROLLER_BUTTON_GUIDE, false);
    twl_internal_set_controller_axis(
      twl, index, TWL_CONTROLLER_AXIS_LEFT_X,
      state.Gamepad.sThumbLX);
    twl_internal_set_controller_axis(
      twl, index, TWL_CONTROLLER_AXIS_LEFT_Y,
      twl_win32_invert_axis(state.Gamepad.sThumbLY));
    twl_internal_set_controller_axis(
      twl, index, TWL_CONTROLLER_AXIS_RIGHT_X,
      state.Gamepad.sThumbRX);
    twl_internal_set_controller_axis(
      twl, index, TWL_CONTROLLER_AXIS_RIGHT_Y,
      twl_win32_invert_axis(state.Gamepad.sThumbRY));
    twl_internal_set_controller_axis(
      twl, index, TWL_CONTROLLER_AXIS_LEFT_TRIGGER,
      (int16_t) ((uint32_t) state.Gamepad.bLeftTrigger * 32767u / 255u));
    twl_internal_set_controller_axis(
      twl, index, TWL_CONTROLLER_AXIS_RIGHT_TRIGGER,
      (int16_t) ((uint32_t) state.Gamepad.bRightTrigger * 32767u / 255u));
  }
}

void twl_backend_pump_events(Twl *twl) {
  MSG message;
  if (!twl) return;
  twl_win32_pump_controllers(twl);
  while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&message);
    DispatchMessageA(&message);
  }
}

TwlResult twl_backend_present(Twl *twl, const TwlSurface *surface) {
  TwlWin32 *win32 = twl ? (TwlWin32 *) twl->backend : NULL;
  TwlWin32Bitmap bitmap;
  size_t expected_stride;
  int result;
  if (!win32 || !win32->device_context || !surface) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  expected_stride = (size_t) surface->width *
    (surface->format == TWL_PIXEL_XRGB8888 ? 4u : 2u);
  expected_stride = (expected_stride + 3u) & ~(size_t) 3u;
  if (surface->stride_bytes != expected_stride)
    return TWL_RESULT_INVALID_ARGUMENT;
  twl_internal_zero(&bitmap, sizeof(bitmap));
  bitmap.header.biSize = sizeof(bitmap.header);
  bitmap.header.biWidth = (LONG) surface->width;
  bitmap.header.biHeight = -(LONG) surface->height;
  bitmap.header.biPlanes = 1;
  bitmap.header.biBitCount =
    surface->format == TWL_PIXEL_XRGB8888 ? 32u : 16u;
  if (surface->format == TWL_PIXEL_XRGB8888) {
    bitmap.header.biCompression = BI_RGB;
  } else {
    bitmap.header.biCompression = BI_BITFIELDS;
    bitmap.masks[0] = 0x001fu;
    bitmap.masks[1] = surface->format == TWL_PIXEL_RGB565
      ? 0x07e0u : 0x03e0u;
    bitmap.masks[2] = surface->format == TWL_PIXEL_RGB565
      ? 0xf800u : 0x7c00u;
  }
  result = StretchDIBits(
    win32->device_context,
    0, 0, (int) twl->display_width, (int) twl->display_height,
    0, 0, (int) surface->width, (int) surface->height,
    surface->pixels, (const BITMAPINFO *) &bitmap,
    DIB_RGB_COLORS, SRCCOPY);
  return result == 0 || result == (int) GDI_ERROR
    ? TWL_RESULT_BACKEND_FAILURE : TWL_RESULT_OK;
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
