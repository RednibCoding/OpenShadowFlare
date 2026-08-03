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

#include "data/caf.h"

#include <stdio.h>
#include <string.h>

static bool sf_caf_read(FILE *file, void *destination, size_t size) {
  return fread(destination, 1u, size, file) == size;
}

static bool sf_caf_i16(FILE *file, int16_t *value) {
  uint8_t bytes[2];
  if (!sf_caf_read(file, bytes, sizeof(bytes))) return false;
  *value = (int16_t) ((uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8u));
  return true;
}

static bool sf_caf_i32(FILE *file, int32_t *value) {
  uint8_t bytes[4];
  uint32_t result;
  if (!sf_caf_read(file, bytes, sizeof(bytes))) return false;
  result = (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
  *value = (int32_t) result;
  return true;
}

bool sf_caf_load_first_chart_direction(
    const char *path, uint8_t wanted_direction, SfCafSequence *output) {
  FILE *file;
  char header[16];
  int version;
  int32_t chart_count;
  int16_t chart_status;
  uint8_t direction;
  bool success = false;
  if (!path || !output || wanted_direction >= 9u) return false;
  file = fopen(path, "rb");
  if (!file) return false;
  memset(output, 0, sizeof(*output));
  if (!sf_caf_read(file, header, sizeof(header)) ||
      memcmp(header, "CHRAnimation", 12u) != 0 ||
      header[12] < '0' || header[12] > '9' ||
      header[13] < '0' || header[13] > '9' ||
      header[14] < '0' || header[14] > '9') goto done;
  version = (header[12] - '0') * 100 + (header[13] - '0') * 10 +
    header[14] - '0';
  if (version < 0 || version > 3 || !sf_caf_i32(file, &chart_count) ||
      chart_count < 1 || !sf_caf_i16(file, &chart_status)) goto done;
  output->looping = (chart_status & 1) != 0;
  for (direction = 0u; direction < 9u; ++direction) {
    int32_t part_count;
    int16_t frame_count;
    int32_t part;
    if (!sf_caf_i32(file, &part_count) || !sf_caf_i16(file, &frame_count) ||
        part_count < 0 || frame_count < 0) goto done;
    if (direction == wanted_direction &&
        (part_count != 1 || frame_count > (int16_t) SF_CAF_FRAME_LIMIT))
      goto done;
    for (part = 0; part < part_count; ++part) {
      int32_t cell_count;
      int32_t cell;
      if (!sf_caf_i32(file, &cell_count) || cell_count < 0) goto done;
      if (direction == wanted_direction && cell_count != frame_count) goto done;
      for (cell = 0; cell < cell_count; ++cell) {
        int16_t status;
        int16_t transparency;
        int32_t pattern;
        int16_t priority;
        int16_t old_pattern;
        if (!sf_caf_i16(file, &status) ||
            !sf_caf_i16(file, &transparency)) goto done;
        if (version < 2) {
          if (!sf_caf_i16(file, &old_pattern)) goto done;
          pattern = old_pattern;
        } else if (!sf_caf_i32(file, &pattern)) goto done;
        if (!sf_caf_i16(file, &priority)) goto done;
        if (direction == wanted_direction) {
          SfCafFrame *frame = &output->frames[cell];
          if (pattern < INT16_MIN || pattern > INT16_MAX || priority != 0)
            goto done;
          frame->pattern = (int16_t) pattern;
          frame->opacity = transparency < 0 ? 0u :
            transparency > 1000 ? 1000u : (uint16_t) transparency;
          frame->additive = (status & 0x10) != 0;
        }
      }
    }
    if (direction == wanted_direction) output->frame_count = (uint8_t) frame_count;
  }
  success = output->frame_count > 0u;
done:
  fclose(file);
  return success;
}

static bool sf_caf_selected_part(
    const uint8_t *parts, uint8_t part_count,
    int32_t source_part, uint8_t *slot) {
  uint8_t index;
  for (index = 0u; index < part_count; ++index) {
    if (parts[index] == source_part) {
      *slot = index;
      return true;
    }
  }
  return false;
}

bool sf_caf_load_selected_chart_direction(
    const char *path, uint16_t wanted_chart, uint8_t wanted_direction,
    const uint8_t *parts, uint8_t part_count,
    SfArena *arena, SfCafSelectedAnimation *output) {
  FILE *file;
  char header[16];
  int version;
  int32_t chart_count;
  int32_t chart;
  size_t mark;
  bool success = false;
  if (!path || !parts || part_count == 0u ||
      part_count > SF_CAF_SELECTED_PART_LIMIT || wanted_direction >= 9u ||
      !arena || !output) return false;
  mark = sf_arena_mark(arena);
  memset(output, 0, sizeof(*output));
  file = fopen(path, "rb");
  if (!file) return false;
  if (!sf_caf_read(file, header, sizeof(header)) ||
      memcmp(header, "CHRAnimation", 12u) != 0 ||
      header[12] < '0' || header[12] > '9' ||
      header[13] < '0' || header[13] > '9' ||
      header[14] < '0' || header[14] > '9') goto done;
  version = (header[12] - '0') * 100 + (header[13] - '0') * 10 +
    header[14] - '0';
  if (version > 3 || !sf_caf_i32(file, &chart_count) ||
      chart_count <= wanted_chart) goto done;
  for (chart = 0; chart < chart_count; ++chart) {
    int16_t status;
    uint8_t direction;
    if (!sf_caf_i16(file, &status)) goto done;
    if (chart == wanted_chart) output->looping = (status & 1) != 0;
    for (direction = 0u; direction < 9u; ++direction) {
      int32_t source_part_count;
      int16_t frame_count;
      int32_t source_part;
      if (!sf_caf_i32(file, &source_part_count) ||
          !sf_caf_i16(file, &frame_count) || source_part_count < 0 ||
          frame_count < 0 || frame_count > (int16_t) SF_CAF_FRAME_LIMIT)
        goto done;
      if (chart == wanted_chart && direction == wanted_direction) {
        uint8_t selected;
        output->frame_count = (uint8_t) frame_count;
        if (source_part_count > UINT8_MAX) goto done;
        output->priority_count = (uint8_t) source_part_count;
        for (selected = 0u; selected < part_count; ++selected) {
          if (parts[selected] >= source_part_count) goto done;
          output->parts[selected].source_index = parts[selected];
          output->parts[selected].cells = (SfCafCell *) sf_arena_push(
            arena, (size_t) frame_count * sizeof(SfCafCell),
            sizeof(int32_t));
          if (!output->parts[selected].cells) goto done;
        }
        output->part_count = part_count;
      }
      for (source_part = 0; source_part < source_part_count; ++source_part) {
        int32_t cell_count;
        int32_t cell;
        uint8_t selected_slot = 0u;
        const bool selected = chart == wanted_chart &&
          direction == wanted_direction &&
          sf_caf_selected_part(
            parts, part_count, source_part, &selected_slot);
        if (!sf_caf_i32(file, &cell_count) || cell_count < 0 ||
            (selected && cell_count != frame_count)) goto done;
        for (cell = 0; cell < cell_count; ++cell) {
          int16_t status;
          int16_t transparency;
          int32_t pattern;
          int16_t priority;
          int16_t old_pattern;
          if (!sf_caf_i16(file, &status) ||
              !sf_caf_i16(file, &transparency)) goto done;
          if (version < 2) {
            if (!sf_caf_i16(file, &old_pattern)) goto done;
            pattern = old_pattern;
          } else if (!sf_caf_i32(file, &pattern)) goto done;
          if (!sf_caf_i16(file, &priority)) goto done;
          if (selected) {
            SfCafCell *output_cell =
              &output->parts[selected_slot].cells[cell];
            output_cell->pattern = pattern;
            output_cell->status = status;
            output_cell->transparency = transparency;
            output_cell->priority = priority;
          }
        }
      }
    }
  }
  if (version != 0 && (!sf_caf_i32(file, &output->palette_mode) ||
      !sf_caf_i32(file, &output->chart_priority_stride))) goto done;
  success = output->part_count == part_count && output->frame_count > 0u;
done:
  fclose(file);
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(output, 0, sizeof(*output));
  }
  return success;
}
