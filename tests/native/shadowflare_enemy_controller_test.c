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

#include "game/enemy_ai_selection.h"
#include "game/scenario_enemy_controller.h"

#include <stdio.h>
#include <string.h>

static int test_event_selection(void) {
  SfAiAction actions[2];
  SfAiControl control;
  SfAiControlCatalog catalog;
  SfEnemyAiTargetDistances targets;
  const SfAiAction *selected;
  uint32_t random_state = 1u;
  memset(actions, 0, sizeof(actions));
  memset(&control, 0, sizeof(control));
  memset(&catalog, 0, sizeof(catalog));
  actions[0].action_number = 1;
  actions[0].parameters[2] = 100;
  actions[1].action_number = 2;
  actions[1].parameters[0] = 100;
  actions[1].parameters[2] = 100;
  actions[1].conditions[3] = 1;
  actions[1].conditions[4] = 0;
  actions[1].conditions[5] = 150;
  control.events[0].action_count = 2u;
  catalog.controls = &control;
  catalog.actions = actions;
  catalog.control_count = 1u;
  catalog.action_count = 2u;
  targets.player = 500;
  targets.companion = -1;
  selected = sf_enemy_ai_select(
    &catalog, &control, 0, 40, 40, targets, &random_state);
  if (selected != &actions[0]) {
    fprintf(stderr, "The distant target did not select retail patrol\n");
    return 1;
  }
  random_state = 1u;
  targets.player = 100;
  selected = sf_enemy_ai_select(
    &catalog, &control, 0, 40, 40, targets, &random_state);
  if (selected != &actions[1]) {
    fprintf(stderr, "The close target did not select the priority action\n");
    return 1;
  }
  return 0;
}

static int test_patrol_controller(void) {
  SfMctEnemy definition;
  SfAiAction patrol;
  SfAiControl control;
  SfAiControlCatalog catalog;
  SfScenarioEnemy enemy;
  SfCollisionQuery collision;
  SfScenarioEnemyControllerContext context;
  uint32_t random_state = 1u;
  memset(&definition, 0, sizeof(definition));
  memset(&patrol, 0, sizeof(patrol));
  memset(&control, 0, sizeof(control));
  memset(&catalog, 0, sizeof(catalog));
  memset(&enemy, 0, sizeof(enemy));
  memset(&collision, 0, sizeof(collision));
  memset(&context, 0, sizeof(context));
  definition.post_ai_values[54] = 3000;
  patrol.action_number = 1;
  patrol.parameters[1] = 5;
  patrol.parameters[3] = 15;
  patrol.parameters[4] = 3;
  patrol.parameters[5] = 2;
  enemy.definition = &definition;
  enemy.control = &control;
  enemy.selected_action = &patrol;
  enemy.position = (SfWorldPoint) {100, 100};
  enemy.previous_position = enemy.position;
  enemy.spawn_position = enemy.position;
  enemy.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  enemy.patrol_bounds = (SfObjectBounds) {-10, -10, 10, 10};
  enemy.state[SF_SCENARIO_VISIBLE] = 1;
  enemy.current_life = 40;
  enemy.maximum_life = 40;
  enemy.event_number = -1;
  enemy.current_action = -1;
  enemy.presentation_action = 7u;
  catalog.controls = &control;
  catalog.actions = &patrol;
  catalog.control_count = 1u;
  catalog.action_count = 1u;
  context.catalog = &catalog;
  context.collision = &collision;
  context.random_state = &random_state;
  context.player.valid = true;
  context.player.position = (SfWorldPoint) {200, 200};
  context.player.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.current_action != 1 || enemy.event_number != 12 ||
      enemy.action_counter != 1 || enemy.patrol_counter != 1 ||
      enemy.movement_speed != 45 || enemy.animation_chart != 1u ||
      (enemy.position.x == 100 && enemy.position.y == 100)) {
    fprintf(stderr, "The fixed patrol controller missed retail cadence\n");
    return 1;
  }
  context.player.position = (SfWorldPoint) {10000, 10000};
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.current_action != -1 || enemy.event_number != 0 ||
      enemy.movement_active || enemy.presentation_action != 7u) {
    fprintf(stderr, "An inactive enemy did not return to retail idle\n");
    return 1;
  }
  return 0;
}

