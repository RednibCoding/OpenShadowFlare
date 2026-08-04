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

#include "data/njp_stream_reader.h"
#include "data/rclib.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define SF_NJP_STREAM_PALETTE_LIMIT 32u

static int sf_stream_request(
    const uint8_t *requests, uint8_t request_count, int32_t source) {
  uint8_t index;
  for (index = 0u; index < request_count; ++index)
    if (requests[index] == source) return index;
  return -1;
}

static int sf_stream_palette(
    const SfNjpDecodedResource *output, int32_t source) {
  uint8_t index;
  for (index = 0u; index < output->palette_count; ++index)
    if (output->palette_sources[index] == source) return index;
  return -1;
}

static bool sf_stream_scan_patterns(
    FILE *file, const SfNjpStreamHeader *header,
    const uint8_t *requests, uint8_t request_count,
    uint8_t *reference_parts, SfNjpDecodedResource *output) {
  int32_t pattern_count;
  int32_t source_pattern;
  int32_t palette_count;
  uint8_t found[SF_NJP_DECODED_PATTERN_LIMIT] = {0u};
  if (!sf_njp_stream_i32(file, &pattern_count) || pattern_count < 0 ||
      (header->version > 2u && !sf_njp_stream_skip(file, 4))) return false;
  for (source_pattern = 0; source_pattern < pattern_count; ++source_pattern) {
    int32_t reference_count;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    int32_t palette_source = -1;
    int request;
    int32_t reference;
    SfNjpDecodedPattern *pattern = NULL;
    if (!sf_njp_stream_i32(file, &reference_count) || reference_count < 0 ||
        !sf_njp_stream_i32(file, &x) || !sf_njp_stream_i32(file, &y) ||
        !sf_njp_stream_i32(file, &width) ||
        !sf_njp_stream_i32(file, &height) ||
        (header->united && !sf_njp_stream_skip(file, 0xa8)) ||
        (header->version > 0u &&
         !sf_njp_stream_i32(file, &palette_source))) return false;
    request = sf_stream_request(requests, request_count, source_pattern);
    if (request >= 0) {
      int palette;
      if (found[request] || palette_source < 0 ||
          reference_count > (int32_t) (
            SF_NJP_DECODED_REFERENCE_LIMIT - output->reference_count) ||
          reference_count > UINT8_MAX)
        return false;
      palette = sf_stream_palette(output, palette_source);
      if (palette < 0) {
        if (output->palette_count >= SF_NJP_STREAM_PALETTE_LIMIT)
          return false;
        palette = output->palette_count++;
        output->palette_sources[palette] = (uint8_t) palette_source;
      }
      pattern = &output->patterns[output->pattern_count++];
      pattern->source_index = (uint8_t) source_pattern;
      pattern->palette = (uint8_t) palette;
      pattern->first_reference = output->reference_count;
      pattern->reference_count = (uint8_t) reference_count;
      pattern->bounds.x = x;
      pattern->bounds.y = y;
      pattern->bounds.width = width;
      pattern->bounds.height = height;
      pattern->bounds.valid = width > 0 && height > 0;
      found[request] = 1u;
    }
    for (reference = 0; reference < reference_count; ++reference) {
      int32_t status;
      int32_t source_part;
      int32_t reference_x;
      int32_t reference_y;
      int32_t palette_offset;
      int32_t scale_x;
      int32_t scale_y;
      if (!sf_njp_stream_i32(file, &status) ||
          !sf_njp_stream_i32(file, &source_part) ||
          !sf_njp_stream_i32(file, &reference_x) ||
          !sf_njp_stream_i32(file, &reference_y) ||
          !sf_njp_stream_i32(file, &palette_offset) ||
          !sf_njp_stream_i32(file, &scale_x) ||
          !sf_njp_stream_i32(file, &scale_y)) return false;
      if (pattern) {
        SfNjpDecodedReference *item;
        if (source_part < 0 || source_part >= header->part_count ||
            reference_x < INT16_MIN || reference_x > INT16_MAX ||
            reference_y < INT16_MIN || reference_y > INT16_MAX ||
            palette_offset != 0 || scale_x != 1000 || scale_y != 1000)
          return false;
        item = &output->references[output->reference_count];
        item->x = (int16_t) reference_x;
        item->y = (int16_t) reference_y;
        reference_parts[output->reference_count] = (uint8_t) source_part;
        ++output->reference_count;
      }
      (void) status;
    }
  }
  if (output->pattern_count != request_count ||
      !sf_njp_stream_i32(file, &palette_count) || palette_count < 0)
    return false;
  for (source_pattern = 0; source_pattern < palette_count; ++source_pattern) {
    const int slot = sf_stream_palette(output, source_pattern);
    unsigned entry;
    for (entry = 0u; entry < 256u; ++entry) {
      uint8_t color[4];
      if (!sf_njp_stream_read(file, color, sizeof(color))) return false;
      if (slot >= 0) output->palettes[slot][entry] = sf_rgb555(
        (uint8_t) (color[0] >> 3u), (uint8_t) (color[1] >> 3u),
        (uint8_t) (color[2] >> 3u));
    }
  }
  for (source_pattern = 0; source_pattern < output->palette_count;
       ++source_pattern) {
    if (output->palette_sources[source_pattern] >= palette_count) return false;
  }
  return true;
}

