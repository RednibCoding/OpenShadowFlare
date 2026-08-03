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

#include "twl_internal.h"

#include <stddef.h>
#include <stdint.h>

typedef union {
  void *pointer;
  uint64_t integer;
  long double floating_point;
} TwlNaturalAlignment;

static size_t twl_pixel_size(TwlPixelFormat format) {
  return format == TWL_PIXEL_XRGB8888 ? 4u : 2u;
}

size_t twl_internal_align_up(size_t value, size_t alignment) {
  const size_t remainder = value % alignment;
  return remainder == 0u ? value : value + alignment - remainder;
}

void twl_internal_zero(void *memory, size_t size) {
  uint8_t *bytes = (uint8_t *) memory;
  size_t index;
  for (index = 0u; index < size; ++index) {
    bytes[index] = 0u;
  }
}

static bool twl_config_valid(const TwlConfig *config) {
  if (!config || config->event_capacity == 0u ||
      config->controller_capacity > 255u) {
    return false;
  }
  if (config->display_mode == TWL_DISPLAY_HEADLESS) {
    return true;
  }
  return config->display_mode == TWL_DISPLAY_WINDOW &&
         config->width > 0u && config->height > 0u &&
         config->width <= (uint32_t) INT32_MAX &&
         config->height <= (uint32_t) INT32_MAX;
}

TwlConfig twl_config_default(void) {
  TwlConfig config;
  config.display_mode = TWL_DISPLAY_WINDOW;
  config.title = "TWL application";
  config.display_target = "#canvas";
  config.width = 640u;
  config.height = 480u;
  config.event_capacity = 64u;
  config.controller_capacity = 4u;
  config.resizable = false;
  return config;
}

size_t twl_memory_alignment(void) {
  const size_t common_alignment = _Alignof(TwlNaturalAlignment);
  const size_t backend_alignment = twl_backend_memory_alignment();
  return backend_alignment > common_alignment
           ? backend_alignment
           : common_alignment;
}

size_t twl_memory_required(const TwlConfig *config) {
  size_t size;
  size_t backend_alignment;
  if (!twl_config_valid(config)) {
    return 0u;
  }
  size = twl_internal_align_up(sizeof(Twl), _Alignof(TwlEvent));
  if (config->event_capacity > (SIZE_MAX - size) / sizeof(TwlEvent)) {
    return 0u;
  }
  size += (size_t) config->event_capacity * sizeof(TwlEvent);
  size = twl_internal_align_up(size, _Alignof(TwlControllerState));
  if (config->controller_capacity >
      (SIZE_MAX - size) / sizeof(TwlControllerState)) {
    return 0u;
  }
  size += (size_t) config->controller_capacity * sizeof(TwlControllerState);
  if (config->display_mode == TWL_DISPLAY_HEADLESS) {
    return size;
  }
  backend_alignment = twl_backend_memory_alignment();
  size = twl_internal_align_up(size, backend_alignment);
  if (twl_backend_memory_required(config) > SIZE_MAX - size) {
    return 0u;
  }
  return size + twl_backend_memory_required(config);
}

TwlResult twl_init(
    void *memory, size_t memory_size, const TwlConfig *config, Twl **out_twl) {
  uint8_t *bytes;
  size_t required;
  size_t offset;
  Twl *twl;
  TwlResult result;

  if (out_twl) {
    *out_twl = NULL;
  }
  required = twl_memory_required(config);
  if (!memory || !out_twl || required == 0u) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if ((uintptr_t) memory % twl_memory_alignment() != 0u) {
    return TWL_RESULT_MISALIGNED_MEMORY;
  }
  if (memory_size < required) {
    return TWL_RESULT_INSUFFICIENT_MEMORY;
  }

  twl_internal_zero(memory, required);
  bytes = (uint8_t *) memory;
  twl = (Twl *) memory;
  twl->config = *config;
  twl->display_width = config->width;
  twl->display_height = config->height;
  offset = twl_internal_align_up(sizeof(Twl), _Alignof(TwlEvent));
  twl->events = (TwlEvent *) (bytes + offset);
  offset += (size_t) config->event_capacity * sizeof(TwlEvent);
  offset = twl_internal_align_up(offset, _Alignof(TwlControllerState));
  twl->controllers = (TwlControllerState *) (bytes + offset);
  offset +=
    (size_t) config->controller_capacity * sizeof(TwlControllerState);

  if (config->display_mode == TWL_DISPLAY_WINDOW) {
    offset = twl_internal_align_up(offset, twl_backend_memory_alignment());
    twl->backend = bytes + offset;
    twl->backend_size = required - offset;
    result = twl_backend_init(
      twl, twl->backend, twl->backend_size, config);
    if (result != TWL_RESULT_OK) {
      return result;
    }
    twl->backend_ready = true;
  }

  *out_twl = twl;
  return TWL_RESULT_OK;
}

void twl_shutdown(Twl *twl) {
  if (!twl) {
    return;
  }
  if (twl->backend_ready) {
    twl_backend_shutdown(twl);
    twl->backend_ready = false;
  }
}

void twl_internal_push_event(Twl *twl, const TwlEvent *event) {
  if (!twl || !event || twl->config.event_capacity == 0u) {
    return;
  }
  if (twl->event_count == twl->config.event_capacity) {
    twl->event_head =
      (twl->event_head + 1u) % twl->config.event_capacity;
    --twl->event_count;
  }
  twl->events[twl->event_tail] = *event;
  twl->event_tail =
    (twl->event_tail + 1u) % twl->config.event_capacity;
  ++twl->event_count;
}

