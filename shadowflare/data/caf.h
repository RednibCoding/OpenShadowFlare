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

#ifndef SHADOWFLARE_DATA_CAF_H
#define SHADOWFLARE_DATA_CAF_H

#include "core/arena.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_CAF_FRAME_LIMIT 64u
#define SF_CAF_SELECTED_FRAME_LIMIT 512u
#define SF_CAF_SELECTED_PART_LIMIT 8u

typedef struct SfCafFrame {
  int16_t pattern;
  uint16_t opacity;
  bool additive;
} SfCafFrame;

typedef struct SfCafSequence {
  SfCafFrame frames[SF_CAF_FRAME_LIMIT];
  uint8_t frame_count;
  bool looping;
} SfCafSequence;

typedef struct SfCafCell {
  int32_t pattern;
  int16_t status;
  int16_t transparency;
  int16_t priority;
} SfCafCell;

typedef struct SfCafSelectedPart {
  SfCafCell *cells;
  uint8_t source_index;
} SfCafSelectedPart;

typedef struct SfCafSelectedAnimation {
  SfCafSelectedPart parts[SF_CAF_SELECTED_PART_LIMIT];
  int32_t palette_mode;
  int32_t chart_priority_stride;
  uint8_t part_count;
  uint8_t priority_count;
  uint16_t frame_count;
  bool looping;
} SfCafSelectedAnimation;

typedef struct SfCafAnimationSelection {
  uint16_t chart;
  uint8_t direction;
} SfCafAnimationSelection;

bool sf_caf_load_first_chart_direction(
  const char *path, uint8_t direction, SfCafSequence *output);
bool sf_caf_chart_direction_part_count(
  const char *path, uint16_t chart, uint8_t direction,
  uint8_t *part_count);
bool sf_caf_load_selected_chart_direction(
  const char *path, uint16_t chart, uint8_t direction,
  const uint8_t *parts, uint8_t part_count,
  SfArena *arena, SfCafSelectedAnimation *output);
bool sf_caf_load_selected_animations(
  const char *path,
  const SfCafAnimationSelection *selections, uint8_t selection_count,
  const uint8_t *parts, uint8_t part_count,
  SfArena *arena, SfCafSelectedAnimation *outputs);

#endif
