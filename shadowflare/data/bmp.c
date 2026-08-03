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

#include "data/bmp.h"

#include <stdio.h>
#include <string.h>

static uint16_t sf_bmp_u16(const uint8_t *bytes) {
  return (uint16_t) (bytes[0] | ((uint16_t) bytes[1] << 8u));
}

static uint32_t sf_bmp_u32(const uint8_t *bytes) {
  return (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
}

bool sf_bmp_load_rgb555(
    const char *path, uint16_t *pixels, size_t pixel_capacity,
    void *row_scratch, size_t row_scratch_size, SfRgb555Image *image) {
  uint8_t header[54];
  uint8_t *row_bytes = (uint8_t *) row_scratch;
  uint32_t data_offset;
  int32_t width;
  int32_t height;
  uint32_t absolute_height;
  size_t row_size;
  uint32_t row;
  FILE *file;
  bool success = false;
  if (!path || !pixels || !row_scratch || !image) return false;
  memset(image, 0, sizeof(*image));
  file = fopen(path, "rb");
  if (!file) return false;
  if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
      header[0] != 'B' || header[1] != 'M' || sf_bmp_u32(header + 14u) < 40u ||
      sf_bmp_u16(header + 26u) != 1u || sf_bmp_u16(header + 28u) != 24u ||
      sf_bmp_u32(header + 30u) != 0u) goto done;
  data_offset = sf_bmp_u32(header + 10u);
  width = (int32_t) sf_bmp_u32(header + 18u);
  height = (int32_t) sf_bmp_u32(header + 22u);
  if (width <= 0 || width > UINT16_MAX || height == 0 ||
      height == INT32_MIN) goto done;
  absolute_height = (uint32_t) (height < 0 ? -height : height);
  if (absolute_height > UINT16_MAX ||
      (size_t) width > pixel_capacity / absolute_height) goto done;
  row_size = ((size_t) width * 3u + 3u) & ~(size_t) 3u;
  if (row_size > row_scratch_size || data_offset > UINT32_C(0x7fffffff) ||
      fseek(file, (long) data_offset, SEEK_SET) != 0) goto done;
  for (row = 0u; row < absolute_height; ++row) {
    const uint32_t destination_row = height > 0
      ? absolute_height - row - 1u : row;
    uint16_t *destination = pixels + (size_t) destination_row * width;
    int32_t column;
    if (fread(row_bytes, 1u, row_size, file) != row_size) goto done;
    for (column = 0; column < width; ++column) {
      const uint8_t *source = row_bytes + (size_t) column * 3u;
      destination[column] = sf_rgb555(
        (uint8_t) (source[2] >> 3u),
        (uint8_t) (source[1] >> 3u),
        (uint8_t) (source[0] >> 3u));
    }
  }
  image->pixels = pixels;
  image->width = (uint16_t) width;
  image->height = (uint16_t) absolute_height;
  image->stride = (uint16_t) width;
  success = true;
done:
  fclose(file);
  return success;
}
