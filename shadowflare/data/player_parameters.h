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

#ifndef SHADOWFLARE_DATA_PLAYER_PARAMETERS_H
#define SHADOWFLARE_DATA_PLAYER_PARAMETERS_H

#include <stdbool.h>
#include <stdint.h>

#define SF_PLAYER_INITIAL_PARAMETER_COUNT 13u

typedef struct SfPlayerInitialParameters {
  int32_t values[SF_PLAYER_INITIAL_PARAMETER_COUNT];
  int32_t experience_threshold;
} SfPlayerInitialParameters;

bool sf_player_initial_parameters_load(
  const char *path, uint8_t gender,
  SfPlayerInitialParameters *parameters);

#endif
