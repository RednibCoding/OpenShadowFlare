/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of OpenShadowFlare.
 *
 * OpenShadowFlare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * OpenShadowFlare is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
 */

#include "twl.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef union {
  void *pointer_alignment;
  uint64_t integer_alignment;
  long double floating_point_alignment;
  uint8_t bytes[16384];
} AlignedStorage;

int main(void) {
  AlignedStorage storage;
  uint16_t framebuffer[64u * 64u];
  TwlConfig config = twl_config_default();
  TwlSurface surface;
  TwlEvent event;
  Twl *twl = NULL;
  size_t required;
  size_t index;

  config.width = 64u;
  config.height = 64u;
  config.event_capacity = 16u;
  config.controller_capacity = 4u;
  required = twl_memory_required(&config);
  if (required == 0u || required > sizeof(storage.bytes)) return 1;
  if (twl_init(storage.bytes, required, &config, &twl) != TWL_RESULT_OK)
    return 1;
  for (index = 0u; index < sizeof(framebuffer) / sizeof(framebuffer[0]); ++index)
    framebuffer[index] = (uint16_t) ((index & 31u) | ((index & 31u) << 10u));
  surface.pixels = framebuffer;
  surface.width = 64u;
  surface.height = 64u;
  surface.stride_bytes = 64u * sizeof(uint16_t);
  surface.format = TWL_PIXEL_RGB555;
  if (twl_present(twl, &surface) != TWL_RESULT_OK) return 1;
  twl_pump_events(twl);
  while (twl_poll_event(twl, &event)) {
  }
  twl_shutdown(twl);
#ifdef __EMSCRIPTEN__
  EM_ASM({ document.body.setAttribute('data-twl-smoke', 'passed'); });
#endif
  return 0;
}
