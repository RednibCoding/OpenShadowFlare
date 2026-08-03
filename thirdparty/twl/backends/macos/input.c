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

static uint32_t twl_macos_utf8_codepoint(const char *text) {
  const uint8_t *bytes = (const uint8_t *) text;
  if (!bytes || bytes[0] == 0u) return 0u;
  if (bytes[0] < 0x80u) return bytes[0];
  if ((bytes[0] & 0xe0u) == 0xc0u)
    return ((uint32_t) (bytes[0] & 0x1fu) << 6u) |
      (uint32_t) (bytes[1] & 0x3fu);
  if ((bytes[0] & 0xf0u) == 0xe0u)
    return ((uint32_t) (bytes[0] & 0x0fu) << 12u) |
      ((uint32_t) (bytes[1] & 0x3fu) << 6u) |
      (uint32_t) (bytes[2] & 0x3fu);
  if ((bytes[0] & 0xf8u) == 0xf0u)
    return ((uint32_t) (bytes[0] & 0x07u) << 18u) |
      ((uint32_t) (bytes[1] & 0x3fu) << 12u) |
      ((uint32_t) (bytes[2] & 0x3fu) << 6u) |
      (uint32_t) (bytes[3] & 0x3fu);
  return 0u;
}

static TwlKey twl_macos_key(id event) {
  const unsigned short code = ((unsigned short (*)(id, SEL)) objc_msgSend)(
    event, sel_registerName("keyCode"));
  const char *characters;
  switch (code) {
    case 51u: return TWL_KEY_BACKSPACE;
    case 48u: return TWL_KEY_TAB;
    case 36u: return TWL_KEY_RETURN;
    case 53u: return TWL_KEY_ESCAPE;
    case 49u: return TWL_KEY_SPACE;
    case 117u: return TWL_KEY_DELETE;
    case 123u: return TWL_KEY_LEFT;
    case 124u: return TWL_KEY_RIGHT;
    case 126u: return TWL_KEY_UP;
    case 125u: return TWL_KEY_DOWN;
    case 115u: return TWL_KEY_HOME;
    case 119u: return TWL_KEY_END;
    case 116u: return TWL_KEY_PAGE_UP;
    case 121u: return TWL_KEY_PAGE_DOWN;
    case 114u: return TWL_KEY_INSERT;
    case 122u: return TWL_KEY_F1;
    case 120u: return TWL_KEY_F2;
    case 99u: return TWL_KEY_F3;
    case 118u: return TWL_KEY_F4;
    case 96u: return TWL_KEY_F5;
    case 97u: return TWL_KEY_F6;
    case 98u: return TWL_KEY_F7;
    case 100u: return TWL_KEY_F8;
    case 101u: return TWL_KEY_F9;
    case 109u: return TWL_KEY_F10;
    case 103u: return TWL_KEY_F11;
    case 111u: return TWL_KEY_F12;
    default: break;
  }
  characters = twl_ns_utf8(
    twl_msg_id(event, "charactersIgnoringModifiers"));
  if (characters[0] >= 'a' && characters[0] <= 'z')
    return (TwlKey) (TWL_KEY_A + characters[0] - 'a');
  if (characters[0] >= 'A' && characters[0] <= 'Z')
    return (TwlKey) (TWL_KEY_A + characters[0] - 'A');
  if (characters[0] >= '0' && characters[0] <= '9')
    return (TwlKey) (TWL_KEY_0 + characters[0] - '0');
  return TWL_KEY_UNKNOWN;
}

static uint8_t twl_macos_mouse_button(TwlNSInteger native_button) {
  if (native_button == 0) return 1u;
  if (native_button == 1) return 3u;
  if (native_button == 2) return 2u;
  return native_button < 255 ? (uint8_t) (native_button + 1) : 0u;
}

