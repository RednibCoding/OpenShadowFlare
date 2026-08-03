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

#include "data/voc.h"

#include <stdio.h>
#include <string.h>

static bool sf_voc_read(FILE *file, void *destination, size_t size) {
  return size == 0u || fread(destination, 1u, size, file) == size;
}

static bool sf_voc_skip(FILE *file, long bytes) {
  return bytes >= 0 && fseek(file, bytes, SEEK_CUR) == 0;
}

static bool sf_voc_u16(FILE *file, uint16_t *value) {
  uint8_t bytes[2];
  if (!sf_voc_read(file, bytes, sizeof(bytes))) return false;
  *value = (uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8u);
  return true;
}

static bool sf_voc_u32(FILE *file, uint32_t *value) {
  uint8_t bytes[4];
  if (!sf_voc_read(file, bytes, sizeof(bytes))) return false;
  *value = (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
  return true;
}

static int sf_voc_wanted(
    uint32_t index, const uint16_t *indices, uint8_t count) {
  uint8_t selected;
  for (selected = 0u; selected < count; ++selected) {
    if (indices[selected] == index) return selected;
  }
  return -1;
}

bool sf_voc_load_u8_mono_samples(
    const char *path, const uint16_t *indices, uint8_t count,
    SfArena *arena, SfPcmU8 *output) {
  FILE *file;
  char header[16];
  uint32_t sample_count;
  uint32_t ignored;
  uint32_t index;
  uint8_t loaded = 0u;
  size_t mark;
  if (!path || !indices || count == 0u || count > SF_VOC_SELECTED_LIMIT ||
      !arena || !output) return false;
  mark = sf_arena_mark(arena);
  memset(output, 0, (size_t) count * sizeof(*output));
  file = fopen(path, "rb");
  if (!file) return false;
  if (!sf_voc_read(file, header, sizeof(header)) ||
      memcmp(header, "VoiceData  V003", 15u) != 0 ||
      !sf_voc_u32(file, &sample_count) || !sf_voc_u32(file, &ignored))
    goto failed;
  for (index = 0u; index < sample_count; ++index) {
    uint32_t flags;
    uint16_t format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t average_rate;
    uint16_t frame_stride;
    uint16_t bits;
    uint16_t extra;
    uint32_t pcm_size;
    const int selected = sf_voc_wanted(index, indices, count);
    if (!sf_voc_u32(file, &flags) || !sf_voc_skip(file, 256)) goto failed;
    if ((flags & 1u) != 0u) {
      if (!sf_voc_skip(file, 256) || selected >= 0) goto failed;
      continue;
    }
    if (!sf_voc_skip(file, 256) || !sf_voc_u16(file, &format) ||
        !sf_voc_u16(file, &channels) || !sf_voc_u32(file, &sample_rate) ||
        !sf_voc_u32(file, &average_rate) ||
        !sf_voc_u16(file, &frame_stride) || !sf_voc_u16(file, &bits) ||
        !sf_voc_u16(file, &extra) || !sf_voc_u32(file, &pcm_size))
      goto failed;
    (void) average_rate;
    (void) extra;
    if (selected >= 0) {
      uint8_t *samples;
      if (format != 1u || channels != 1u || bits != 8u ||
          frame_stride != 1u || sample_rate == 0u) goto failed;
      samples = (uint8_t *) sf_arena_push(arena, pcm_size, 1u);
      if (!samples || !sf_voc_read(file, samples, pcm_size)) goto failed;
      output[selected].samples = samples;
      output[selected].frame_count = pcm_size;
      output[selected].sample_rate = sample_rate;
      ++loaded;
    } else if (pcm_size > UINT32_C(0x7fffffff) ||
               !sf_voc_skip(file, (long) pcm_size)) {
      goto failed;
    }
  }
  fclose(file);
  if (loaded == count) return true;
  (void) sf_arena_rewind(arena, mark);
  return false;
failed:
  fclose(file);
  (void) sf_arena_rewind(arena, mark);
  return false;
}
