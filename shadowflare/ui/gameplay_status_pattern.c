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

#include "ui/gameplay_status_pattern.h"

void sf_gameplay_status_pattern_draw(
    SfRenderer *renderer, const SfNjpDecodedResource *resource,
    uint8_t source_pattern, int x, int y, const SfRect *clip) {
  sf_gameplay_status_pattern_draw_opacity(
    renderer, resource, source_pattern, x, y, 1000u, clip);
}

void sf_gameplay_status_pattern_draw_opacity(
    SfRenderer *renderer, const SfNjpDecodedResource *resource,
    uint8_t source_pattern, int x, int y, uint16_t opacity,
    const SfRect *clip) {
  const SfNjpDecodedPattern *pattern =
    sf_njp_decoded_pattern(resource, source_pattern);
  uint8_t reference;
  if (!renderer || !pattern || pattern->palette >= resource->palette_count)
    return;
  for (reference = 0u; reference < pattern->reference_count; ++reference) {
    const SfNjpDecodedReference *item =
      &resource->references[pattern->first_reference + reference];
    SfIndexedImage image;
    if (item->part >= resource->part_count) continue;
    image = resource->parts[item->part].image;
    image.palette = resource->palettes[pattern->palette];
    sf_renderer_draw_indexed(
      renderer, &image, x + item->x, y + item->y,
      1000u, opacity,
      opacity < 1000u ? SF_BLEND_TRANSLUCENT : SF_BLEND_MASKED, clip);
  }
}