static void twl_macos_push_event(TwlMacos *macos, id native_event) {
  const TwlNSInteger type = twl_msg_integer(native_event, "type");
  TwlEvent event;
  TwlPoint position;
  twl_internal_zero(&event, sizeof(event));
  event.timestamp_us = twl_backend_time_microseconds(macos->twl);
  if (type == TWL_NS_EVENT_KEY_DOWN || type == TWL_NS_EVENT_KEY_UP) {
    event.type = type == TWL_NS_EVENT_KEY_DOWN
      ? TWL_EVENT_KEY_DOWN : TWL_EVENT_KEY_UP;
    event.key = twl_macos_key(native_event);
    event.repeat = twl_msg_bool(native_event, "isARepeat");
    twl_internal_push_event(macos->twl, &event);
    if (type == TWL_NS_EVENT_KEY_DOWN) {
      const uint32_t codepoint = twl_macos_utf8_codepoint(
        twl_ns_utf8(twl_msg_id(native_event, "characters")));
      if (codepoint >= 0x20u && codepoint != 0x7fu) {
        event.type = TWL_EVENT_TEXT;
        event.codepoint = codepoint;
        twl_internal_push_event(macos->twl, &event);
      }
    }
    return;
  }
  position = twl_msg_point(native_event, "locationInWindow");
  event.x = (int32_t) position.x;
  event.y = (int32_t) macos->twl->display_height - (int32_t) position.y;
  event.dx = event.x - macos->mouse_x;
  event.dy = event.y - macos->mouse_y;
  macos->mouse_x = event.x;
  macos->mouse_y = event.y;
  switch (type) {
    case TWL_NS_EVENT_LEFT_DOWN:
    case TWL_NS_EVENT_RIGHT_DOWN:
    case TWL_NS_EVENT_OTHER_DOWN:
      event.type = TWL_EVENT_POINTER_DOWN;
      event.button = twl_macos_mouse_button(
        twl_msg_integer(native_event, "buttonNumber"));
      break;
    case TWL_NS_EVENT_LEFT_UP:
    case TWL_NS_EVENT_RIGHT_UP:
    case TWL_NS_EVENT_OTHER_UP:
      event.type = TWL_EVENT_POINTER_UP;
      event.button = twl_macos_mouse_button(
        twl_msg_integer(native_event, "buttonNumber"));
      break;
    case TWL_NS_EVENT_MOUSE_MOVED:
    case TWL_NS_EVENT_LEFT_DRAGGED:
    case TWL_NS_EVENT_RIGHT_DRAGGED:
    case TWL_NS_EVENT_OTHER_DRAGGED:
      event.type = TWL_EVENT_POINTER_MOVE;
      break;
    case TWL_NS_EVENT_SCROLL:
      event.type = TWL_EVENT_POINTER_WHEEL;
      event.dx = (int32_t) twl_msg_double(native_event, "scrollingDeltaX");
      event.dy = (int32_t) twl_msg_double(native_event, "scrollingDeltaY");
      break;
    default:
      return;
  }
  twl_internal_push_event(macos->twl, &event);
}

static int16_t twl_macos_axis(float value) {
  if (value > 1.0f) value = 1.0f;
  if (value < -1.0f) value = -1.0f;
  return (int16_t) (value * 32767.0f);
}

static void twl_macos_controller_button(
    Twl *twl, uint32_t index, id gamepad, const char *selector,
    TwlControllerButton button) {
  id input = twl_optional_id(gamepad, selector);
  twl_internal_set_controller_button(
    twl, index, button, twl_msg_bool(input, "isPressed"));
}

static bool twl_macos_controller_in_array(id controllers, id controller) {
  TwlNSUInteger index;
  const TwlNSUInteger count = twl_msg_count(controllers);
  for (index = 0u; index < count; ++index) {
    if (twl_msg_object_at(controllers, index) == controller) return true;
  }
  return false;
}

static bool twl_macos_controller_assigned(
    const TwlMacos *macos, id controller) {
  uint32_t index;
  for (index = 0u; index < macos->controller_count; ++index) {
    if (macos->controllers[index] == controller) return true;
  }
  return false;
}

static void twl_macos_assign_controllers(
    TwlMacos *macos, id controllers) {
  const TwlNSUInteger connected_count = twl_msg_count(controllers);
  uint32_t slot;
  TwlNSUInteger connected_index;
  for (slot = 0u; slot < macos->controller_count; ++slot) {
    if (macos->controllers[slot] &&
        !twl_macos_controller_in_array(
          controllers, macos->controllers[slot])) {
      macos->controllers[slot] = nil;
    }
  }
  for (connected_index = 0u;
       connected_index < connected_count; ++connected_index) {
    id controller = twl_msg_object_at(controllers, connected_index);
    if (twl_macos_controller_assigned(macos, controller)) continue;
    for (slot = 0u; slot < macos->controller_count; ++slot) {
      if (!macos->controllers[slot]) {
        macos->controllers[slot] = controller;
        break;
      }
    }
  }
}

