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

#ifndef SHADOWFLARE_GAME_ENEMY_AI_SELECTION_H
#define SHADOWFLARE_GAME_ENEMY_AI_SELECTION_H

#include "data/ai_control.h"

#include <stdint.h>

typedef struct SfEnemyAiTargetDistances {
  int32_t player;
  int32_t companion;
} SfEnemyAiTargetDistances;

const SfAiAction *sf_enemy_ai_select(
  const SfAiControlCatalog *catalog, const SfAiControl *control,
  int32_t event_number, int32_t current_life, int32_t maximum_life,
  SfEnemyAiTargetDistances targets, uint32_t *random_state);

#endif