static bool sf_stream_decode_parts(
    FILE *file, const SfNjpStreamHeader *header,
    const uint8_t *reference_parts, SfArena *arena,
    SfNjpDecodedResource *output) {
  int32_t source_part;
  for (source_part = 0; source_part < header->part_count; ++source_part) {
    int32_t bits;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    bool compressed;
    uint32_t decoded_size;
    uint8_t reference;
    bool selected = false;
    if (!sf_njp_stream_part(
          file, header->shadow, &bits, &width, &height, &stride,
          &compressed, &decoded_size)) return false;
    for (reference = 0u; reference < output->reference_count; ++reference)
      if (reference_parts[reference] == source_part) selected = true;
    if (selected) {
      SfNjpDecodedPart *part;
      uint8_t *pixels;
      uint8_t slot;
      if (output->part_count >= SF_NJP_DECODED_PART_LIMIT) return false;
      slot = output->part_count++;
      part = &output->parts[slot];
      pixels = (uint8_t *) sf_arena_push(arena, decoded_size, 4u);
      if (!pixels) return false;
      if (compressed) {
        if (!sf_rclib_decode_stream(file, pixels, decoded_size)) return false;
      } else if (!sf_njp_stream_read(file, pixels, decoded_size)) return false;
      part->image.pixels = pixels;
      part->image.palette = NULL;
      part->image.width = width;
      part->image.height = height;
      part->image.stride = stride;
      part->image.palette_size = bits == 8
        ? 256u : (uint16_t) (1u << bits);
      part->image.bits_per_pixel = (uint8_t) bits;
      part->image.bottom_up = true;
      part->source_index = (uint8_t) source_part;
      for (reference = 0u; reference < output->reference_count; ++reference)
        if (reference_parts[reference] == source_part)
          output->references[reference].part = slot;
    } else if (!compressed) {
      if (decoded_size > UINT32_C(0x7fffffff) ||
          !sf_njp_stream_skip(file, (long) decoded_size)) return false;
    } else {
      uint8_t compression[16];
      uint32_t encoded_size;
      if (!sf_njp_stream_read(file, compression, sizeof(compression)))
        return false;
      encoded_size = sf_njp_stream_u32(compression + 12u);
      if (encoded_size > UINT32_C(0x7fffffff) ||
          !sf_njp_stream_skip(file, (long) encoded_size)) return false;
    }
  }
  return true;
}

bool sf_njp_stream_decoded_patterns(
    const char *path, const uint8_t *pattern_indices, uint8_t pattern_count,
    SfArena *arena, SfNjpDecodedResource *output) {
  SfNjpStreamHeader header;
  uint8_t reference_parts[SF_NJP_DECODED_REFERENCE_LIMIT];
  size_t mark;
  FILE *file = NULL;
  bool success = false;
  uint8_t index;
  int32_t part;
  if (!path || !pattern_indices || pattern_count == 0u ||
      pattern_count > SF_NJP_DECODED_PATTERN_LIMIT || !arena || !output)
    return false;
  for (index = 0u; index < pattern_count; ++index) {
    uint8_t previous;
    for (previous = 0u; previous < index; ++previous)
      if (pattern_indices[previous] == pattern_indices[index]) return false;
  }
  mark = sf_arena_mark(arena);
  memset(output, 0, sizeof(*output));
  output->palettes = (uint16_t (*)[256]) sf_arena_push_zero(
    arena, SF_NJP_STREAM_PALETTE_LIMIT * sizeof(*output->palettes),
    sizeof(uint16_t));
  output->palette_sources = (uint8_t *) sf_arena_push(
    arena, SF_NJP_STREAM_PALETTE_LIMIT, sizeof(uint8_t));
  if (!output->palettes || !output->palette_sources) goto done;
  file = fopen(path, "rb");
  if (!file || !sf_njp_stream_header(file, &header)) goto done;
  output->is_shadow = header.shadow;
  for (part = 0; part < header.part_count; ++part)
    if (!sf_njp_stream_skip_part(file, header.shadow)) goto done;
  if (!sf_stream_scan_patterns(
        file, &header, pattern_indices, pattern_count,
        reference_parts, output)) goto done;
  fclose(file);
  file = fopen(path, "rb");
  if (!file || !sf_njp_stream_header(file, &header) ||
      !sf_stream_decode_parts(
        file, &header, reference_parts, arena, output)) goto done;
  success = true;
done:
  if (file) fclose(file);
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(output, 0, sizeof(*output));
  }
  return success;
}
