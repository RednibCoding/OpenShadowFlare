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

typedef struct SfNjpSparseRequest {
  int32_t pattern;
  int32_t part;
  int32_t palette_source;
  int16_t x;
  int16_t y;
  uint8_t palette_slot;
  bool found;
} SfNjpSparseRequest;

static int sf_njp_sparse_request(
    const SfNjpSparseRequest *requests, uint16_t count, int32_t pattern) {
  uint16_t index;
  for (index = 0u; index < count; ++index) {
    if (requests[index].pattern == pattern) return (int) index;
  }
  return -1;
}

static int sf_njp_sparse_palette(
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
    FILE *file, SfNjpSparseRequest *requests, uint16_t request_count,
    SfNjpSparseResource *output) {
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
    selected = sf_njp_sparse_request(requests, request_count, pattern);
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
        SfNjpSparseRequest *request = &requests[selected];
        int palette_slot;
        if (reference_part < 0 || reference_part >= part_count ||
            x < INT16_MIN || x > INT16_MAX ||
            y < INT16_MIN || y > INT16_MAX || palette_offset != 0 ||
            scale_x != 1000 || scale_y != 1000 || palette_source < 0)
          return false;
        palette_slot = sf_njp_sparse_palette(output, palette_source);
        if (palette_slot < 0) {
          if (output->palette_count >= SF_NJP_SPARSE_PALETTE_LIMIT)
            return false;
          palette_slot = output->palette_count++;
          output->palette_sources[palette_slot] = palette_source;
        }
        request->part = reference_part;
        request->palette_source = palette_source;
        request->palette_slot = (uint8_t) palette_slot;
        request->x = (int16_t) x;
        request->y = (int16_t) y;
        request->found = true;
      }
      (void) reference_status;
    }
  }
  for (pattern = 0; pattern < request_count; ++pattern) {
    if (!requests[pattern].found) return false;
  }
  if (!sf_i32(file, &palette_count) || palette_count < 0) return false;
  for (pattern = 0; pattern < palette_count; ++pattern) {
    const int slot = sf_njp_sparse_palette(output, pattern);
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
    FILE *file, const SfNjpSparseRequest *requests, uint16_t request_count,
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
      if (requests[request].part == part) selected = true;
    }
    if (selected) {
      pixels = (uint8_t *) sf_arena_push(arena, decoded_size, 4u);
      if (!pixels) return false;
      if (compressed) {
        if (!sf_rclib_decode_stream(file, pixels, decoded_size)) return false;
      } else if (!sf_read(file, pixels, decoded_size)) return false;
      for (request = 0u; request < request_count; ++request) {
        SfNjpSparsePattern *pattern;
        if (requests[request].part != part) continue;
        pattern = &output->patterns[request];
        pattern->source_index = requests[request].pattern;
        pattern->palette = requests[request].palette_slot;
        pattern->image.image.pixels = pixels;
        pattern->image.image.palette =
          output->palettes[requests[request].palette_slot];
        pattern->image.image.width = checked_width;
        pattern->image.image.height = checked_height;
        pattern->image.image.stride = stride;
        pattern->image.image.palette_size = bits == 8
          ? 256u : (uint16_t) (1u << bits);
        pattern->image.image.bits_per_pixel = (uint8_t) bits;
        pattern->image.image.bottom_up = true;
        pattern->image.x = requests[request].x;
        pattern->image.y = requests[request].y;
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

bool sf_njp_load_sparse_patterns(
    const char *path, const int32_t *pattern_indices, uint16_t pattern_count,
    SfArena *arena, SfNjpSparseResource *output) {
  SfNjpSparseRequest requests[SF_NJP_SPARSE_PATTERN_LIMIT];
  FILE *file;
  size_t mark;
  uint16_t pattern;
  bool success = false;
  if (!path || !pattern_indices || pattern_count == 0u ||
      pattern_count > SF_NJP_SPARSE_PATTERN_LIMIT || !arena || !output)
    return false;
  mark = sf_arena_mark(arena);
  memset(output, 0, sizeof(*output));
  memset(requests, 0, sizeof(requests));
  for (pattern = 0u; pattern < pattern_count; ++pattern) {
    if (pattern_indices[pattern] < 0 ||
        sf_njp_sparse_request(requests, pattern, pattern_indices[pattern]) >= 0)
      goto done;
    requests[pattern].pattern = pattern_indices[pattern];
  }
  file = fopen(path, "rb");
  if (!file) goto done;
  success = sf_njp_sparse_scan(file, requests, pattern_count, output);
  fclose(file);
  if (!success) goto done;
  file = fopen(path, "rb");
  if (!file) {
    success = false;
    goto done;
  }
  success = sf_njp_sparse_decode(
    file, requests, pattern_count, arena, output);
  fclose(file);
  if (success) output->pattern_count = pattern_count;
done:
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(output, 0, sizeof(*output));
  }
  return success;
}

const SfNjpSparsePattern *sf_njp_sparse_pattern(
    const SfNjpSparseResource *resource, int32_t source_index) {
  uint16_t pattern;
  if (!resource) return NULL;
  for (pattern = 0u; pattern < resource->pattern_count; ++pattern) {
    if (resource->patterns[pattern].source_index == source_index)
      return &resource->patterns[pattern];
  }
  return NULL;
}

