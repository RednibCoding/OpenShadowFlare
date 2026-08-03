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

#ifndef SHADOWFLARE_DATA_NJP_H
#define SHADOWFLARE_DATA_NJP_H

#include "core/arena.h"
#include "render/renderer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SF_NJP_SELECTED_LIMIT 8u
#define SF_NJP_PALETTE_LIMIT 4u
#define SF_NJP_FRAME_LIMIT 64u

typedef struct SfNjpPatternImage {
  SfIndexedImage image;
  int16_t x;
  int16_t y;
} SfNjpPatternImage;

typedef struct SfNjpSelected {
  SfNjpPatternImage images[SF_NJP_SELECTED_LIMIT];
  uint16_t palettes[SF_NJP_PALETTE_LIMIT][256];
  uint8_t image_count;
  uint8_t palette_count;
} SfNjpSelected;

typedef struct SfNjpCompressedFrame {
  const uint8_t *bytes;
  uint32_t encoded_size;
  uint32_t decoded_size;
  uint16_t width;
  uint16_t height;
  uint16_t stride;
  int16_t x;
  int16_t y;
  uint8_t bits_per_pixel;
  bool compressed;
  bool blank;
} SfNjpCompressedFrame;

typedef struct SfNjpAnimation {
  SfNjpCompressedFrame frames[SF_NJP_FRAME_LIMIT];
  uint16_t palette[256];
  uint8_t frame_count;
  uint16_t palette_size;
} SfNjpAnimation;

bool sf_njp_load_selected(
  const char *path, const uint8_t *pattern_indices, uint8_t pattern_count,
  SfArena *arena, SfNjpSelected *output);
bool sf_njp_load_animation(
  const char *path, SfArena *arena, SfNjpAnimation *output);
bool sf_njp_decode_frame(
  const SfNjpAnimation *animation, uint8_t frame,
  void *scratch, size_t scratch_size, SfNjpPatternImage *output);
bool sf_njp_find_blank_frames(
  SfNjpAnimation *animation, void *scratch, size_t scratch_size);

#endif
