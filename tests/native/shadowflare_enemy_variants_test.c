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

#include "assets/scenario_enemy_assets.h"
#include "core/arena.h"
#include "core/memory_budget.h"
#include "data/ai_control.h"
#include "data/mct.h"
#include "game/scenario_enemy_controller.h"

#include <stdio.h>
#include <string.h>

static uint8_t sf_enemy_variant_fixture_memory[1024u * 1024u];
static uint8_t sf_enemy_variant_video_memory[SF_VIDEO_MEMORY_LIMIT_BYTES];

static int sf_test_direct_variant(uint8_t variant) {
  SfMctEnemy definition;
  SfAiAction attack;
  SfAiControl control;
  SfAiControlCatalog catalog;
  SfScenarioEnemy enemy;
  SfCollisionQuery collision;
  SfScenarioEnemyControllerContext context;
  SfCafSelectedAnimation animations[8];
  SfCafCell cells[2];
  SfEnemyAttackRequest request = {-1, -1};
  uint32_t random_state = 1u;
  uint8_t direction;
  const int32_t action = (int32_t) variant + 2;
  const uint16_t chart = (uint16_t) variant + 4u;
  memset(&definition, 0, sizeof(definition));
  memset(&attack, 0, sizeof(attack));
  memset(&control, 0, sizeof(control));
  memset(&catalog, 0, sizeof(catalog));
  memset(&enemy, 0, sizeof(enemy));
  memset(&collision, 0, sizeof(collision));
  memset(&context, 0, sizeof(context));
  memset(animations, 0, sizeof(animations));
  memset(cells, 0, sizeof(cells));
  definition.resource_id = 7;
  definition.post_ai_values[3u + variant] = 300;
  definition.post_ai_values[41u + variant] = variant;
  definition.post_ai_values[47u + variant] = 4;
  attack.action_number = action;
  enemy.definition = &definition;
  enemy.control = &control;
  enemy.selected_action = &attack;
  enemy.position = (SfWorldPoint) {100, 100};
  enemy.previous_position = enemy.position;
  enemy.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  enemy.state[SF_SCENARIO_VISIBLE] = 1;
  enemy.current_life = 40;
  enemy.maximum_life = 40;
  enemy.event_number = -1;
  enemy.current_action = -1;
  enemy.presentation_action = 7u;
  enemy.presentation_previous_frame = -1;
  enemy.direct_attack_chart = UINT16_MAX;
  cells[0].status = 0x40;
  for (direction = 0u; direction < 8u; ++direction) {
    animations[direction].parts[0].cells = cells;
    animations[direction].part_count = 1u;
    animations[direction].frame_count = 2u;
  }
  catalog.controls = &control;
  catalog.actions = &attack;
  catalog.control_count = 1u;
  catalog.action_count = 1u;
  context.catalog = &catalog;
  context.collision = &collision;
  context.random_state = &random_state;
  context.attack_request = &request;
  context.player.valid = true;
  context.player.position = (SfWorldPoint) {180, 100};
  context.player.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (request.resource_id != 7 || request.chart != chart ||
      enemy.presentation_action != 7u) {
    fprintf(stderr, "Direct variant %u requested the wrong artwork\n", variant);
    return 1;
  }
  enemy.direct_attack_animations = animations;
  enemy.direct_attack_chart = chart;
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.current_action != action ||
      enemy.presentation_action != variant + 1u ||
      enemy.animation_chart != chart || !enemy.direct_impact_pending) {
    fprintf(stderr, "Direct variant %u did not enter its CAF marker\n", variant);
    return 1;
  }
  sf_scenario_enemy_controller_update(&enemy, &context);
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.presentation_action != 7u || enemy.event_number != action) {
    fprintf(stderr, "Direct variant %u published the wrong event\n", variant);
    return 1;
  }
  return 0;
}

static int sf_test_retail_action_three(void) {
#if defined(OPENSHADOWFLARE_SOURCE_DIR)
  SfArena arena;
  SfMctScenario scenario;
  SfAiControlCatalog catalog;
  const SfMctEnemy *authored_enemy = NULL;
  char path[1024];
  uint16_t enemy;
  uint16_t action;
  bool found = false;
  sf_arena_init(
    &arena, sf_enemy_variant_fixture_memory,
    sizeof(sf_enemy_variant_fixture_memory));
  (void) snprintf(
    path, sizeof(path),
    "%s/tmp/ShadowFlare/Scenario/04060004/Scenario.Mct",
    OPENSHADOWFLARE_SOURCE_DIR);
  if (!sf_mct_load(path, &arena, &scenario)) {
    FILE *fixture = fopen(path, "rb");
    if (!fixture) return 0;
    fclose(fixture);
    fprintf(stderr, "The retail direct-variant scenario did not load\n");
    return 1;
  }
  (void) snprintf(
    path, sizeof(path),
    "%s/tmp/ShadowFlare/System/Game/Parameter/Control.aid",
    OPENSHADOWFLARE_SOURCE_DIR);
  if (!sf_ai_control_catalog_load(path, &scenario, &arena, &catalog)) {
    fprintf(stderr, "The retail direct-variant controls did not load\n");
    return 1;
  }
  for (action = 0u; action < catalog.action_count; ++action) {
    if (catalog.actions[action].action_number == 3) {
      found = true;
      break;
    }
  }
  if (!found) {
    fprintf(stderr, "The shipped action-three fixture disappeared\n");
    return 1;
  }
  for (enemy = 0u; enemy < scenario.enemy_count && !authored_enemy; ++enemy) {
    const SfMctEnemy *candidate = &scenario.enemies[enemy];
    const SfAiControl *control = sf_ai_control_find(
      &catalog, candidate->ai_control_name);
    uint8_t event;
    if (!control || candidate->resource_id < 0) continue;
    for (event = 0u; event < SF_AI_CONTROL_EVENT_COUNT; ++event) {
      uint16_t index;
      for (index = 0u; index < control->events[event].action_count; ++index) {
        const SfAiAction *candidate_action = sf_ai_control_action(
          &catalog, control, event, index);
        if (candidate_action && candidate_action->action_number == 3) {
          authored_enemy = candidate;
          break;
        }
      }
      if (authored_enemy) break;
    }
  }
  if (authored_enemy) {
    SfArena video_arena;
    SfScenarioEnemyAttackAssets attack_assets;
    const uint16_t chart = (uint16_t) (
      authored_enemy->post_ai_values[42] + 4);
    sf_arena_init(
      &video_arena, sf_enemy_variant_video_memory,
      sizeof(sf_enemy_variant_video_memory));
    if (!sf_arena_push(
          &video_arena, SF_FRAMEBUFFER_BYTES, sizeof(uint16_t)) ||
        !sf_scenario_enemy_attack_assets_load(
          &attack_assets, OPENSHADOWFLARE_SOURCE_DIR "/tmp/ShadowFlare",
          &scenario, authored_enemy->resource_id, chart, &video_arena) ||
        attack_assets.chart != chart) {
      fprintf(stderr, "The shipped action-three artwork did not stream\n");
      return 1;
    }
  } else {
    fprintf(stderr, "No authored enemy owns the action-three fixture\n");
    return 1;
  }
#endif
  return 0;
}

int main(void) {
  return sf_test_direct_variant(1u) || sf_test_direct_variant(2u) ||
    sf_test_retail_action_three();
}
