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

#include <stdbool.h>
#include <stdint.h>

#define SF_CAF_FRAME_LIMIT 64u

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

bool sf_caf_load_first_chart_direction(
  const char *path, uint8_t direction, SfCafSequence *output);

#endif
