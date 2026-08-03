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
#define SF_NJP_SELECTED_PALETTE_LIMIT 8u
#define SF_NJP_FRAME_LIMIT 64u
#define SF_NJP_DECODED_PATTERN_LIMIT 64u
#define SF_NJP_DECODED_PART_LIMIT 80u
#define SF_NJP_DECODED_REFERENCE_LIMIT 128u
#define SF_NJP_PATTERN_FILE_LIMIT 80u
#define SF_NJP_SPARSE_PATTERN_LIMIT 2048u
#define SF_NJP_SPARSE_PALETTE_LIMIT 8u

typedef struct SfNjpPatternImage {
  SfIndexedImage image;
  int16_t x;
  int16_t y;
} SfNjpPatternImage;

typedef struct SfNjpSelected {
  SfNjpPatternImage images[SF_NJP_SELECTED_LIMIT];
  uint16_t palettes[SF_NJP_SELECTED_PALETTE_LIMIT][256];
  uint8_t palette_sources[SF_NJP_SELECTED_PALETTE_LIMIT];
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

typedef struct SfNjpDecodedPart {
  SfIndexedImage image;
  uint8_t source_index;
} SfNjpDecodedPart;

typedef struct SfNjpDecodedReference {
  int16_t x;
  int16_t y;
  uint8_t part;
} SfNjpDecodedReference;

typedef struct SfNjpPatternBounds {
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
  bool valid;
} SfNjpPatternBounds;

typedef struct SfNjpDecodedPattern {
  SfNjpPatternBounds bounds;
  uint8_t source_index;
  uint8_t palette;
  uint8_t first_reference;
  uint8_t reference_count;
} SfNjpDecodedPattern;

typedef struct SfNjpDecodedResource {
  SfNjpDecodedPart parts[SF_NJP_DECODED_PART_LIMIT];
  SfNjpDecodedReference references[SF_NJP_DECODED_REFERENCE_LIMIT];
  SfNjpDecodedPattern patterns[SF_NJP_DECODED_PATTERN_LIMIT];
  uint16_t (*palettes)[256];
  uint8_t *palette_sources;
  uint8_t part_count;
  uint8_t reference_count;
  uint8_t pattern_count;
  uint8_t palette_count;
  bool is_shadow;
} SfNjpDecodedResource;

typedef struct SfNjpSparsePattern {
  SfNjpPatternImage image;
  int32_t source_index;
  int32_t source_part;
  uint8_t palette;
} SfNjpSparsePattern;

typedef struct SfNjpSparseResource {
  SfNjpSparsePattern *patterns;
  uint16_t (*palettes)[256];
  int32_t *palette_sources;
  uint16_t pattern_count;
  uint8_t palette_count;
} SfNjpSparseResource;

bool sf_njp_load_selected(
  const char *path, const uint8_t *pattern_indices, uint8_t pattern_count,
  SfArena *arena, SfNjpSelected *output);
bool sf_njp_load_animation(
  const char *path, SfArena *arena, SfNjpAnimation *output);
bool sf_njp_load_decoded_patterns(
  const char *path, const uint8_t *pattern_indices, uint8_t pattern_count,
  SfArena *arena, SfNjpDecodedResource *output);
bool sf_njp_read_pattern_bounds(
  const char *path, SfNjpPatternBounds *bounds,
  uint8_t capacity, uint8_t *pattern_count);
bool sf_njp_load_sparse_patterns(
  const char *path, const int32_t *pattern_indices, uint16_t pattern_count,
  SfArena *arena, SfNjpSparseResource *output);
const SfNjpSparsePattern *sf_njp_sparse_pattern(
  const SfNjpSparseResource *resource, int32_t source_index);
const SfNjpDecodedPattern *sf_njp_decoded_pattern(
  const SfNjpDecodedResource *resource, uint8_t source_index);
const uint16_t *sf_njp_decoded_palette(
  const SfNjpDecodedResource *resource, uint16_t source_index);
bool sf_njp_decode_frame(
  const SfNjpAnimation *animation, uint8_t frame,
  void *scratch, size_t scratch_size, SfNjpPatternImage *output);
bool sf_njp_find_blank_frames(
  SfNjpAnimation *animation, void *scratch, size_t scratch_size);

#endif
