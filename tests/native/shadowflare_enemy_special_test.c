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

#include "game/combat_effect_request.h"
#include "game/enemy_direct_impact.h"
#include "game/generic_effect_actor.h"
#include "game/world.h"
#include "game/world_enemy_combat.h"

#include <stdio.h>
#include <string.h>

static void sf_test_special_fixture(
    SfMctEnemy *definition, SfScenarioEnemy *enemy) {
  memset(definition, 0, sizeof(*definition));
  memset(enemy, 0, sizeof(*enemy));
  definition->id = 12;
  definition->post_ai_values[1] = 50;
  definition->post_ai_values[7] = 190;
  definition->post_ai_values[21] = 5;
  definition->post_ai_values[22] = 71;
  definition->post_ai_values[23] = 61;
  definition->post_ai_values[24] = 211;
  definition->post_ai_values[25] = 1;
  enemy->definition = definition;
  enemy->presentation_action = 2u;
  enemy->presentation_direction = (SfWorldPoint) {30, -40};
  enemy->direct_impact_pending = true;
}

static int sf_test_exact_request(void) {
  SfMctEnemy definition;
  SfScenarioEnemy enemy;
  SfScenarioEnemyControllerContext context;
  SfEnemyDirectImpactResult result;
  SfCombatEffectRequestQueue queue;
  uint32_t random_state = 1u;
  uint8_t request;
  sf_test_special_fixture(&definition, &enemy);
  memset(&context, 0, sizeof(context));
  result = sf_enemy_direct_impact_resolve(
    &enemy, &context, 1, &random_state);
  if (!result.valid || !result.special_effect || result.apply_damage ||
      !result.effect_request.valid || !result.effect_request.has_packet ||
      result.effect_request.effect_number != 5 ||
      result.effect_request.owner_kind != 4 ||
      result.effect_request.source_character_number != 14000012 ||
      result.effect_request.target_kind != 19 ||
      result.effect_request.target_identifier != -1 ||
      result.effect_request.direction_vector.x != 30 ||
      result.effect_request.direction_vector.y != -40 ||
      result.effect_request.constructor_value_6 != 61 ||
      result.effect_request.constructor_value_7 != 71 ||
      result.effect_request.constructor_values_16_to_22[
        SF_COMBAT_EFFECT_CONSTRUCTOR_21] != 211 ||
      result.effect_request.packet.words[3] != 1 ||
      result.effect_request.packet.words[34] != 21009 ||
      result.effect_request.packet.words[38] != 0 ||
      result.effect_request.packet.words[74] != 21013 ||
      random_state != 3357800067u) {
    fprintf(stderr, "The direct special request lost retail arguments\n");
    return 1;
  }
  sf_combat_effect_requests_reset(&queue);
  for (request = 0u; request < SF_COMBAT_EFFECT_REQUEST_LIMIT; ++request) {
    if (!sf_combat_effect_requests_push(&queue, &result.effect_request))
      return 1;
  }
  if (sf_combat_effect_requests_push(&queue, &result.effect_request) ||
      !queue.overflowed || queue.count != SF_COMBAT_EFFECT_REQUEST_LIMIT) {
    fprintf(stderr, "The fixed effect-request queue did not saturate safely\n");
    return 1;
  }
  return 0;
}

static int sf_test_world_dispatch(void) {
  SfWorldState world;
  SfMctEnemy definition;
  SfScenarioEnemy enemy;
  SfScenarioEnemyControllerContext context;
  sf_world_state_init(&world, 1, 0, 1u);
  sf_test_special_fixture(&definition, &enemy);
  memset(&context, 0, sizeof(context));
  context.random_state = &world.random_state;
  if (!sf_world_enemy_combat_apply_direct(&world, &enemy, &context) ||
      world.combat_effect_requests.count != 1u ||
      world.combat_effect_requests.requests[0].effect_number != 5 ||
      world.combat_effect_requests.requests[0].direction_vector.x != 30 ||
      world.combat_effect_requests.requests[0].direction_vector.y != -40) {
    fprintf(stderr, "The live special marker did not publish its request\n");
    return 1;
  }
  return 0;
}

static int sf_test_generic_actor_descriptors(void) {
  static const int32_t effects[6] = {0, 1, 4, 5, 6, 7};
  static const int32_t resources[6] = {0, 1, 0, 0, 4, 0};
  SfCombatEffectRequest request;
  SfGenericEffectActor actor;
  uint8_t index;
  memset(&request, 0, sizeof(request));
  request.valid = true;
  request.has_packet = true;
  request.owner_kind = 4;
  request.source_character_number = 14000012;
  request.target_kind = 19;
  request.target_identifier = -1;
  request.direction_vector = (SfWorldPoint) {30, -40};
  request.constructor_value_6 = 61;
  request.constructor_value_7 = 71;
  request.packet_kind = 8;
  request.constructor_values_16_to_22[
    SF_COMBAT_EFFECT_CONSTRUCTOR_21] = 211;
  for (index = 0u; index < 6u; ++index) {
    request.effect_number = effects[index];
    if (!sf_generic_effect_actor_build(
          &request, (SfWorldPoint) {1000, 2000}, &actor) ||
        actor.resource_id != resources[index] ||
        actor.position.x != 1126 || actor.position.y != 1832 ||
        actor.animation_direction != (effects[index] == 1 ? 8 : 2) ||
        actor.judgement.left != (effects[index] == 6 ? -160 : -30) ||
        actor.judgement.right != (effects[index] == 6 ? 159 : 30) ||
        actor.contact_effect_number !=
          (effects[index] == 6 ? 21023 : -1) ||
        actor.target_audio_sample != 20u || !actor.has_packet ||
        !actor.expire_on_target ||
        !actor.expire_on_environment_collision) {
      fprintf(stderr, "Raw effect %d lost its retail actor descriptor\n",
        effects[index]);
      return 1;
    }
  }
  request.effect_number = 10001;
  if (sf_generic_effect_actor_build(
        &request, (SfWorldPoint) {1000, 2000}, &actor)) {
    fprintf(stderr, "A controller effect entered the generic actor owner\n");
    return 1;
  }
  return 0;
}

int main(void) {
  return sf_test_exact_request() || sf_test_world_dispatch() ||
    sf_test_generic_actor_descriptors();
}
