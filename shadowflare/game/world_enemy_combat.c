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

#include "game/world_enemy_combat.h"

#include "game/companion_damage.h"
#include "game/enemy_direct_impact.h"
#include "game/player_damage.h"

void sf_world_enemy_combat_targets(
    const SfWorldState *world, SfScenarioEnemyControllerContext *context) {
  int32_t defense = 0;
  if (!world || !context) return;
  context->player.valid = world->player.current_life > 0;
  context->player.position = world->player.position;
  context->player.judgement = world->player.judgement;
  sf_player_combat_defense(
    &world->player, world->ground_items.definitions,
    world->ground_items.definition_count, NULL, &defense);
  context->player.combat_defense = defense;
  context->companion.valid = world->companion.valid &&
    !world->companion.inactive && world->companion.current_life > 0;
  context->companion.position = world->companion.position;
  context->companion.judgement = world->companion.judgement;
  sf_companion_combat_defense(&world->companion, NULL, &defense);
  context->companion.combat_defense = defense;
}

bool sf_world_enemy_combat_apply_direct(
    SfWorldState *world, SfScenarioEnemy *enemy,
    SfScenarioEnemyControllerContext *context) {
  SfEnemyDirectImpactResult impact;
  bool accepted = false;
  if (!world || !enemy || !context || !world->combat_tables ||
      !enemy->direct_impact_pending) return false;
  impact = sf_enemy_direct_impact_resolve(
    enemy, context, 0, &world->random_state);
  if (!impact.valid) return false;
  if (impact.post_hit_event != -1)
    enemy->event_number = impact.post_hit_event;
  if (!impact.apply_damage) return false;
  if (impact.target == SF_ENEMY_IMPACT_TARGET_PLAYER) {
    const SfPlayerDamageResult result = sf_player_receive_damage(
      &world->player, &impact.packet, impact.damage_origin,
      world->ground_items.definitions, world->ground_items.definition_count,
      world->combat_tables, &world->random_state);
    accepted = result.valid && result.accepted;
    if (accepted && result.audio_sample != 0u)
      sf_sound_events_push(&world->sounds, result.audio_sample);
  } else if (impact.target == SF_ENEMY_IMPACT_TARGET_COMPANION) {
    const SfCompanionDamageResult result = sf_companion_receive_damage(
      &world->companion, &impact.packet, impact.damage_origin,
      world->combat_tables, &world->random_state);
    accepted = result.valid && result.accepted;
  }
  if (accepted && impact.post_hit_audio_sample != 0u)
    sf_sound_events_push(&world->sounds, impact.post_hit_audio_sample);
  sf_world_enemy_combat_targets(world, context);
  return accepted;
}
