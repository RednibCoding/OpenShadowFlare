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

EM_JS(void, twl_web_poll_gamepads, (Twl *twl, int capacity), {
  var pads = navigator.getGamepads ? navigator.getGamepads() : [];
  var buttonMap = [0, 1, 2, 3, 4, 5, -1, -1, 6, 7, 9, 10, 11, 12, 13, 14, 8];
  for (var index = 0; index < capacity; ++index) {
    var pad = pads[index];
    _twl_web_controller_connected(twl, index, pad ? 1 : 0);
    if (!pad) continue;
    for (var nativeButton = 0; nativeButton < buttonMap.length; ++nativeButton) {
      var mapped = buttonMap[nativeButton];
      if (mapped >= 0 && nativeButton < pad.buttons.length)
        _twl_web_controller_button(
          twl, index, mapped, pad.buttons[nativeButton].pressed ? 1 : 0);
    }
    for (var axis = 0; axis < 4; ++axis) {
      var value = axis < pad.axes.length ? pad.axes[axis] : 0;
      _twl_web_controller_axis(twl, index, axis,
        Math.max(-32767, Math.min(32767, Math.round(value * 32767))));
    }
    var leftTrigger = pad.buttons.length > 6 ? pad.buttons[6].value : 0;
    var rightTrigger = pad.buttons.length > 7 ? pad.buttons[7].value : 0;
    _twl_web_controller_axis(twl, index, 4, Math.round(leftTrigger * 32767));
    _twl_web_controller_axis(twl, index, 5, Math.round(rightTrigger * 32767));
  }
})

EMSCRIPTEN_KEEPALIVE
void twl_web_controller_connected(Twl *twl, int index, int connected) {
  if (index >= 0)
    twl_internal_set_controller_connected(
      twl, (uint32_t) index, connected != 0);
}

EMSCRIPTEN_KEEPALIVE
void twl_web_controller_button(Twl *twl, int index, int button, int pressed) {
  if (index >= 0 && button >= 0)
    twl_internal_set_controller_button(
      twl, (uint32_t) index, (TwlControllerButton) button, pressed != 0);
}

EMSCRIPTEN_KEEPALIVE
void twl_web_controller_axis(Twl *twl, int index, int axis, int value) {
  if (index >= 0 && axis >= 0)
    twl_internal_set_controller_axis(
      twl, (uint32_t) index, (TwlControllerAxis) axis, (int16_t) value);
}

static TwlKey twl_web_key(unsigned long key_code) {
  if (key_code >= 48u && key_code <= 57u) {
    return (TwlKey) (TWL_KEY_0 + key_code - 48u);
  }
  if (key_code >= 65u && key_code <= 90u) {
    return (TwlKey) (TWL_KEY_A + key_code - 65u);
  }
  switch (key_code) {
    case 8u: return TWL_KEY_BACKSPACE;
    case 9u: return TWL_KEY_TAB;
    case 13u: return TWL_KEY_RETURN;
    case 27u: return TWL_KEY_ESCAPE;
    case 32u: return TWL_KEY_SPACE;
    case 33u: return TWL_KEY_PAGE_UP;
    case 34u: return TWL_KEY_PAGE_DOWN;
    case 35u: return TWL_KEY_END;
    case 36u: return TWL_KEY_HOME;
    case 37u: return TWL_KEY_LEFT;
    case 38u: return TWL_KEY_UP;
    case 39u: return TWL_KEY_RIGHT;
    case 40u: return TWL_KEY_DOWN;
    case 45u: return TWL_KEY_INSERT;
    case 46u: return TWL_KEY_DELETE;
    case 112u: return TWL_KEY_F1;
    case 113u: return TWL_KEY_F2;
    case 114u: return TWL_KEY_F3;
    case 115u: return TWL_KEY_F4;
    case 116u: return TWL_KEY_F5;
    case 117u: return TWL_KEY_F6;
    case 118u: return TWL_KEY_F7;
    case 119u: return TWL_KEY_F8;
    case 120u: return TWL_KEY_F9;
    case 121u: return TWL_KEY_F10;
    case 122u: return TWL_KEY_F11;
    case 123u: return TWL_KEY_F12;
    default: return TWL_KEY_UNKNOWN;
  }
}

EM_BOOL twl_web_mouse(
    int event_type, const EmscriptenMouseEvent *mouse, void *user_data) {
  TwlWeb *web = (TwlWeb *) user_data;
  TwlEvent event = {0};
  if (!web || !web->twl) {
    return EM_FALSE;
  }
  event.timestamp_us = twl_backend_time_microseconds(web->twl);
  event.x = mouse->targetX;
  event.y = mouse->targetY;
  event.dx = mouse->movementX;
  event.dy = mouse->movementY;
  event.button = (uint8_t) (mouse->button + 1u);
  if (event_type == EMSCRIPTEN_EVENT_MOUSEDOWN) {
    event.type = TWL_EVENT_POINTER_DOWN;
  } else if (event_type == EMSCRIPTEN_EVENT_MOUSEUP) {
    event.type = TWL_EVENT_POINTER_UP;
  } else {
    event.type = TWL_EVENT_POINTER_MOVE;
  }
  twl_internal_push_event(web->twl, &event);
  return EM_TRUE;
}

EM_BOOL twl_web_wheel(
    int event_type, const EmscriptenWheelEvent *wheel, void *user_data) {
  TwlWeb *web = (TwlWeb *) user_data;
  TwlEvent event = {0};
  (void) event_type;
  if (!web || !web->twl) {
    return EM_FALSE;
  }
  event.type = TWL_EVENT_POINTER_WHEEL;
  event.timestamp_us = twl_backend_time_microseconds(web->twl);
  event.x = wheel->mouse.targetX;
  event.y = wheel->mouse.targetY;
  event.dy = wheel->deltaY < 0.0 ? 1 : (wheel->deltaY > 0.0 ? -1 : 0);
  twl_internal_push_event(web->twl, &event);
  return EM_TRUE;
}

EM_BOOL twl_web_keyboard(
    int event_type, const EmscriptenKeyboardEvent *key, void *user_data) {
  TwlWeb *web = (TwlWeb *) user_data;
  TwlEvent event = {0};
  if (!web || !web->twl) {
    return EM_FALSE;
  }
  event.type = event_type == EMSCRIPTEN_EVENT_KEYDOWN
    ? TWL_EVENT_KEY_DOWN : TWL_EVENT_KEY_UP;
  event.timestamp_us = twl_backend_time_microseconds(web->twl);
  event.key = twl_web_key(key->keyCode);
  event.repeat = key->repeat;
  twl_internal_push_event(web->twl, &event);
  if (event_type == EMSCRIPTEN_EVENT_KEYDOWN &&
      key->key[0] >= 0x20 && key->key[0] <= 0x7e && key->key[1] == '\0') {
    event.type = TWL_EVENT_TEXT;
    event.codepoint = (uint8_t) key->key[0];
    twl_internal_push_event(web->twl, &event);
  }
  return EM_TRUE;
}

void twl_backend_pump_events(Twl *twl) {
  if (twl)
    twl_web_poll_gamepads(twl, (int) twl->config.controller_capacity);
}
