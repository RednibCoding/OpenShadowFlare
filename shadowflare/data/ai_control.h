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

#ifndef SHADOWFLARE_DATA_AI_CONTROL_H
#define SHADOWFLARE_DATA_AI_CONTROL_H

#include "core/arena.h"
#include "data/mct.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_AI_CONTROL_EVENT_COUNT 18u
#define SF_AI_ACTION_PARAMETER_COUNT 9u
#define SF_AI_ACTION_CONDITION_COUNT 6u
#define SF_AI_CONTROL_NAME_CAPACITY 33u

typedef struct SfAiAction {
  int32_t parameters[SF_AI_ACTION_PARAMETER_COUNT];
  int32_t conditions[SF_AI_ACTION_CONDITION_COUNT];
  int32_t action_number;
} SfAiAction;

typedef struct SfAiEvent {
  uint16_t first_action;
  uint16_t action_count;
} SfAiEvent;

typedef struct SfAiControl {
  char name[SF_AI_CONTROL_NAME_CAPACITY];
  SfAiEvent events[SF_AI_CONTROL_EVENT_COUNT];
  int32_t walk_point_speed;
} SfAiControl;

typedef struct SfAiControlCatalog {
  SfAiControl *controls;
  SfAiAction *actions;
  uint16_t control_count;
  uint16_t action_count;
} SfAiControlCatalog;

bool sf_ai_control_catalog_load(
  const char *path, const SfMctScenario *scenario,
  SfArena *arena, SfAiControlCatalog *catalog);
const SfAiControl *sf_ai_control_find(
  const SfAiControlCatalog *catalog, const char *name);
const SfAiAction *sf_ai_control_action(
  const SfAiControlCatalog *catalog, const SfAiControl *control,
  uint8_t event, uint16_t action_index);

#endif
