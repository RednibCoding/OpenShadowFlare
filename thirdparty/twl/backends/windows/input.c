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
#include <xinput.h>

TwlKey twl_win32_key(WPARAM key) {
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

void twl_win32_push_text(Twl *twl, uint16_t unit) {
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

void twl_win32_pointer_event(
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
