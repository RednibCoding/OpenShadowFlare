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

#include "render/renderer.h"

static uint8_t sf_indexed_pixel(
    const SfIndexedImage *image, uint16_t x, uint16_t y) {
  const uint16_t source_y = image->bottom_up
    ? (uint16_t) (image->height - y - 1u) : y;
  const uint8_t *row = image->pixels + (size_t) source_y * image->stride;
  if (image->bits_per_pixel == 8u) return row[x];
  if (image->bits_per_pixel == 4u) {
    const uint8_t packed = row[x >> 1u];
    return (uint8_t) ((packed >> ((x & 1u) ? 0u : 4u)) & 15u);
  }
  return (uint8_t) ((row[x >> 3u] >> (7u - (x & 7u))) & 1u);
}

static uint16_t sf_brighten(uint16_t color, uint16_t brightness) {
  uint16_t red;
  uint16_t green;
  uint16_t blue;
  if (brightness >= 1000u) return color;
  red = (uint16_t) ((color & 31u) * brightness / 1000u);
  green = (uint16_t) (((color >> 5u) & 31u) * brightness / 1000u);
  blue = (uint16_t) (((color >> 10u) & 31u) * brightness / 1000u);
  return sf_rgb555((uint8_t) red, (uint8_t) green, (uint8_t) blue);
}

static uint16_t sf_blend(uint16_t destination, uint16_t source,
    uint16_t opacity, bool additive) {
  uint16_t result = 0u;
  unsigned shift;
  for (shift = 0u; shift <= 10u; shift += 5u) {
    const uint16_t dst = (uint16_t) ((destination >> shift) & 31u);
    const uint16_t src = (uint16_t) ((source >> shift) & 31u);
    uint16_t value;
    if (additive) {
      value = opacity == 1000u
        ? (uint16_t) (dst + src)
        : (uint16_t) (dst + src * opacity / 1000u);
      if (value > 31u) value = 31u;
    } else {
      value = (uint16_t) (
        (src * opacity + dst * (1000u - opacity)) / 1000u);
    }
    result = (uint16_t) (result | (uint16_t) (value << shift));
  }
  return result;
}

bool sf_renderer_init(
    SfRenderer *renderer, void *memory, size_t memory_size,
    uint16_t width, uint16_t height) {
  return renderer && sf_framebuffer_init(
    &renderer->target, memory, memory_size, width, height);
}

const SfFramebuffer *sf_renderer_framebuffer(const SfRenderer *renderer) {
  return renderer ? &renderer->target : NULL;
}

void sf_renderer_clear(SfRenderer *renderer, uint16_t color) {
  if (renderer) sf_framebuffer_clear(&renderer->target, color);
}

void sf_renderer_fill_rect(
    SfRenderer *renderer, SfRect rectangle, uint16_t color) {
  if (!renderer) return;
  sf_framebuffer_fill_rect(
    &renderer->target, rectangle.x, rectangle.y,
    rectangle.width, rectangle.height, color);
}

void sf_renderer_draw_indexed(
    SfRenderer *renderer, const SfIndexedImage *image,
    int x, int y, uint16_t brightness, uint16_t opacity,
    SfBlendMode blend, const SfRect *clip) {
  int first_x = 0;
  int first_y = 0;
  int last_x;
  int last_y;
  int row_index;
  uint16_t adjusted_palette[256];
  const uint16_t *palette;
  uint16_t palette_index;
  if (!renderer || !image || !image->pixels || !image->palette ||
      image->width == 0u || image->height == 0u ||
      image->palette_size == 0u || image->palette_size > 256u ||
      (image->bits_per_pixel != 1u && image->bits_per_pixel != 4u &&
       image->bits_per_pixel != 8u) || opacity == 0u) return;
  if (brightness > 1000u) brightness = 1000u;
  if (opacity > 1000u) opacity = 1000u;
  palette = image->palette;
  if (brightness != 1000u) {
    for (palette_index = 0u; palette_index < image->palette_size;
         ++palette_index)
      adjusted_palette[palette_index] =
        sf_brighten(image->palette[palette_index], brightness);
    palette = adjusted_palette;
  }
  last_x = image->width;
  last_y = image->height;
  if (x < 0) first_x = -x;
  if (y < 0) first_y = -y;
  if (x + last_x > renderer->target.width)
    last_x = renderer->target.width - x;
  if (y + last_y > renderer->target.height)
    last_y = renderer->target.height - y;
  if (clip && clip->width > 0 && clip->height > 0) {
    if (clip->x - x > first_x) first_x = clip->x - x;
    if (clip->y - y > first_y) first_y = clip->y - y;
    if (clip->x + clip->width - x < last_x)
      last_x = clip->x + clip->width - x;
    if (clip->y + clip->height - y < last_y)
      last_y = clip->y + clip->height - y;
  }
  if (first_x >= last_x || first_y >= last_y) return;
  for (row_index = first_y; row_index < last_y; ++row_index) {
    uint16_t *destination = renderer->target.pixels +
      (size_t) (y + row_index) * renderer->target.stride;
    int column;
    for (column = first_x; column < last_x; ++column) {
      const uint8_t index = sf_indexed_pixel(
        image, (uint16_t) column, (uint16_t) row_index);
      uint16_t color;
      if (index >= image->palette_size ||
          (blend != SF_BLEND_OPAQUE && index == 0u)) continue;
      color = palette[index];
      if (blend == SF_BLEND_OPAQUE ||
          (blend == SF_BLEND_MASKED && opacity == 1000u)) {
        destination[x + column] = color;
      } else {
        destination[x + column] = sf_blend(
          destination[x + column], color, opacity,
          blend == SF_BLEND_ADDITIVE);
      }
    }
  }
}

void sf_renderer_restore_indexed(
    SfRenderer *renderer, const SfIndexedImage *image,
    int x, int y, uint16_t brightness, SfRect region) {
  sf_renderer_draw_indexed(
    renderer, image, x, y, brightness, 1000u,
    SF_BLEND_OPAQUE, &region);
}
