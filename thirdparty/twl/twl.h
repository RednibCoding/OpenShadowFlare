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

#ifndef TWL_H
#define TWL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Twl Twl;

typedef enum {
  TWL_RESULT_OK = 0,
  TWL_RESULT_INVALID_ARGUMENT,
  TWL_RESULT_MISALIGNED_MEMORY,
  TWL_RESULT_INSUFFICIENT_MEMORY,
  TWL_RESULT_BACKEND_UNAVAILABLE,
  TWL_RESULT_BACKEND_FAILURE
} TwlResult;

typedef enum {
  TWL_DISPLAY_HEADLESS = 0,
  TWL_DISPLAY_WINDOW
} TwlDisplayMode;

typedef enum {
  TWL_PIXEL_RGB555 = 0,
  TWL_PIXEL_RGB565,
  TWL_PIXEL_XRGB8888
} TwlPixelFormat;

typedef enum {
  TWL_EVENT_NONE = 0,
  TWL_EVENT_QUIT,
  TWL_EVENT_RESIZED,
  TWL_EVENT_KEY_DOWN,
  TWL_EVENT_KEY_UP,
  TWL_EVENT_POINTER_DOWN,
  TWL_EVENT_POINTER_UP,
  TWL_EVENT_POINTER_MOVE,
  TWL_EVENT_POINTER_WHEEL,
  TWL_EVENT_TEXT,
  TWL_EVENT_CONTROLLER_CONNECTED,
  TWL_EVENT_CONTROLLER_DISCONNECTED,
  TWL_EVENT_CONTROLLER_BUTTON_DOWN,
  TWL_EVENT_CONTROLLER_BUTTON_UP,
  TWL_EVENT_CONTROLLER_AXIS
} TwlEventType;

typedef enum {
  TWL_CONTROLLER_BUTTON_SOUTH = 0,
  TWL_CONTROLLER_BUTTON_EAST,
  TWL_CONTROLLER_BUTTON_WEST,
  TWL_CONTROLLER_BUTTON_NORTH,
  TWL_CONTROLLER_BUTTON_LEFT_SHOULDER,
  TWL_CONTROLLER_BUTTON_RIGHT_SHOULDER,
  TWL_CONTROLLER_BUTTON_BACK,
  TWL_CONTROLLER_BUTTON_START,
  TWL_CONTROLLER_BUTTON_GUIDE,
  TWL_CONTROLLER_BUTTON_LEFT_STICK,
  TWL_CONTROLLER_BUTTON_RIGHT_STICK,
  TWL_CONTROLLER_BUTTON_DPAD_UP,
  TWL_CONTROLLER_BUTTON_DPAD_DOWN,
  TWL_CONTROLLER_BUTTON_DPAD_LEFT,
  TWL_CONTROLLER_BUTTON_DPAD_RIGHT,
  TWL_CONTROLLER_BUTTON_COUNT
} TwlControllerButton;

typedef enum {
  TWL_CONTROLLER_AXIS_LEFT_X = 0,
  TWL_CONTROLLER_AXIS_LEFT_Y,
  TWL_CONTROLLER_AXIS_RIGHT_X,
  TWL_CONTROLLER_AXIS_RIGHT_Y,
  TWL_CONTROLLER_AXIS_LEFT_TRIGGER,
  TWL_CONTROLLER_AXIS_RIGHT_TRIGGER,
  TWL_CONTROLLER_AXIS_COUNT
} TwlControllerAxis;

typedef enum {
  TWL_KEY_UNKNOWN = 0,
  TWL_KEY_BACKSPACE = 8,
  TWL_KEY_TAB = 9,
  TWL_KEY_RETURN = 13,
  TWL_KEY_ESCAPE = 27,
  TWL_KEY_SPACE = 32,
  TWL_KEY_0 = 48,
  TWL_KEY_1,
  TWL_KEY_2,
  TWL_KEY_3,
  TWL_KEY_4,
  TWL_KEY_5,
  TWL_KEY_6,
  TWL_KEY_7,
  TWL_KEY_8,
  TWL_KEY_9,
  TWL_KEY_A = 65,
  TWL_KEY_B,
  TWL_KEY_C,
  TWL_KEY_D,
  TWL_KEY_E,
  TWL_KEY_F,
  TWL_KEY_G,
  TWL_KEY_H,
  TWL_KEY_I,
  TWL_KEY_J,
  TWL_KEY_K,
  TWL_KEY_L,
  TWL_KEY_M,
  TWL_KEY_N,
  TWL_KEY_O,
  TWL_KEY_P,
  TWL_KEY_Q,
  TWL_KEY_R,
  TWL_KEY_S,
  TWL_KEY_T,
  TWL_KEY_U,
  TWL_KEY_V,
  TWL_KEY_W,
  TWL_KEY_X,
  TWL_KEY_Y,
  TWL_KEY_Z,
  TWL_KEY_DELETE = 127,
  TWL_KEY_LEFT = 256,
  TWL_KEY_RIGHT,
  TWL_KEY_UP,
  TWL_KEY_DOWN,
  TWL_KEY_HOME,
  TWL_KEY_END,
  TWL_KEY_PAGE_UP,
  TWL_KEY_PAGE_DOWN,
  TWL_KEY_INSERT,
  TWL_KEY_F1,
  TWL_KEY_F2,
  TWL_KEY_F3,
  TWL_KEY_F4,
  TWL_KEY_F5,
  TWL_KEY_F6,
  TWL_KEY_F7,
  TWL_KEY_F8,
  TWL_KEY_F9,
  TWL_KEY_F10,
  TWL_KEY_F11,
  TWL_KEY_F12
} TwlKey;

typedef struct {
  TwlEventType type;
  uint64_t timestamp_us;
  int32_t x;
  int32_t y;
  int32_t dx;
  int32_t dy;
  int32_t width;
  int32_t height;
  uint32_t codepoint;
  TwlKey key;
  int16_t axis_value;
  uint8_t controller_index;
  TwlControllerButton controller_button;
  TwlControllerAxis controller_axis;
  uint8_t button;
  bool repeat;
} TwlEvent;

typedef struct {
  uint32_t buttons;
  int16_t axes[TWL_CONTROLLER_AXIS_COUNT];
  bool connected;
} TwlControllerState;

typedef struct {
  const void *pixels;
  uint32_t width;
  uint32_t height;
  size_t stride_bytes;
  TwlPixelFormat format;
} TwlSurface;

typedef struct {
  TwlDisplayMode display_mode;
  const char *title;
  const char *display_target;
  uint32_t width;
  uint32_t height;
  uint32_t event_capacity;
  uint32_t controller_capacity;
  bool resizable;
} TwlConfig;

TwlConfig twl_config_default(void);
size_t twl_memory_alignment(void);
size_t twl_memory_required(const TwlConfig *config);

TwlResult twl_init(
  void *memory, size_t memory_size, const TwlConfig *config, Twl **out_twl);
void twl_shutdown(Twl *twl);

void twl_pump_events(Twl *twl);
bool twl_poll_event(Twl *twl, TwlEvent *event);
bool twl_controller_state(
  const Twl *twl, uint32_t controller_index, TwlControllerState *state);
TwlResult twl_present(Twl *twl, const TwlSurface *surface);
void twl_get_display_size(
  const Twl *twl, uint32_t *width, uint32_t *height);
uint64_t twl_time_microseconds(const Twl *twl);

#ifdef __cplusplus
}
#endif

#endif