static void twl_macos_pump_controllers(Twl *twl, TwlMacos *macos) {
  id controllers = twl_msg_id((id) objc_getClass("GCController"), "controllers");
  uint32_t index;
  twl_macos_assign_controllers(macos, controllers);
  for (index = 0u; index < twl->config.controller_capacity; ++index) {
    id controller = macos->controllers[index];
    id gamepad = twl_optional_id(controller, "extendedGamepad");
    id stick;
    id axis;
    twl_internal_set_controller_connected(twl, index, gamepad != nil);
    if (!gamepad) continue;
    twl_macos_controller_button(twl, index, gamepad, "buttonA",
      TWL_CONTROLLER_BUTTON_SOUTH);
    twl_macos_controller_button(twl, index, gamepad, "buttonB",
      TWL_CONTROLLER_BUTTON_EAST);
    twl_macos_controller_button(twl, index, gamepad, "buttonX",
      TWL_CONTROLLER_BUTTON_WEST);
    twl_macos_controller_button(twl, index, gamepad, "buttonY",
      TWL_CONTROLLER_BUTTON_NORTH);
    twl_macos_controller_button(twl, index, gamepad, "leftShoulder",
      TWL_CONTROLLER_BUTTON_LEFT_SHOULDER);
    twl_macos_controller_button(twl, index, gamepad, "rightShoulder",
      TWL_CONTROLLER_BUTTON_RIGHT_SHOULDER);
    twl_macos_controller_button(twl, index, gamepad, "buttonOptions",
      TWL_CONTROLLER_BUTTON_BACK);
    twl_macos_controller_button(twl, index, gamepad, "buttonMenu",
      TWL_CONTROLLER_BUTTON_START);
    twl_macos_controller_button(twl, index, gamepad, "buttonHome",
      TWL_CONTROLLER_BUTTON_GUIDE);
    twl_macos_controller_button(twl, index, gamepad, "leftThumbstickButton",
      TWL_CONTROLLER_BUTTON_LEFT_STICK);
    twl_macos_controller_button(twl, index, gamepad, "rightThumbstickButton",
      TWL_CONTROLLER_BUTTON_RIGHT_STICK);
    stick = twl_optional_id(gamepad, "dpad");
    axis = twl_optional_id(stick, "xAxis");
    twl_internal_set_controller_button(twl, index,
      TWL_CONTROLLER_BUTTON_DPAD_LEFT, twl_msg_float(axis, "value") < -0.5f);
    twl_internal_set_controller_button(twl, index,
      TWL_CONTROLLER_BUTTON_DPAD_RIGHT, twl_msg_float(axis, "value") > 0.5f);
    axis = twl_optional_id(stick, "yAxis");
    twl_internal_set_controller_button(twl, index,
      TWL_CONTROLLER_BUTTON_DPAD_UP, twl_msg_float(axis, "value") > 0.5f);
    twl_internal_set_controller_button(twl, index,
      TWL_CONTROLLER_BUTTON_DPAD_DOWN, twl_msg_float(axis, "value") < -0.5f);
    stick = twl_optional_id(gamepad, "leftThumbstick");
    axis = twl_optional_id(stick, "xAxis");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_LEFT_X, twl_macos_axis(twl_msg_float(axis, "value")));
    axis = twl_optional_id(stick, "yAxis");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_LEFT_Y, twl_macos_axis(-twl_msg_float(axis, "value")));
    stick = twl_optional_id(gamepad, "rightThumbstick");
    axis = twl_optional_id(stick, "xAxis");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_RIGHT_X, twl_macos_axis(twl_msg_float(axis, "value")));
    axis = twl_optional_id(stick, "yAxis");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_RIGHT_Y, twl_macos_axis(-twl_msg_float(axis, "value")));
    axis = twl_optional_id(gamepad, "leftTrigger");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_LEFT_TRIGGER,
      twl_macos_axis(twl_msg_float(axis, "value")));
    axis = twl_optional_id(gamepad, "rightTrigger");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_RIGHT_TRIGGER,
      twl_macos_axis(twl_msg_float(axis, "value")));
  }
}

void twl_backend_pump_events(Twl *twl) {
  TwlMacos *macos = twl ? (TwlMacos *) twl->backend : NULL;
  void *pool;
  id application;
  id distant_past;
  id mode;
  id native_event;
  TwlRect bounds;
  uint32_t width;
  uint32_t height;
  if (!macos || !macos->window) return;
  pool = objc_autoreleasePoolPush();
  twl_macos_pump_controllers(twl, macos);
  application = twl_msg_id(
    (id) objc_getClass("NSApplication"), "sharedApplication");
  distant_past = twl_msg_id((id) objc_getClass("NSDate"), "distantPast");
  mode = macos->run_loop_mode;
  for (;;) {
    native_event = ((id (*)(
      id, SEL, TwlNSUInteger, id, id, BOOL)) objc_msgSend)(
        application,
        sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"),
        TWL_NS_EVENT_MASK_ANY, distant_past, mode, YES);
    if (!native_event) break;
    if (twl_msg_id(native_event, "window") == macos->window)
      twl_macos_push_event(macos, native_event);
    twl_msg_void_id(application, "sendEvent:", native_event);
  }
  bounds = twl_msg_rect(macos->view, "bounds");
  width = bounds.size.width > 0.0 ? (uint32_t) bounds.size.width : 1u;
  height = bounds.size.height > 0.0 ? (uint32_t) bounds.size.height : 1u;
  if (width != twl->display_width || height != twl->display_height) {
    TwlEvent event;
    twl_internal_set_display_size(twl, width, height);
    twl_msg_void(macos->context, "update");
    macos->backing_scale = twl_msg_double(macos->window, "backingScaleFactor");
    if (macos->backing_scale <= 0.0) macos->backing_scale = 1.0;
    twl_internal_zero(&event, sizeof(event));
    event.type = TWL_EVENT_RESIZED;
    event.timestamp_us = twl_backend_time_microseconds(twl);
    event.width = (int32_t) width;
    event.height = (int32_t) height;
    twl_internal_push_event(twl, &event);
  }
  objc_autoreleasePoolPop(pool);
}
