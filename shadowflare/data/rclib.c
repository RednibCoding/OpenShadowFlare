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

#include "data/rclib.h"

#include <string.h>

typedef int (*SfRclibReadByte)(void *source);

typedef struct SfMemoryBytes {
  const uint8_t *current;
  const uint8_t *end;
} SfMemoryBytes;

static uint32_t sf_u32(const uint8_t *bytes) {
  return (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
}

static int sf_read_memory_byte(void *source) {
  SfMemoryBytes *bytes = (SfMemoryBytes *) source;
  if (bytes->current == bytes->end) return -1;
  return *bytes->current++;
}

static int sf_read_file_byte(void *source) {
  return fgetc((FILE *) source);
}

static bool sf_rclib_decode(
    SfRclibReadByte read_byte, void *source,
    uint8_t *decoded, size_t decoded_size) {
  uint8_t window[4096];
  size_t destination = 0u;
  uint16_t window_position = 0x0feeu;
  memset(window, 0, sizeof(window));
  while (destination < decoded_size) {
    const int flags = read_byte(source);
    uint8_t mask;
    if (flags < 0) return false;
    for (mask = 0x80u; mask != 0u && destination < decoded_size;
         mask >>= 1u) {
      if (((uint8_t) flags & mask) != 0u) {
        const int first = read_byte(source);
        const int second = read_byte(source);
        uint16_t offset;
        uint8_t length;
        uint8_t index;
        if (first < 0 || second < 0) return false;
        offset = (uint16_t) ((uint8_t) first |
          ((uint16_t) ((uint8_t) second & 0xf0u) << 4u));
        length = (uint8_t) (((uint8_t) second & 15u) + 3u);
        for (index = 0u; index < length && destination < decoded_size;
             ++index) {
          const uint8_t value = window[(offset + index) & 0x0fffu];
          decoded[destination++] = value;
          window[window_position] = value;
          window_position = (uint16_t) ((window_position + 1u) & 0x0fffu);
        }
      } else {
        const int literal = read_byte(source);
        if (literal < 0) return false;
        decoded[destination++] = (uint8_t) literal;
        window[window_position] = (uint8_t) literal;
        window_position = (uint16_t) ((window_position + 1u) & 0x0fffu);
      }
    }
  }
  return true;
}

bool sf_rclib_decode_memory(
    const uint8_t *encoded, size_t encoded_size,
    uint8_t *decoded, size_t decoded_size) {
  SfMemoryBytes source;
  uint32_t payload_size;
  if (!encoded || !decoded || encoded_size < 16u ||
      memcmp(encoded, "RCLIB-L", 7u) != 0 ||
      sf_u32(encoded + 8u) != decoded_size) return false;
  payload_size = sf_u32(encoded + 12u);
  if ((size_t) payload_size > encoded_size - 16u) return false;
  source.current = encoded + 16u;
  source.end = source.current + payload_size;
  return sf_rclib_decode(
    sf_read_memory_byte, &source, decoded, decoded_size);
}

bool sf_rclib_decode_stream(
    FILE *file, uint8_t *decoded, size_t decoded_size) {
  uint8_t header[16];
  if (!file || !decoded || fread(header, 1u, sizeof(header), file) !=
      sizeof(header) || memcmp(header, "RCLIB-L", 7u) != 0 ||
      sf_u32(header + 8u) != decoded_size) return false;
  return sf_rclib_decode(sf_read_file_byte, file, decoded, decoded_size);
}
