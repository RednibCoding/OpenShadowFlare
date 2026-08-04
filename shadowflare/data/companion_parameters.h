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

#ifndef SHADOWFLARE_DATA_COMPANION_PARAMETERS_H
#define SHADOWFLARE_DATA_COMPANION_PARAMETERS_H

#include <stdbool.h>
#include <stdint.h>

#define SF_COMPANION_COUNT 6u
#define SF_COMPANION_NAME_CAPACITY 24u
#define SF_COMPANION_PARAMETER_COUNT 19u

typedef struct SfCompanionProfile {
  char name[SF_COMPANION_NAME_CAPACITY];
  int32_t values[SF_COMPANION_PARAMETER_COUNT];
  int32_t type;
  int32_t level;
  int32_t resource_id;
  int32_t red_strength;
  int32_t green_strength;
  int32_t blue_strength;
} SfCompanionProfile;

bool sf_companion_profile_load(
  const char *path, int32_t type, int32_t level,
  SfCompanionProfile *profile);

#endif
