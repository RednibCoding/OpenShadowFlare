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

#ifndef SHADOWFLARE_RENDER_FRAMEBUFFER_H
#define SHADOWFLARE_RENDER_FRAMEBUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct SfFramebuffer {
  uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  uint16_t stride;
} SfFramebuffer;

uint16_t sf_rgb555(uint8_t red, uint8_t green, uint8_t blue);
bool sf_framebuffer_init(
  SfFramebuffer *framebuffer, void *memory, size_t memory_size,
  uint16_t width, uint16_t height);
void sf_framebuffer_clear(SfFramebuffer *framebuffer, uint16_t color);
void sf_framebuffer_fill_rect(
  SfFramebuffer *framebuffer, int x, int y, int width, int height,
  uint16_t color);

#endif
