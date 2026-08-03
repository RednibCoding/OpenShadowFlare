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

#ifndef SHADOWFLARE_RENDER_RENDERER_H
#define SHADOWFLARE_RENDER_RENDERER_H

#include "render/framebuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct SfRect {
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
} SfRect;

typedef struct SfIndexedImage {
  const uint8_t *pixels;
  const uint16_t *palette;
  uint16_t width;
  uint16_t height;
  uint16_t stride;
  uint16_t palette_size;
  uint8_t bits_per_pixel;
  bool bottom_up;
} SfIndexedImage;

typedef struct SfRgb555Image {
  const uint16_t *pixels;
  uint16_t width;
  uint16_t height;
  uint16_t stride;
} SfRgb555Image;

typedef enum SfBlendMode {
  SF_BLEND_OPAQUE = 0,
  SF_BLEND_MASKED,
  SF_BLEND_TRANSLUCENT,
  SF_BLEND_ADDITIVE
} SfBlendMode;

typedef struct SfRenderer {
  SfFramebuffer target;
} SfRenderer;

bool sf_renderer_init(
  SfRenderer *renderer, void *memory, size_t memory_size,
  uint16_t width, uint16_t height);
const SfFramebuffer *sf_renderer_framebuffer(const SfRenderer *renderer);
void sf_renderer_clear(SfRenderer *renderer, uint16_t color);
void sf_renderer_fill_rect(
  SfRenderer *renderer, SfRect rectangle, uint16_t color);
void sf_renderer_draw_indexed(
  SfRenderer *renderer, const SfIndexedImage *image,
  int x, int y, uint16_t brightness, uint16_t opacity,
  SfBlendMode blend, const SfRect *clip);
void sf_renderer_draw_indexed_tinted(
  SfRenderer *renderer, const SfIndexedImage *image,
  int x, int y, uint16_t red_strength, uint16_t green_strength,
  uint16_t blue_strength, uint16_t opacity,
  SfBlendMode blend, const SfRect *clip);
void sf_renderer_draw_rgb555(
  SfRenderer *renderer, const SfRgb555Image *image,
  int x, int y, uint16_t brightness);
void sf_renderer_restore_indexed(
  SfRenderer *renderer, const SfIndexedImage *image,
  int x, int y, uint16_t brightness, SfRect region);
void sf_renderer_draw_text(
  SfRenderer *renderer, const SfIndexedImage *font,
  const char *text, int x, int y, uint16_t color, uint16_t brightness);

#endif
