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

#include "core/retail_random.h"

#include <stdbool.h>

#define SF_AI_PRIORITY_PARAMETER 0u
#define SF_AI_WEIGHT_PARAMETER 2u
#define SF_AI_LIFE_ENABLED_CONDITION 0u
#define SF_AI_MINIMUM_LIFE_CONDITION 1u
#define SF_AI_MAXIMUM_LIFE_CONDITION 2u
#define SF_AI_TARGET_ENABLED_CONDITION 3u
#define SF_AI_MINIMUM_DISTANCE_CONDITION 4u
#define SF_AI_MAXIMUM_DISTANCE_CONDITION 5u

static bool sf_enemy_ai_in_range(
    int32_t value, int32_t minimum, int32_t maximum) {
  return (minimum == -1 || value >= minimum) &&
    (maximum == -1 || value <= maximum);
}

static bool sf_enemy_ai_eligible(
    const SfAiAction *action, int32_t current_life,
    int32_t maximum_life, SfEnemyAiTargetDistances targets) {
  int32_t life_percent;
  if (action->conditions[SF_AI_TARGET_ENABLED_CONDITION] == 1 &&
      !((targets.player >= 0 && sf_enemy_ai_in_range(
          targets.player,
          action->conditions[SF_AI_MINIMUM_DISTANCE_CONDITION],
          action->conditions[SF_AI_MAXIMUM_DISTANCE_CONDITION])) ||
        (targets.companion >= 0 && sf_enemy_ai_in_range(
          targets.companion,
          action->conditions[SF_AI_MINIMUM_DISTANCE_CONDITION],
          action->conditions[SF_AI_MAXIMUM_DISTANCE_CONDITION])))) return false;
  if (action->conditions[SF_AI_LIFE_ENABLED_CONDITION] != 1) return true;
  if (maximum_life <= 0) return false;
  life_percent = (int32_t) (
    (int64_t) current_life * 100 / maximum_life);
  return sf_enemy_ai_in_range(
    life_percent,
    action->conditions[SF_AI_MINIMUM_LIFE_CONDITION],
    action->conditions[SF_AI_MAXIMUM_LIFE_CONDITION]);
}

static const SfAiAction *sf_enemy_ai_select_event(
    const SfAiControlCatalog *catalog, const SfAiControl *control,
    uint8_t event, int32_t current_life, int32_t maximum_life,
    SfEnemyAiTargetDistances targets, uint32_t *random_state) {
  const SfAiEvent *record = &control->events[event];
  int32_t highest_priority = -1;
  uint16_t first_candidate = 0u;
  int64_t total_weight = 0;
  int64_t draw;
  int64_t accumulated = 0;
  uint16_t index;
  bool found = false;
  for (index = 0u; index < record->action_count; ++index) {
    const SfAiAction *action = sf_ai_control_action(
      catalog, control, event, index);
    if (!action || !sf_enemy_ai_eligible(
          action, current_life, maximum_life, targets)) continue;
    if (highest_priority < action->parameters[SF_AI_PRIORITY_PARAMETER]) {
      highest_priority = action->parameters[SF_AI_PRIORITY_PARAMETER];
      first_candidate = index;
    }
    found = true;
  }
  if (!found) return NULL;
  for (index = first_candidate; index < record->action_count; ++index) {
    const SfAiAction *action = sf_ai_control_action(
      catalog, control, event, index);
    if (action && sf_enemy_ai_eligible(
          action, current_life, maximum_life, targets))
      total_weight += action->parameters[SF_AI_WEIGHT_PARAMETER];
  }
  if (total_weight <= 0) return NULL;
  draw = sf_retail_random_next(random_state) % total_weight;
  index = record->action_count;
  while (index > first_candidate) {
    const SfAiAction *action;
    --index;
    action = sf_ai_control_action(catalog, control, event, index);
    if (!action || !sf_enemy_ai_eligible(
          action, current_life, maximum_life, targets)) continue;
    accumulated += action->parameters[SF_AI_WEIGHT_PARAMETER];
    if (draw < accumulated) return action;
  }
  return NULL;
}

const SfAiAction *sf_enemy_ai_select(
    const SfAiControlCatalog *catalog, const SfAiControl *control,
    int32_t event_number, int32_t current_life, int32_t maximum_life,
    SfEnemyAiTargetDistances targets, uint32_t *random_state) {
  const SfAiAction *selected;
  if (!catalog || !control || !random_state || event_number < 0 ||
      event_number >= (int32_t) SF_AI_CONTROL_EVENT_COUNT) return NULL;
  selected = sf_enemy_ai_select_event(
    catalog, control, (uint8_t) event_number,
    current_life, maximum_life, targets, random_state);
  if (!selected && ((event_number >= 1 && event_number <= 10) ||
      event_number == 16 || event_number == 17))
    selected = sf_enemy_ai_select_event(
      catalog, control, 0u, current_life, maximum_life,
      targets, random_state);
  return selected;
}
