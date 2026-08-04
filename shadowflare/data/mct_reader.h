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

#ifndef SHADOWFLARE_DATA_MCT_READER_H
#define SHADOWFLARE_DATA_MCT_READER_H

#include "data/mct.h"

#include <stdio.h>

#define SF_MCT_ENTITY_LIMIT 4096u

typedef struct SfMctCommonEntity {
  char name[SF_MCT_PERSON_NAME_CAPACITY];
  int32_t id;
  int32_t resource_id;
  uint32_t name_color;
  int32_t label_height;
  int32_t world_x;
  int32_t world_y;
  int32_t judgement_left;
  int32_t judgement_top;
  int32_t judgement_right;
  int32_t judgement_bottom;
  int32_t direction;
  int32_t initial_state[SF_MCT_ENTITY_STATE_COUNT];
  int16_t red_strength[SF_MCT_PERSON_PART_LIMIT];
  int16_t green_strength[SF_MCT_PERSON_PART_LIMIT];
  int16_t blue_strength[SF_MCT_PERSON_PART_LIMIT];
  uint8_t part_visibility[SF_MCT_PERSON_PART_LIMIT];
  uint8_t custom_part_count;
  bool custom_parts;
} SfMctCommonEntity;

bool sf_mct_reader_read(FILE *file, void *output, size_t size);
bool sf_mct_reader_skip(FILE *file, uint64_t size);
bool sf_mct_reader_u32(FILE *file, uint32_t *value);
bool sf_mct_reader_i32(FILE *file, int32_t *value);
bool sf_mct_reader_i16(FILE *file, int16_t *value);
bool sf_mct_reader_skip_values(FILE *file, uint32_t size);
bool sf_mct_reader_string(
  FILE *file, uint32_t size, char *output, size_t capacity);
bool sf_mct_reader_common(FILE *file, SfMctCommonEntity *entity);

#endif
