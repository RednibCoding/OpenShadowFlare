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

#ifndef SHADOWFLARE_DATA_NJP_STREAM_READER_H
#define SHADOWFLARE_DATA_NJP_STREAM_READER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct SfNjpStreamHeader {
  int32_t part_count;
  uint8_t version;
  bool united;
  bool shadow;
} SfNjpStreamHeader;

bool sf_njp_stream_read(FILE *file, void *destination, size_t size);
bool sf_njp_stream_skip(FILE *file, long size);
bool sf_njp_stream_i32(FILE *file, int32_t *value);
uint32_t sf_njp_stream_u32(const uint8_t *bytes);
bool sf_njp_stream_header(FILE *file, SfNjpStreamHeader *header);
bool sf_njp_stream_part(
  FILE *file, bool shadow, int32_t *bits, uint16_t *width,
  uint16_t *height, uint16_t *stride, bool *compressed,
  uint32_t *decoded_size);
bool sf_njp_stream_skip_part(FILE *file, bool shadow);

#endif
