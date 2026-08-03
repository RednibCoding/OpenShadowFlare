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

#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <errno.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <unistd.h>

void twl_x11_input_init(TwlX11 *x11) {
  uint32_t index;
  for (index = 0u; index < x11->controller_count; ++index)
    x11->controllers[index].descriptor = -1;
}

void twl_x11_input_shutdown(TwlX11 *x11) {
  uint32_t index;
  for (index = 0u; index < x11->controller_count; ++index) {
    if (x11->controllers[index].descriptor >= 0) {
      close(x11->controllers[index].descriptor);
      x11->controllers[index].descriptor = -1;
    }
  }
}

static TwlKey twl_x11_key(KeySym symbol) {
  if (symbol >= XK_a && symbol <= XK_z)
    return (TwlKey) (TWL_KEY_A + (symbol - XK_a));
  if (symbol >= XK_A && symbol <= XK_Z)
    return (TwlKey) (TWL_KEY_A + (symbol - XK_A));
  if (symbol >= XK_0 && symbol <= XK_9)
    return (TwlKey) (TWL_KEY_0 + (symbol - XK_0));
  switch (symbol) {
    case XK_BackSpace: return TWL_KEY_BACKSPACE;
    case XK_Tab: return TWL_KEY_TAB;
    case XK_Return: return TWL_KEY_RETURN;
    case XK_Escape: return TWL_KEY_ESCAPE;
    case XK_space: return TWL_KEY_SPACE;
    case XK_Delete: return TWL_KEY_DELETE;
    case XK_Left: return TWL_KEY_LEFT;
    case XK_Right: return TWL_KEY_RIGHT;
    case XK_Up: return TWL_KEY_UP;
    case XK_Down: return TWL_KEY_DOWN;
    case XK_Home: return TWL_KEY_HOME;
    case XK_End: return TWL_KEY_END;
    case XK_Page_Up: return TWL_KEY_PAGE_UP;
    case XK_Page_Down: return TWL_KEY_PAGE_DOWN;
    case XK_Insert: return TWL_KEY_INSERT;
    case XK_F1: return TWL_KEY_F1;
    case XK_F2: return TWL_KEY_F2;
    case XK_F3: return TWL_KEY_F3;
    case XK_F4: return TWL_KEY_F4;
    case XK_F5: return TWL_KEY_F5;
    case XK_F6: return TWL_KEY_F6;
    case XK_F7: return TWL_KEY_F7;
    case XK_F8: return TWL_KEY_F8;
    case XK_F9: return TWL_KEY_F9;
    case XK_F10: return TWL_KEY_F10;
    case XK_F11: return TWL_KEY_F11;
    case XK_F12: return TWL_KEY_F12;
    default: return TWL_KEY_UNKNOWN;
  }
}

static void twl_x11_push_key(Twl *twl, XKeyEvent *key, bool pressed) {
  TwlEvent event = {0};
  char text[8];
  KeySym symbol = NoSymbol;
  const int count = XLookupString(key, text, (int) sizeof(text), &symbol, NULL);
  event.type = pressed ? TWL_EVENT_KEY_DOWN : TWL_EVENT_KEY_UP;
  event.timestamp_us = twl_backend_time_microseconds(twl);
  event.key = twl_x11_key(symbol);
  twl_internal_push_event(twl, &event);
  if (pressed && count == 1 && (unsigned char) text[0] >= 0x20u) {
    event.type = TWL_EVENT_TEXT;
    event.codepoint = (unsigned char) text[0];
    twl_internal_push_event(twl, &event);
  }
}

static bool twl_x11_controller_path(
    char *path, size_t path_size, uint32_t index) {
  static const char prefix[] = "/dev/input/js";
  char digits[10];
  size_t prefix_length = sizeof(prefix) - 1u;
  size_t digit_count = 0u;
  size_t position;
  if (!path || path_size <= prefix_length + 1u) return false;
  do {
    digits[digit_count++] = (char) ('0' + index % 10u);
    index /= 10u;
  } while (index > 0u && digit_count < sizeof(digits));
  if (prefix_length + digit_count + 1u > path_size) return false;
  for (position = 0u; position < prefix_length; ++position)
    path[position] = prefix[position];
  for (position = 0u; position < digit_count; ++position)
    path[prefix_length + position] = digits[digit_count - position - 1u];
  path[prefix_length + digit_count] = '\0';
  return true;
}

static void twl_x11_controller_button(
    Twl *twl, uint32_t index, uint8_t native_button, bool pressed) {
  static const TwlControllerButton buttons[] = {
    TWL_CONTROLLER_BUTTON_SOUTH, TWL_CONTROLLER_BUTTON_EAST,
    TWL_CONTROLLER_BUTTON_WEST, TWL_CONTROLLER_BUTTON_NORTH,
    TWL_CONTROLLER_BUTTON_LEFT_SHOULDER,
    TWL_CONTROLLER_BUTTON_RIGHT_SHOULDER,
    TWL_CONTROLLER_BUTTON_BACK, TWL_CONTROLLER_BUTTON_START,
    TWL_CONTROLLER_BUTTON_GUIDE, TWL_CONTROLLER_BUTTON_LEFT_STICK,
    TWL_CONTROLLER_BUTTON_RIGHT_STICK
  };
  if (native_button < sizeof(buttons) / sizeof(buttons[0]))
    twl_internal_set_controller_button(
      twl, index, buttons[native_button], pressed);
}

