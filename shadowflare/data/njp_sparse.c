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

#include "data/njp.h"

#include "data/rclib.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static bool sf_read(FILE *file, void *destination, size_t size) {
  return size == 0u || fread(destination, 1u, size, file) == size;
}

static bool sf_skip(FILE *file, long size) {
  return size >= 0 && fseek(file, size, SEEK_CUR) == 0;
}

static bool sf_i32(FILE *file, int32_t *value) {
  uint8_t bytes[4];
  uint32_t result;
  if (!sf_read(file, bytes, sizeof(bytes))) return false;
  result = (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
  *value = (int32_t) result;
  return true;
}

static uint32_t sf_u32_bytes(const uint8_t *bytes) {
  return (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
}

static bool sf_dimension(int32_t value, uint16_t *output) {
  if (value <= 0 || value > UINT16_MAX) return false;
  *output = (uint16_t) value;
  return true;
}

static bool sf_stride(uint8_t bits, uint16_t width, uint16_t *output) {
  uint32_t stride;
  if (bits == 1u) stride = ((uint32_t) width + 7u) / 8u;
  else if (bits == 4u) stride = ((uint32_t) width + 1u) / 2u;
  else if (bits == 8u) stride = width;
  else return false;
  stride = (stride + 3u) & ~UINT32_C(3);
  if (stride > UINT16_MAX) return false;
  *output = (uint16_t) stride;
  return true;
}

static int sf_njp_sparse_request(
    const SfNjpSparseResource *output, uint16_t count, int32_t pattern) {
  uint16_t first = 0u;
  uint16_t last = count;
  while (first < last) {
    const uint16_t middle = (uint16_t) (first + (last - first) / 2u);
    const int32_t source = output->patterns[middle].source_index;
    if (source == pattern) return (int) middle;
    if (source < pattern) first = (uint16_t) (middle + 1u);
    else last = middle;
  }
  return -1;
}

static bool sf_njp_sparse_prepare_patterns(
    SfNjpSparseResource *output,
    const int32_t *pattern_indices, uint16_t pattern_count) {
  uint16_t pattern;
  for (pattern = 0u; pattern < pattern_count; ++pattern) {
    uint16_t position = pattern;
    if (pattern_indices[pattern] < 0) return false;
    while (position > 0u &&
           output->patterns[position - 1u].source_index >
             pattern_indices[pattern]) {
      output->patterns[position] = output->patterns[position - 1u];
      --position;
    }
    if (position > 0u &&
        output->patterns[position - 1u].source_index ==
          pattern_indices[pattern]) return false;
    output->patterns[position].source_index = pattern_indices[pattern];
    output->patterns[position].source_part = -1;
  }
  return true;
}

static int sf_njp_sparse_palette_slot(
    const SfNjpSparseResource *output, int32_t source) {
  uint8_t palette;
  for (palette = 0u; palette < output->palette_count; ++palette) {
    if (output->palette_sources[palette] == source) return (int) palette;
  }
  return -1;
}

static bool sf_njp_sparse_begin(
    FILE *file, uint8_t *version, bool *united,
    bool *shadow, int32_t *part_count) {
  char header[16];
  if (!sf_read(file, header, sizeof(header))) return false;
  *united = memcmp(header, "UnitePatData", 12u) == 0;
  *shadow = memcmp(header, "ShadowLowPat", 12u) == 0;
  if (!*united && !*shadow &&
      memcmp(header, "NJudgeUniPat", 12u) != 0) return false;
  if (header[12] < '0' || header[12] > '9' ||
      header[13] < '0' || header[13] > '9' ||
      header[14] < '0' || header[14] > '9') return false;
  *version = (uint8_t) (
    (header[12] - '0') * 100 + (header[13] - '0') * 10 +
    header[14] - '0');
  return *version <= 3u && sf_i32(file, part_count) && *part_count >= 0 &&
    (*version <= 2u || sf_skip(file, 4));
}

static bool sf_njp_sparse_skip_part(FILE *file, bool shadow) {
  int32_t bits;
  int32_t width;
  int32_t height;
  int32_t compressed;
  uint16_t checked_width;
  uint16_t checked_height;
  uint16_t stride;
  uint8_t header[16];
  uint32_t decoded_size;
  if (!sf_i32(file, &bits) || !sf_i32(file, &width) ||
      !sf_i32(file, &height) || !sf_i32(file, &compressed) ||
      !sf_dimension(width, &checked_width) ||
      !sf_dimension(height, &checked_height)) return false;
  if (shadow) bits = 1;
  if (bits < 0 || bits > UINT8_MAX ||
      !sf_stride((uint8_t) bits, checked_width, &stride)) return false;
  decoded_size = (uint32_t) stride * checked_height;
  if (!compressed) return decoded_size <= UINT32_C(0x7fffffff) &&
    sf_skip(file, (long) decoded_size);
  if (!sf_read(file, header, sizeof(header))) return false;
  return sf_u32_bytes(header + 12u) <= UINT32_C(0x7fffffff) &&
    sf_skip(file, (long) sf_u32_bytes(header + 12u));
}

static bool sf_njp_sparse_scan(
    FILE *file, uint16_t request_count, SfNjpSparseResource *output) {
  uint8_t version;
  bool united;
  bool shadow;
  int32_t part_count;
  int32_t part;
  int32_t pattern_count;
  int32_t pattern;
  int32_t palette_count;
  if (!sf_njp_sparse_begin(
        file, &version, &united, &shadow, &part_count)) return false;
  for (part = 0; part < part_count; ++part) {
    if (!sf_njp_sparse_skip_part(file, shadow)) return false;
  }
  if (!sf_i32(file, &pattern_count) || pattern_count < 0 ||
      (version > 2u && !sf_skip(file, 4))) return false;
  for (pattern = 0; pattern < pattern_count; ++pattern) {
    int32_t reference_count;
    int32_t ignored;
    int32_t palette_source = -1;
    int selected;
    int32_t reference;
    if (!sf_i32(file, &reference_count) || reference_count < 0 ||
        !sf_i32(file, &ignored) || !sf_i32(file, &ignored) ||
        !sf_i32(file, &ignored) || !sf_i32(file, &ignored) ||
        (united && !sf_skip(file, 0xa8)) ||
        (version > 0u && !sf_i32(file, &palette_source))) return false;
    selected = sf_njp_sparse_request(output, request_count, pattern);
    if (selected >= 0 && reference_count != 1) return false;
    for (reference = 0; reference < reference_count; ++reference) {
      int32_t reference_status;
      int32_t reference_part;
      int32_t x;
      int32_t y;
      int32_t palette_offset;
      int32_t scale_x;
      int32_t scale_y;
      if (!sf_i32(file, &reference_status) ||
          !sf_i32(file, &reference_part) || !sf_i32(file, &x) ||
          !sf_i32(file, &y) || !sf_i32(file, &palette_offset) ||
          !sf_i32(file, &scale_x) || !sf_i32(file, &scale_y)) return false;
      if (selected >= 0) {
        SfNjpSparsePattern *request = &output->patterns[selected];
        int palette_slot;
        if (reference_part < 0 || reference_part >= part_count ||
            x < INT16_MIN || x > INT16_MAX ||
            y < INT16_MIN || y > INT16_MAX || palette_offset != 0 ||
            scale_x != 1000 || scale_y != 1000 || palette_source < 0)
          return false;
        palette_slot = sf_njp_sparse_palette_slot(output, palette_source);
        if (palette_slot < 0) {
          if (output->palette_count >= output->palette_capacity)
            return false;
          palette_slot = output->palette_count++;
          output->palette_sources[palette_slot] = palette_source;
        }
        request->source_part = reference_part;
        request->palette = (uint8_t) palette_slot;
        request->image.x = (int16_t) x;
        request->image.y = (int16_t) y;
      }
      (void) reference_status;
    }
  }
  for (pattern = 0; pattern < request_count; ++pattern) {
    if (output->patterns[pattern].source_part < 0) return false;
  }
  if (!sf_i32(file, &palette_count) || palette_count < 0) return false;
  for (pattern = 0; pattern < palette_count; ++pattern) {
    const int slot = sf_njp_sparse_palette_slot(output, pattern);
    unsigned entry;
    for (entry = 0u; entry < 256u; ++entry) {
      uint8_t color[4];
      if (!sf_read(file, color, sizeof(color))) return false;
      if (slot >= 0) output->palettes[slot][entry] = sf_rgb555(
        (uint8_t) (color[0] >> 3u), (uint8_t) (color[1] >> 3u),
        (uint8_t) (color[2] >> 3u));
    }
  }
  for (pattern = 0; pattern < output->palette_count; ++pattern) {
    if (output->palette_sources[pattern] >= palette_count) return false;
  }
  return true;
}

static bool sf_njp_sparse_decode(
    FILE *file, uint16_t request_count,
    SfArena *arena, SfNjpSparseResource *output) {
  uint8_t version;
  bool united;
  bool shadow;
  int32_t part_count;
  int32_t part;
  if (!sf_njp_sparse_begin(
        file, &version, &united, &shadow, &part_count)) return false;
  for (part = 0; part < part_count; ++part) {
    int32_t bits;
    int32_t width;
    int32_t height;
    int32_t compressed;
    uint16_t checked_width;
    uint16_t checked_height;
    uint16_t stride;
    uint32_t decoded_size;
    uint16_t request;
    bool selected = false;
    uint8_t *pixels = NULL;
    if (!sf_i32(file, &bits) || !sf_i32(file, &width) ||
        !sf_i32(file, &height) || !sf_i32(file, &compressed) ||
        !sf_dimension(width, &checked_width) ||
        !sf_dimension(height, &checked_height)) return false;
    if (shadow) bits = 1;
    if (bits < 0 || bits > UINT8_MAX ||
        !sf_stride((uint8_t) bits, checked_width, &stride)) return false;
    decoded_size = (uint32_t) stride * checked_height;
    for (request = 0u; request < request_count; ++request) {
      if (output->patterns[request].source_part == part) selected = true;
    }
    if (selected) {
      pixels = (uint8_t *) sf_arena_push(arena, decoded_size, 4u);
      if (!pixels) return false;
      if (compressed) {
        if (!sf_rclib_decode_stream(file, pixels, decoded_size)) return false;
      } else if (!sf_read(file, pixels, decoded_size)) return false;
      for (request = 0u; request < request_count; ++request) {
        SfNjpSparsePattern *pattern;
        if (output->patterns[request].source_part != part) continue;
        pattern = &output->patterns[request];
        pattern->image.image.pixels = pixels;
        pattern->image.image.palette =
          output->palettes[pattern->palette];
        pattern->image.image.width = checked_width;
        pattern->image.image.height = checked_height;
        pattern->image.image.stride = stride;
        pattern->image.image.palette_size = bits == 8
          ? 256u : (uint16_t) (1u << bits);
        pattern->image.image.bits_per_pixel = (uint8_t) bits;
        pattern->image.image.bottom_up = true;
      }
    } else if (!compressed) {
      if (decoded_size > UINT32_C(0x7fffffff) ||
          !sf_skip(file, (long) decoded_size)) return false;
    } else {
      uint8_t header[16];
      uint32_t encoded_size;
      if (!sf_read(file, header, sizeof(header))) return false;
      encoded_size = sf_u32_bytes(header + 12u);
      if (encoded_size > UINT32_C(0x7fffffff) ||
          !sf_skip(file, (long) encoded_size)) return false;
    }
  }
  (void) version;
  (void) united;
  return true;
}

bool sf_njp_load_sparse_patterns_with_palette_capacity(
    const char *path, const int32_t *pattern_indices, uint16_t pattern_count,
    const int32_t *palette_indices, uint8_t palette_count,
    uint8_t palette_capacity, SfArena *arena, SfNjpSparseResource *output) {
  FILE *file;
  size_t mark;
  uint16_t pattern;
  uint16_t maximum_palette_count;
  bool success = false;
  if (!path || !pattern_indices || pattern_count == 0u ||
      pattern_count > SF_NJP_SPARSE_PATTERN_LIMIT || !arena || !output)
    return false;
  if ((palette_count > 0u && !palette_indices) || palette_capacity == 0u ||
      palette_capacity > SF_NJP_SPARSE_PALETTE_LIMIT ||
      palette_count > palette_capacity) return false;
  mark = sf_arena_mark(arena);
  memset(output, 0, sizeof(*output));
  maximum_palette_count = pattern_count + palette_count;
  if (maximum_palette_count > palette_capacity)
    maximum_palette_count = palette_capacity;
  output->palette_capacity = (uint8_t) maximum_palette_count;
  output->patterns = (SfNjpSparsePattern *) sf_arena_push_zero(
    arena, (size_t) pattern_count * sizeof(*output->patterns), sizeof(void *));
  output->palettes = (uint16_t (*)[256]) sf_arena_push_zero(
    arena, sizeof(*output->palettes) * output->palette_capacity,
    sizeof(uint16_t));
  output->palette_sources = (int32_t *) sf_arena_push(
    arena, sizeof(*output->palette_sources) * output->palette_capacity,
    sizeof(int32_t));
  if (!output->patterns || !output->palettes || !output->palette_sources)
    goto done;
  for (pattern = 0u; pattern < output->palette_capacity; ++pattern)
    output->palette_sources[pattern] = -1;
  for (pattern = 0u; pattern < palette_count; ++pattern) {
    if (palette_indices[pattern] < 0 ||
        sf_njp_sparse_palette_slot(output, palette_indices[pattern]) >= 0)
      goto done;
    output->palette_sources[output->palette_count++] =
      palette_indices[pattern];
  }
  if (!sf_njp_sparse_prepare_patterns(
        output, pattern_indices, pattern_count)) goto done;
  file = fopen(path, "rb");
  if (!file) goto done;
  success = sf_njp_sparse_scan(file, pattern_count, output);
  fclose(file);
  if (!success) goto done;
  file = fopen(path, "rb");
  if (!file) {
    success = false;
    goto done;
  }
  success = sf_njp_sparse_decode(
    file, pattern_count, arena, output);
  fclose(file);
  if (success) output->pattern_count = pattern_count;
done:
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(output, 0, sizeof(*output));
  }
  return success;
}

bool sf_njp_load_sparse_patterns_with_palettes(
    const char *path, const int32_t *pattern_indices, uint16_t pattern_count,
    const int32_t *palette_indices, uint8_t palette_count,
    SfArena *arena, SfNjpSparseResource *output) {
  return sf_njp_load_sparse_patterns_with_palette_capacity(
    path, pattern_indices, pattern_count, palette_indices, palette_count,
    SF_NJP_SPARSE_DEFAULT_PALETTE_LIMIT, arena, output);
}

bool sf_njp_load_sparse_patterns(
    const char *path, const int32_t *pattern_indices, uint16_t pattern_count,
    SfArena *arena, SfNjpSparseResource *output) {
  return sf_njp_load_sparse_patterns_with_palettes(
    path, pattern_indices, pattern_count, NULL, 0u, arena, output);
}

const SfNjpSparsePattern *sf_njp_sparse_pattern(
    const SfNjpSparseResource *resource, int32_t source_index) {
  int pattern;
  if (!resource) return NULL;
  pattern = sf_njp_sparse_request(
    resource, resource->pattern_count, source_index);
  return pattern >= 0 ? &resource->patterns[pattern] : NULL;
}

const uint16_t *sf_njp_sparse_palette(
    const SfNjpSparseResource *resource, int32_t source_index) {
  const int slot = resource
    ? sf_njp_sparse_palette_slot(resource, source_index) : -1;
  return slot >= 0 ? resource->palettes[slot] : NULL;
}