void twl_pump_events(Twl *twl) {
  if (twl && twl->backend_ready) {
    twl_backend_pump_events(twl);
  }
}

bool twl_poll_event(Twl *twl, TwlEvent *event) {
  if (!twl || !event) {
    return false;
  }
  if (twl->event_count == 0u) {
    return false;
  }
  *event = twl->events[twl->event_head];
  twl->event_head =
    (twl->event_head + 1u) % twl->config.event_capacity;
  --twl->event_count;
  return true;
}

bool twl_controller_state(
    const Twl *twl, uint32_t controller_index,
    TwlControllerState *state) {
  if (!twl || !state || controller_index >= twl->config.controller_capacity) {
    return false;
  }
  *state = twl->controllers[controller_index];
  return true;
}

void twl_internal_set_controller_connected(
    Twl *twl, uint32_t controller_index, bool connected) {
  TwlControllerState *state;
  TwlEvent event;
  if (!twl || controller_index >= twl->config.controller_capacity) {
    return;
  }
  state = &twl->controllers[controller_index];
  if (state->connected == connected) {
    return;
  }
  twl_internal_zero(&event, sizeof(event));
  if (!connected) {
    twl_internal_zero(state, sizeof(*state));
  } else {
    state->connected = true;
  }
  event.type = connected
    ? TWL_EVENT_CONTROLLER_CONNECTED
    : TWL_EVENT_CONTROLLER_DISCONNECTED;
  event.timestamp_us = twl_time_microseconds(twl);
  event.controller_index = (uint8_t) controller_index;
  twl_internal_push_event(twl, &event);
}

void twl_internal_set_controller_button(
    Twl *twl, uint32_t controller_index,
    TwlControllerButton button, bool pressed) {
  TwlControllerState *state;
  TwlEvent event;
  uint32_t mask;
  bool was_pressed;
  if (!twl || controller_index >= twl->config.controller_capacity ||
      button < 0 || button >= TWL_CONTROLLER_BUTTON_COUNT) {
    return;
  }
  state = &twl->controllers[controller_index];
  if (!state->connected) {
    twl_internal_set_controller_connected(twl, controller_index, true);
  }
  mask = UINT32_C(1) << (unsigned) button;
  was_pressed = (state->buttons & mask) != 0u;
  if (was_pressed == pressed) {
    return;
  }
  if (pressed) {
    state->buttons |= mask;
  } else {
    state->buttons &= ~mask;
  }
  twl_internal_zero(&event, sizeof(event));
  event.type = pressed
    ? TWL_EVENT_CONTROLLER_BUTTON_DOWN
    : TWL_EVENT_CONTROLLER_BUTTON_UP;
  event.timestamp_us = twl_time_microseconds(twl);
  event.controller_index = (uint8_t) controller_index;
  event.controller_button = button;
  twl_internal_push_event(twl, &event);
}

void twl_internal_set_controller_axis(
    Twl *twl, uint32_t controller_index,
    TwlControllerAxis axis, int16_t value) {
  TwlControllerState *state;
  TwlEvent event;
  if (!twl || controller_index >= twl->config.controller_capacity ||
      axis < 0 || axis >= TWL_CONTROLLER_AXIS_COUNT) {
    return;
  }
  state = &twl->controllers[controller_index];
  if (!state->connected) {
    twl_internal_set_controller_connected(twl, controller_index, true);
  }
  if (state->axes[axis] == value) {
    return;
  }
  state->axes[axis] = value;
  twl_internal_zero(&event, sizeof(event));
  event.type = TWL_EVENT_CONTROLLER_AXIS;
  event.timestamp_us = twl_time_microseconds(twl);
  event.controller_index = (uint8_t) controller_index;
  event.controller_axis = axis;
  event.axis_value = value;
  twl_internal_push_event(twl, &event);
}

TwlResult twl_present(Twl *twl, const TwlSurface *surface) {
  const size_t pixel_size = surface ? twl_pixel_size(surface->format) : 0u;
  if (!twl || !surface || !surface->pixels || surface->width == 0u ||
      surface->height == 0u || surface->format < TWL_PIXEL_RGB555 ||
      surface->format > TWL_PIXEL_XRGB8888 ||
      surface->width > SIZE_MAX / pixel_size ||
      surface->stride_bytes < (size_t) surface->width * pixel_size) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if (twl->config.display_mode == TWL_DISPLAY_HEADLESS) {
    return TWL_RESULT_OK;
  }
  return twl_backend_present(twl, surface);
}

void twl_internal_set_display_size(
    Twl *twl, uint32_t width, uint32_t height) {
  if (twl) {
    twl->display_width = width;
    twl->display_height = height;
  }
}

void twl_get_display_size(
    const Twl *twl, uint32_t *width, uint32_t *height) {
  if (width) {
    *width = twl ? twl->display_width : 0u;
  }
  if (height) {
    *height = twl ? twl->display_height : 0u;
  }
}

uint64_t twl_time_microseconds(const Twl *twl) {
  return twl && twl->backend_ready
           ? twl_backend_time_microseconds(twl)
           : 0u;
}
