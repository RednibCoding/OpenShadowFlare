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

#include "game/player_profile.h"

#include "game/equipment.h"

#include <limits.h>
#include <string.h>

static int32_t sf_profile_add(int32_t value, int32_t bonus) {
  const int64_t sum = (int64_t) value + bonus;
  return sum > INT32_MAX ? INT32_MAX :
    sum < INT32_MIN ? INT32_MIN : (int32_t) sum;
}

static int32_t sf_profile_minimum(int32_t value, int32_t minimum) {
  return value < minimum ? minimum : value;
}

static void sf_profile_equipment_bonuses(
    const SfPlayerState *player,
    const SfItemGroundDefinition *definitions, uint8_t definition_count,
    int32_t bonuses[10]) {
  uint8_t slot;
  memset(bonuses, 0, 10u * sizeof(bonuses[0]));
  for (slot = 0u; slot < SF_EQUIPMENT_VISIBLE_SLOT_COUNT; ++slot) {
    const SfInventoryItem *item = sf_equipment_item(
      &player->equipment, (SfEquipmentSlot) slot);
    const SfItemGroundDefinition *definition = item
      ? sf_equipment_find_definition(definitions, definition_count, item)
      : NULL;
    uint8_t parameter;
    if (!definition || (item->category <= 1u && item->durability == 0))
      continue;
    if (slot == SF_EQUIPMENT_OFF_HAND) {
      const SfInventoryItem *main_hand = sf_equipment_item(
        &player->equipment, SF_EQUIPMENT_MAIN_HAND);
      const SfItemGroundDefinition *main_definition = main_hand
        ? sf_equipment_find_definition(
            definitions, definition_count, main_hand) : NULL;
      if (main_definition && main_definition->suppresses_off_hand) continue;
    }
    for (parameter = 0u; parameter < 10u; ++parameter)
      bonuses[parameter] = sf_profile_add(
        bonuses[parameter], definition->parameter_bonuses[parameter]);
  }
}

void sf_player_profile_build(
    const SfPlayerState *player,
    const SfItemGroundDefinition *definitions, uint8_t definition_count,
    SfPlayerProfile *profile) {
  int32_t bonus[10];
  if (!player || !profile) return;
  sf_profile_equipment_bonuses(
    player, definitions, definition_count, bonus);
  profile->attack_speed = sf_profile_add(
    player->initial_parameters.values[0], bonus[8]);
  profile->walking_speed = sf_profile_add(
    player->initial_parameters.values[1], bonus[9]);
  if (profile->attack_speed < 0) profile->attack_speed = 0;
  if (profile->attack_speed > 255) profile->attack_speed = 255;
  if (profile->walking_speed < 0) profile->walking_speed = 0;
  if (profile->walking_speed > 255) profile->walking_speed = 255;
  profile->maximum_life = sf_profile_minimum(
    player->initial_parameters.values[2], 1);
  profile->maximum_mana = sf_profile_minimum(
    player->initial_parameters.values[3], 1);
  profile->weight_capacity = sf_profile_minimum(
    player->initial_parameters.values[4], 0);
  profile->physical_attack = sf_profile_minimum(sf_profile_add(
    player->initial_parameters.values[5], bonus[0]), 1);
  profile->physical_defense = sf_profile_minimum(sf_profile_add(
    player->initial_parameters.values[6], bonus[2]), 1);
  profile->magical_attack = sf_profile_minimum(sf_profile_add(
    player->initial_parameters.values[7], bonus[4]), 1);
  profile->magical_defense = sf_profile_minimum(sf_profile_add(
    player->initial_parameters.values[8], bonus[6]), 1);
  profile->hit_rate = sf_profile_minimum(sf_profile_add(
    player->initial_parameters.values[9], bonus[1]), 1);
  profile->physical_evasion = sf_profile_minimum(sf_profile_add(
    player->initial_parameters.values[10], bonus[3]), 1);
  profile->magical_hit_rate = sf_profile_minimum(sf_profile_add(
    player->initial_parameters.values[11], bonus[5]), 1);
  profile->magical_evasion = sf_profile_minimum(sf_profile_add(
    player->initial_parameters.values[12], bonus[7]), 1);
}

const char *sf_player_job_name(int32_t job, uint8_t gender) {
  if (job == 5) return "Hunter";
  if (job == 6) return "Warrior";
  if (job == 9) return gender == 1u ? "Wizard" : "Witch";
  if (job == 16) return "Mercenary";
  return "";
}
