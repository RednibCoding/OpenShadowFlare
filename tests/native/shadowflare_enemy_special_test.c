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

int main(void) {
  return sf_test_exact_request() || sf_test_world_dispatch();
}
