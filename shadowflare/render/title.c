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

#include "render/title.h"

enum { SF_TITLE_ENTRY_COUNT = 3 };

void sf_title_render(const SfGame *game, SfFramebuffer *framebuffer) {
  const uint16_t background = sf_rgb555(1u, 2u, 5u);
  const uint16_t panel = sf_rgb555(3u, 5u, 8u);
  const uint16_t idle = sf_rgb555(9u, 10u, 12u);
  const uint16_t selected = sf_rgb555(20u, 23u, 29u);
  int entry;
  if (!game || !framebuffer) return;

  /* Temporary bring-up image. Retail title artwork replaces this next. */
  sf_framebuffer_clear(framebuffer, background);
  sf_framebuffer_fill_rect(framebuffer, 96, 64, 448, 352, panel);
  sf_framebuffer_fill_rect(framebuffer, 100, 68, 440, 344, background);
  for (entry = 0; entry < SF_TITLE_ENTRY_COUNT; ++entry) {
    const int y = 270 + entry * 42;
    const uint16_t color = game->title_selection == (uint8_t) entry
      ? selected : idle;
    sf_framebuffer_fill_rect(framebuffer, 224, y, 192, 22, color);
  }
}