static int test_direct_attack_presentation(void) {
  SfMctEnemy definition;
  SfAiAction attack;
  SfAiControl control;
  SfAiControlCatalog catalog;
  SfScenarioEnemy enemy;
  SfCollisionQuery collision;
  SfScenarioEnemyControllerContext context;
  SfCafSelectedAnimation animations[8];
  SfCafCell cells[3];
  uint32_t random_state = 1u;
  SfEnemyAttackRequest resource_request = {-1, -1};
  uint8_t direction;
  memset(&definition, 0, sizeof(definition));
  memset(&attack, 0, sizeof(attack));
  memset(&control, 0, sizeof(control));
  memset(&catalog, 0, sizeof(catalog));
  memset(&enemy, 0, sizeof(enemy));
  memset(&collision, 0, sizeof(collision));
  memset(&context, 0, sizeof(context));
  memset(animations, 0, sizeof(animations));
  memset(cells, 0, sizeof(cells));
  definition.resource_id = 1;
  definition.post_ai_values[3] = 159;
  definition.post_ai_values[41] = 0;
  definition.post_ai_values[47] = 4;
  attack.action_number = 2;
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
  enemy.presentation_target = UINT8_MAX;
  cells[1].status = 0x440;
  for (direction = 0u; direction < 8u; ++direction) {
    animations[direction].parts[0].cells = cells;
    animations[direction].part_count = 1u;
    animations[direction].frame_count = 3u;
  }
  catalog.controls = &control;
  catalog.actions = &attack;
  catalog.control_count = 1u;
  catalog.action_count = 1u;
  context.catalog = &catalog;
  context.collision = &collision;
  context.random_state = &random_state;
  context.attack_request = &resource_request;
  context.player.valid = true;
  context.player.position = (SfWorldPoint) {180, 100};
  context.player.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (resource_request.resource_id != 1 || resource_request.chart != 4 ||
      enemy.current_action != 2 ||
      enemy.presentation_action != 7u) {
    fprintf(stderr, "An unloaded direct attack did not request its artwork\n");
    return 1;
  }
  enemy.direct_attack_animations = animations;
  enemy.direct_attack_chart = 4u;
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.presentation_action != 1u || enemy.animation_chart != 4u ||
      enemy.animation_frame != 0u) {
    fprintf(stderr, "The direct attack did not enter its retail CAF chart\n");
    return 1;
  }
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.animation_frame != 1u || !enemy.direct_impact_pending ||
      enemy.presentation_audio_markers != 1u) {
    fprintf(stderr, "The direct attack missed its CAF impact marker\n");
    return 1;
  }
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.presentation_action != 1u || enemy.animation_frame != 2u) {
    fprintf(stderr, "The direct attack did not hold its final retail frame\n");
    return 1;
  }
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.presentation_action != 7u || enemy.animation_chart != 0u ||
      enemy.event_number != 2) {
    fprintf(stderr, "The direct attack did not publish its completion event\n");
    return 1;
  }
  return 0;
}

static int test_retreat_controller(void) {
  SfMctEnemy definition;
  SfAiAction retreat;
  SfAiControl control;
  SfAiControlCatalog catalog;
  SfScenarioEnemy enemy;
  SfCollisionQuery collision;
  SfScenarioEnemyControllerContext context;
  uint32_t random_state = 1u;
  memset(&definition, 0, sizeof(definition));
  memset(&retreat, 0, sizeof(retreat));
  memset(&control, 0, sizeof(control));
  memset(&catalog, 0, sizeof(catalog));
  memset(&enemy, 0, sizeof(enemy));
  memset(&collision, 0, sizeof(collision));
  memset(&context, 0, sizeof(context));
  definition.post_ai_values[54] = 3000;
  retreat.action_number = 9;
  retreat.parameters[1] = 2;
  retreat.parameters[3] = 15;
  retreat.parameters[7] = 4;
  retreat.parameters[8] = 25;
  retreat.conditions[3] = 1;
  retreat.conditions[4] = 100;
  retreat.conditions[5] = 500;
  enemy.definition = &definition;
  enemy.control = &control;
  enemy.selected_action = &retreat;
  enemy.position = (SfWorldPoint) {100, 100};
  enemy.previous_position = enemy.position;
  enemy.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  enemy.state[SF_SCENARIO_VISIBLE] = 1;
  enemy.current_life = 40;
  enemy.maximum_life = 40;
  enemy.event_number = -1;
  enemy.current_action = -1;
  enemy.presentation_action = 7u;
  catalog.controls = &control;
  catalog.actions = &retreat;
  catalog.control_count = 1u;
  catalog.action_count = 1u;
  context.catalog = &catalog;
  context.collision = &collision;
  context.random_state = &random_state;
  context.player.valid = true;
  context.player.position = (SfWorldPoint) {300, 100};
  context.player.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.current_action != 9 || enemy.event_number != 14 ||
      !enemy.movement_active || enemy.movement_speed != 45 ||
      enemy.position.x != 55 || enemy.position.y != 100 ||
      enemy.direction != 5u || enemy.animation_chart != 1u ||
      random_state != 2745024u) {
    fprintf(stderr, "Action nine did not enter retail player retreat\n");
    return 1;
  }
  sf_scenario_enemy_controller_update(&enemy, &context);
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.event_number != 9 || enemy.action_counter != 3 ||
      enemy.position.x != -35) {
    fprintf(stderr, "Action nine missed its inclusive completion event\n");
    return 1;
  }

  enemy.position = (SfWorldPoint) {100, 100};
  enemy.previous_position = enemy.position;
  enemy.selected_action = &retreat;
  enemy.current_action = -1;
  enemy.event_number = -1;
  enemy.presentation_action = 7u;
  enemy.movement_active = false;
  context.player.valid = false;
  context.companion.valid = true;
  context.companion.position = (SfWorldPoint) {300, 100};
  context.companion.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  random_state = 1u;
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.event_number != 14 || enemy.movement_active ||
      enemy.presentation_action != 7u || enemy.position.x != 100 ||
      random_state != 2745024u) {
    fprintf(stderr, "Companion retreat lost the retail no-step quirk\n");
    return 1;
  }
  sf_scenario_enemy_controller_update(&enemy, &context);
  if (enemy.event_number != 9) {
    fprintf(stderr, "Companion retreat did not publish completion\n");
    return 1;
  }
  return 0;
}

int main(void) {
  return test_event_selection() || test_patrol_controller() ||
    test_direct_attack_presentation() || test_retreat_controller();
}
