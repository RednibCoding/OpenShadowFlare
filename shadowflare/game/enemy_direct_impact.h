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

#ifndef SHADOWFLARE_GAME_ENEMY_DIRECT_IMPACT_H
#define SHADOWFLARE_GAME_ENEMY_DIRECT_IMPACT_H

#include "game/combat_packet.h"
#include "game/scenario_enemy_controller.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum SfEnemyImpactTargetKind {
  SF_ENEMY_IMPACT_TARGET_NONE = 0,
  SF_ENEMY_IMPACT_TARGET_PLAYER,
  SF_ENEMY_IMPACT_TARGET_COMPANION
} SfEnemyImpactTargetKind;

typedef struct SfEnemyDirectImpactResult {
  SfCombatPacket packet;
  SfWorldPoint damage_origin;
  int32_t hit_chance;
  int32_t hit_roll;
  int32_t post_hit_event;
  uint16_t post_hit_audio_sample;
  SfEnemyImpactTargetKind target;
  bool valid;
  bool special_effect;
  bool show_miss;
  bool apply_damage;
} SfEnemyDirectImpactResult;

SfEnemyDirectImpactResult sf_enemy_direct_impact_resolve(
  const SfScenarioEnemy *enemy,
  const SfScenarioEnemyControllerContext *context,
  int32_t variant, uint32_t *random_state);

#endif
