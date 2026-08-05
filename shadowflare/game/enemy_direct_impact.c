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

#include "game/enemy_direct_impact.h"

#include "core/retail_random.h"
#include "game/movement.h"

#include <string.h>

enum {
  SF_ENEMY_DIRECT_DAMAGE = 0,
  SF_ENEMY_DIRECT_RANGE = 3,
  SF_ENEMY_DIRECT_HIT_RATE = 6,
  SF_ENEMY_DIRECT_SPECIAL_EFFECT = 21,
  SF_ENEMY_DIRECT_SPECIAL_VARIANT = 25,
  SF_ENEMY_DIRECT_REACTION_CHANCE = 29,
  SF_ENEMY_DIRECT_REACTION_DURATION = 32,
  SF_ENEMY_DIRECT_REACTION_MOTION = 35
};

static int32_t sf_enemy_direct_hit_chance(
    int32_t attack, int32_t defense) {
  const int32_t chance = attack - defense;
  return chance < 20 ? 20 : chance > 98 ? 98 : chance;
}

static bool sf_enemy_direct_candidate(
    const SfScenarioEnemy *enemy, SfEnemyControllerTarget target,
    int32_t maximum, bool companion) {
  const int32_t distance = target.valid ? sf_movement_bounds_distance(
    enemy->position, enemy->judgement,
    target.position, target.judgement) : -1;
  const uint8_t direction = target.valid
    ? sf_movement_direction(enemy->position, target.position) : 0u;
  const uint8_t difference = (uint8_t) ((enemy->direction - direction + 8) % 8);
  return distance >= 0 && distance <= maximum &&
    (companion ? direction == enemy->direction
               : difference == 0u || difference == 1u || difference == 7u);
}

static SfEnemyImpactTargetKind sf_enemy_direct_target(
    const SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context,
    int32_t maximum, const SfEnemyControllerTarget **target) {
  *target = NULL;
  if (sf_enemy_direct_candidate(enemy, context->player, maximum, false)) {
    *target = &context->player;
    return SF_ENEMY_IMPACT_TARGET_PLAYER;
  }
  if (sf_enemy_direct_candidate(enemy, context->companion, maximum, true)) {
    *target = &context->companion;
    return SF_ENEMY_IMPACT_TARGET_COMPANION;
  }
  return SF_ENEMY_IMPACT_TARGET_NONE;
}

static bool sf_enemy_direct_special(
    const SfMctEnemy *definition, int32_t variant) {
  return definition->post_ai_values[SF_ENEMY_DIRECT_SPECIAL_EFFECT] != -1 &&
    definition->post_ai_values[SF_ENEMY_DIRECT_SPECIAL_VARIANT] == variant;
}

static void sf_enemy_direct_packet(
    SfCombatPacket *packet, const SfScenarioEnemy *enemy,
    int32_t variant, bool special, uint32_t *random_state) {
  const SfMctEnemy *definition = enemy->definition;
  memset(packet, 0, sizeof(*packet));
  packet->words[0] = 2;
  packet->words[1] = 0;
  packet->words[2] = sf_scenario_enemy_character_number(enemy);
  packet->words[4] = definition->post_ai_values[
    SF_ENEMY_DIRECT_DAMAGE + variant];
  packet->words[31] = definition->pre_ai_values[7];
  packet->words[32] = definition->pre_ai_values[6];
  packet->words[34] = sf_retail_random_next(random_state) % 4 + 21000;
  packet->words[35] = 8;
  packet->words[36] = definition->post_ai_values[
    SF_ENEMY_DIRECT_HIT_RATE + variant];
  packet->words[38] = special ? 0 : 1;
  packet->words[40] = definition->post_ai_values[
    SF_ENEMY_DIRECT_REACTION_MOTION + variant];
  packet->words[41] = definition->post_ai_values[
    SF_ENEMY_DIRECT_REACTION_CHANCE + variant];
  packet->words[43] = definition->post_ai_values[
    SF_ENEMY_DIRECT_REACTION_DURATION + variant];
  packet->words[72] = 1;
  packet->words[73] = -1;
  packet->words[74] = -1;
  packet->words[75] = 8;
}

static void sf_enemy_direct_special_packet(
    SfCombatPacket *packet, const SfMctEnemy *definition,
    uint32_t *random_state) {
  const int32_t draw = sf_retail_random_next(random_state);
  switch (definition->post_ai_values[SF_ENEMY_DIRECT_SPECIAL_EFFECT]) {
    case 0:
      packet->words[34] = draw % 3 + 21007;
      break;
    case 4:
      packet->words[34] = draw % 3 + 21007;
      packet->words[74] = 20000;
      break;
    case 5:
      packet->words[3] = 1;
      packet->words[34] = draw % 3 + 21007;
      packet->words[74] = 21013;
      break;
    case 7:
      packet->words[3] = 2;
      packet->words[34] = draw % 3 + 21007;
      break;
    default:
      packet->words[34] = draw % 4 + 21000;
      break;
  }
}

static void sf_enemy_direct_special_request(
    SfCombatEffectRequest *request, const SfCombatPacket *packet,
    const SfScenarioEnemy *enemy) {
  const SfMctEnemy *definition = enemy->definition;
  memset(request, 0, sizeof(*request));
  request->packet = *packet;
  request->direction_vector = enemy->presentation_direction;
  request->effect_number = definition->post_ai_values[
    SF_ENEMY_DIRECT_SPECIAL_EFFECT];
  request->owner_kind = 4;
  request->source_character_number = sf_scenario_enemy_character_number(enemy);
  request->target_kind = 19;
  request->target_identifier = -1;
  request->constructor_value_6 = definition->post_ai_values[23];
  request->constructor_value_7 = definition->post_ai_values[22];
  request->packet_kind = 8;
  request->instance_identifier = -1;
  request->constructor_values_16_to_22[
    SF_COMBAT_EFFECT_CONSTRUCTOR_21] =
    definition->post_ai_values[24];
  request->valid = true;
  request->has_packet = true;
}

SfEnemyDirectImpactResult sf_enemy_direct_impact_resolve(
    const SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context,
    int32_t variant, uint32_t *random_state) {
  SfEnemyDirectImpactResult result;
  const SfEnemyControllerTarget *target = NULL;
  memset(&result, 0, sizeof(result));
  result.post_hit_event = -1;
  if (!enemy || !enemy->definition || !context || !random_state ||
      variant < 0 || variant >= 3) return result;
  result.valid = true;
  result.special_effect = sf_enemy_direct_special(enemy->definition, variant);
  sf_enemy_direct_packet(
    &result.packet, enemy, variant, result.special_effect, random_state);
  if (result.special_effect) {
    sf_enemy_direct_special_packet(
      &result.packet, enemy->definition, random_state);
    sf_enemy_direct_special_request(
      &result.effect_request, &result.packet, enemy);
    return result;
  }
  result.target = sf_enemy_direct_target(
    enemy, context, enemy->definition->post_ai_values[
      SF_ENEMY_DIRECT_RANGE + variant], &target);
  if (!target) return result;
  result.hit_chance = sf_enemy_direct_hit_chance(
    enemy->definition->post_ai_values[
      SF_ENEMY_DIRECT_HIT_RATE + variant], target->combat_defense);
  result.hit_roll = sf_retail_random_next(random_state) % 100;
  if (result.hit_roll >= result.hit_chance) {
    result.show_miss = true;
    return result;
  }
  result.apply_damage = true;
  result.damage_origin = enemy->position;
  result.post_hit_audio_sample = 6u;
  if (enemy->event_number == -1) result.post_hit_event = 17;
  return result;
}
