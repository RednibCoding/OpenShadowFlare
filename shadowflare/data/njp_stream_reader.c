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

#include "data/njp_stream_reader.h"

#include <limits.h>
#include <string.h>

bool sf_njp_stream_read(FILE *file, void *destination, size_t size) {
  return size == 0u || fread(destination, 1u, size, file) == size;
}

bool sf_njp_stream_skip(FILE *file, long size) {
  return size >= 0 && fseek(file, size, SEEK_CUR) == 0;
}

bool sf_njp_stream_i32(FILE *file, int32_t *value) {
  uint8_t bytes[4];
  uint32_t result;
  if (!sf_njp_stream_read(file, bytes, sizeof(bytes))) return false;
  result = (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
  *value = (int32_t) result;
  return true;
}

uint32_t sf_njp_stream_u32(const uint8_t *bytes) {
  return (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
}

static bool sf_njp_stream_stride(
    int32_t bits, int32_t width, uint16_t *stride) {
  uint32_t value;
  if (width <= 0 || width > UINT16_MAX) return false;
  if (bits == 1) value = ((uint32_t) width + 7u) / 8u;
  else if (bits == 4) value = ((uint32_t) width + 1u) / 2u;
  else if (bits == 8) value = (uint32_t) width;
  else return false;
  value = (value + 3u) & ~UINT32_C(3);
  if (value > UINT16_MAX) return false;
  *stride = (uint16_t) value;
  return true;
}

bool sf_njp_stream_header(FILE *file, SfNjpStreamHeader *header) {
  char text[16];
  if (!sf_njp_stream_read(file, text, sizeof(text))) return false;
  header->united = memcmp(text, "UnitePatData", 12u) == 0;
  header->shadow = memcmp(text, "ShadowLowPat", 12u) == 0;
  if (!header->united && !header->shadow &&
      memcmp(text, "NJudgeUniPat", 12u) != 0) return false;
  if (text[12] < '0' || text[12] > '9' ||
      text[13] < '0' || text[13] > '9' ||
      text[14] < '0' || text[14] > '9') return false;
  header->version = (uint8_t) (
    (text[12] - '0') * 100 + (text[13] - '0') * 10 + text[14] - '0');
  return header->version <= 3u &&
    sf_njp_stream_i32(file, &header->part_count) &&
    header->part_count >= 0 && header->part_count <= UINT8_MAX &&
    (header->version <= 2u || sf_njp_stream_skip(file, 4));
}

bool sf_njp_stream_part(
    FILE *file, bool shadow, int32_t *bits, uint16_t *width,
    uint16_t *height, uint16_t *stride, bool *compressed,
    uint32_t *decoded_size) {
  int32_t source_width;
  int32_t source_height;
  int32_t source_compressed;
  if (!sf_njp_stream_i32(file, bits) ||
      !sf_njp_stream_i32(file, &source_width) ||
      !sf_njp_stream_i32(file, &source_height) ||
      !sf_njp_stream_i32(file, &source_compressed) ||
      source_height <= 0 || source_height > UINT16_MAX) return false;
  if (shadow) *bits = 1;
  if (!sf_njp_stream_stride(*bits, source_width, stride)) return false;
  if ((uint32_t) *stride > UINT32_MAX / (uint32_t) source_height)
    return false;
  *width = (uint16_t) source_width;
  *height = (uint16_t) source_height;
  *compressed = source_compressed != 0;
  *decoded_size = (uint32_t) *stride * (uint32_t) *height;
  return true;
}

bool sf_njp_stream_skip_part(FILE *file, bool shadow) {
  int32_t bits;
  uint16_t width;
  uint16_t height;
  uint16_t stride;
  bool compressed;
  uint32_t decoded_size;
  uint8_t compression[16];
  if (!sf_njp_stream_part(
        file, shadow, &bits, &width, &height, &stride,
        &compressed, &decoded_size)) return false;
  (void) bits;
  (void) width;
  (void) height;
  (void) stride;
  if (!compressed)
    return decoded_size <= UINT32_C(0x7fffffff) &&
      sf_njp_stream_skip(file, (long) decoded_size);
  if (!sf_njp_stream_read(file, compression, sizeof(compression)))
    return false;
  return sf_njp_stream_u32(compression + 12u) <= UINT32_C(0x7fffffff) &&
    sf_njp_stream_skip(
      file, (long) sf_njp_stream_u32(compression + 12u));
}