static void twl_x11_controller_axis(
    Twl *twl, uint32_t index, uint8_t native_axis, int16_t value) {
  switch (native_axis) {
    case 0u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_LEFT_X, value);
      break;
    case 1u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_LEFT_Y, value);
      break;
    case 2u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_LEFT_TRIGGER,
        (int16_t) (((int32_t) value + 32767) / 2));
      break;
    case 3u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_RIGHT_X, value);
      break;
    case 4u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_RIGHT_Y, value);
      break;
    case 5u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_RIGHT_TRIGGER,
        (int16_t) (((int32_t) value + 32767) / 2));
      break;
    case 6u:
      twl_internal_set_controller_button(
        twl, index, TWL_CONTROLLER_BUTTON_DPAD_LEFT, value < -16384);
      twl_internal_set_controller_button(
        twl, index, TWL_CONTROLLER_BUTTON_DPAD_RIGHT, value > 16384);
      break;
    case 7u:
      twl_internal_set_controller_button(
        twl, index, TWL_CONTROLLER_BUTTON_DPAD_UP, value < -16384);
      twl_internal_set_controller_button(
        twl, index, TWL_CONTROLLER_BUTTON_DPAD_DOWN, value > 16384);
      break;
    default:
      break;
  }
}

static void twl_x11_pump_controllers(Twl *twl, TwlX11 *x11) {
  const uint64_t now = twl_backend_time_microseconds(twl);
  uint32_t index;
  for (index = 0u; index < x11->controller_count; ++index) {
    TwlX11Controller *controller = &x11->controllers[index];
    if (controller->descriptor < 0 && now >= controller->next_open_attempt_us) {
      char path[32];
      if (twl_x11_controller_path(path, sizeof(path), index))
        controller->descriptor = open(path, O_RDONLY | O_NONBLOCK);
      controller->next_open_attempt_us = now + UINT64_C(1000000);
      if (controller->descriptor >= 0)
        twl_internal_set_controller_connected(twl, index, true);
    }
    if (controller->descriptor >= 0) {
      struct js_event native_event;
      ssize_t read_size;
      while ((read_size = read(
                controller->descriptor, &native_event,
                sizeof(native_event))) == (ssize_t) sizeof(native_event)) {
        const uint8_t type = native_event.type & (uint8_t) ~JS_EVENT_INIT;
        if (type == JS_EVENT_BUTTON)
          twl_x11_controller_button(
            twl, index, native_event.number, native_event.value != 0);
        else if (type == JS_EVENT_AXIS)
          twl_x11_controller_axis(
            twl, index, native_event.number, native_event.value);
      }
      if (read_size < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        close(controller->descriptor);
        controller->descriptor = -1;
        controller->next_open_attempt_us = now + UINT64_C(1000000);
        twl_internal_set_controller_connected(twl, index, false);
      }
    }
  }
}

void twl_backend_pump_events(Twl *twl) {
  TwlX11 *x11 = twl ? (TwlX11 *) twl->backend : NULL;
  if (!x11 || !x11->display) return;
  twl_x11_pump_controllers(twl, x11);
  while (XPending(x11->display) > 0) {
    XEvent native_event;
    TwlEvent event = {0};
    XNextEvent(x11->display, &native_event);
    event.timestamp_us = twl_backend_time_microseconds(twl);
    switch (native_event.type) {
      case ClientMessage:
        if ((Atom) native_event.xclient.data.l[0] == x11->wm_delete) {
          event.type = TWL_EVENT_QUIT;
          twl_internal_push_event(twl, &event);
        }
        break;
      case ConfigureNotify:
        event.type = TWL_EVENT_RESIZED;
        event.width = native_event.xconfigure.width;
        event.height = native_event.xconfigure.height;
        twl_internal_set_display_size(
          twl, (uint32_t) event.width, (uint32_t) event.height);
        twl_internal_push_event(twl, &event);
        break;
      case KeyPress:
        twl_x11_push_key(twl, &native_event.xkey, true);
        break;
      case KeyRelease:
        twl_x11_push_key(twl, &native_event.xkey, false);
        break;
      case ButtonPress:
      case ButtonRelease:
        event.type = native_event.type == ButtonPress
          ? TWL_EVENT_POINTER_DOWN : TWL_EVENT_POINTER_UP;
        event.x = native_event.xbutton.x;
        event.y = native_event.xbutton.y;
        event.button = (uint8_t) native_event.xbutton.button;
        if (native_event.xbutton.button == Button4 ||
            native_event.xbutton.button == Button5) {
          if (native_event.type == ButtonPress) {
            event.type = TWL_EVENT_POINTER_WHEEL;
            event.dy = native_event.xbutton.button == Button4 ? 1 : -1;
            twl_internal_push_event(twl, &event);
          }
        } else {
          twl_internal_push_event(twl, &event);
        }
        break;
      case MotionNotify:
        event.type = TWL_EVENT_POINTER_MOVE;
        event.x = native_event.xmotion.x;
        event.y = native_event.xmotion.y;
        twl_internal_push_event(twl, &event);
        break;
      default:
        break;
    }
  }
}
