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

typedef struct SfVocFormat {
  uint32_t sample_rate;
  uint32_t pcm_size;
  uint16_t format;
  uint16_t channels;
  uint16_t frame_stride;
  uint16_t bits;
} SfVocFormat;

static bool sf_voc_header(FILE *file, uint32_t *sample_count) {
  char header[16];
  uint32_t ignored;
  return sf_voc_read(file, header, sizeof(header)) &&
    memcmp(header, "VoiceData  V003", 15u) == 0 &&
    sf_voc_u32(file, sample_count) && sf_voc_u32(file, &ignored);
}

static bool sf_voc_format(FILE *file, SfVocFormat *format) {
  uint32_t ignored_rate;
  uint16_t ignored_extra;
  return sf_voc_u16(file, &format->format) &&
    sf_voc_u16(file, &format->channels) &&
    sf_voc_u32(file, &format->sample_rate) &&
    sf_voc_u32(file, &ignored_rate) &&
    sf_voc_u16(file, &format->frame_stride) &&
    sf_voc_u16(file, &format->bits) &&
    sf_voc_u16(file, &ignored_extra) &&
    sf_voc_u32(file, &format->pcm_size);
}

static bool sf_voc_format_supported(const SfVocFormat *format) {
  return format->format == 1u && format->channels == 1u &&
    (format->bits == 8u || format->bits == 16u) &&
    format->frame_stride == format->bits / 8u &&
    format->frame_stride > 0u && format->sample_rate > 0u &&
    format->pcm_size % format->frame_stride == 0u;
}

static bool sf_voc_load_pcm_u8(
    FILE *file, const SfVocFormat *format,
    SfArena *arena, SfPcmU8 *output) {
  const uint32_t frame_count = format->pcm_size / format->frame_stride;
  uint8_t *samples;
  uint32_t frame;
  if (!sf_voc_format_supported(format)) return false;
  samples = (uint8_t *) sf_arena_push(arena, frame_count, 1u);
  if (!samples) return false;
  if (format->bits == 8u) {
    if (!sf_voc_read(file, samples, frame_count)) return false;
  } else {
    for (frame = 0u; frame < frame_count; ++frame) {
      uint8_t pcm[2];
      if (!sf_voc_read(file, pcm, sizeof(pcm))) return false;
      samples[frame] = (uint8_t) (pcm[1] ^ 0x80u);
    }
  }
  output->samples = samples;
  output->frame_count = frame_count;
  output->sample_rate = format->sample_rate;
  return true;
}

static bool sf_voc_resolve_references(
    const char *path, const uint16_t *indices, uint8_t count,
    char references[SF_VOC_SELECTED_LIMIT][256], bool *unresolved,
    SfArena *arena, SfPcmU8 *output, uint8_t *loaded) {
  FILE *file = fopen(path, "rb");
  uint32_t sample_count;
  uint32_t index;
  if (!file || !sf_voc_header(file, &sample_count)) {
    if (file) fclose(file);
    return false;
  }
  for (index = 0u; index < sample_count; ++index) {
    char name[256];
    uint32_t flags;
    SfVocFormat format;
    bool wanted_reference = false;
    uint8_t selected;
    if (!sf_voc_u32(file, &flags) ||
        !sf_voc_read(file, name, sizeof(name))) goto failed;
    if ((flags & 1u) != 0u) {
      if (!sf_voc_skip(file, 256)) goto failed;
      continue;
    }
    if (!sf_voc_skip(file, 256) || !sf_voc_format(file, &format))
      goto failed;
    for (selected = 0u; selected < count; ++selected)
      if (unresolved[selected] &&
          strncmp(references[selected], name, sizeof(name)) == 0)
        wanted_reference = true;
    if (wanted_reference) {
      SfPcmU8 source;
      const int source_slot = sf_voc_wanted(index, indices, count);
      if (source_slot >= 0 && output[source_slot].samples) {
        source = output[source_slot];
        if (format.pcm_size > UINT32_C(0x7fffffff) ||
            !sf_voc_skip(file, (long) format.pcm_size)) goto failed;
      } else {
        if (!sf_voc_load_pcm_u8(file, &format, arena, &source)) goto failed;
      }
      for (selected = 0u; selected < count; ++selected) {
        if (!unresolved[selected] ||
            strncmp(references[selected], name, sizeof(name)) != 0) continue;
        output[selected] = source;
        unresolved[selected] = false;
        ++*loaded;
      }
    } else if (format.pcm_size > UINT32_C(0x7fffffff) ||
               !sf_voc_skip(file, (long) format.pcm_size)) {
      goto failed;
    }
  }
  fclose(file);
  return *loaded == count;
failed:
  fclose(file);
  return false;
}

bool sf_voc_load_u8_mono_samples(
    const char *path, const uint16_t *indices, uint8_t count,
    SfArena *arena, SfPcmU8 *output) {
  FILE *file;
  char references[SF_VOC_SELECTED_LIMIT][256];
  bool unresolved[SF_VOC_SELECTED_LIMIT];
  uint32_t sample_count;
  uint32_t index;
  uint8_t loaded = 0u;
  uint8_t selected_index;
  size_t mark;
  if (!path || !indices || count == 0u || count > SF_VOC_SELECTED_LIMIT ||
      !arena || !output) return false;
  mark = sf_arena_mark(arena);
  memset(output, 0, (size_t) count * sizeof(*output));
  memset(references, 0, sizeof(references));
  memset(unresolved, 0, sizeof(unresolved));
  for (selected_index = 0u; selected_index < count; ++selected_index) {
    uint8_t previous;
    for (previous = 0u; previous < selected_index; ++previous)
      if (indices[previous] == indices[selected_index]) goto failed_without_file;
  }
  file = fopen(path, "rb");
  if (!file) return false;
  if (!sf_voc_header(file, &sample_count)) goto failed;
  for (index = 0u; index < sample_count; ++index) {
    uint32_t flags;
    SfVocFormat format;
    const int selected = sf_voc_wanted(index, indices, count);
    if (!sf_voc_u32(file, &flags) || !sf_voc_skip(file, 256)) goto failed;
    if ((flags & 1u) != 0u) {
      if (selected >= 0) {
        if (!sf_voc_read(
              file, references[selected], sizeof(references[selected])))
          goto failed;
        unresolved[selected] = true;
      } else if (!sf_voc_skip(file, 256)) {
        goto failed;
      }
      continue;
    }
    if (!sf_voc_skip(file, 256) || !sf_voc_format(file, &format))
      goto failed;
    if (selected >= 0) {
      if (!sf_voc_load_pcm_u8(
            file, &format, arena, &output[selected])) goto failed;
      ++loaded;
    } else if (format.pcm_size > UINT32_C(0x7fffffff) ||
               !sf_voc_skip(file, (long) format.pcm_size)) {
      goto failed;
    }
  }
  fclose(file);
  if (loaded == count || sf_voc_resolve_references(
        path, indices, count, references, unresolved,
        arena, output, &loaded)) return true;
  (void) sf_arena_rewind(arena, mark);
  return false;
failed:
  fclose(file);
failed_without_file:
  (void) sf_arena_rewind(arena, mark);
  return false;
}
