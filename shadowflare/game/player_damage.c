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

#include "game/player_damage.h"

#include "core/retail_random.h"
#include "game/combat_damage.h"
#include "game/equipment.h"
#include "game/player_elements.h"
#include "game/player_profile.h"
#include "game/special_items.h"

#include <string.h>

#define SF_PLAYER_HIT_ACTION 4
#define SF_PLAYER_DEATH_ACTION 5

static int32_t sf_damage_add(int32_t left, int32_t right) {
  return (int32_t) ((uint32_t) left + (uint32_t) right);
}

static int32_t sf_damage_subtract(int32_t left, int32_t right) {
  return (int32_t) ((uint32_t) left - (uint32_t) right);
}

static int32_t sf_damage_multiply(int32_t left, int32_t right) {
  return (int32_t) ((uint32_t) left * (uint32_t) right);
}

void sf_player_combat_defense(
    const SfPlayerState *player,
    const SfItemGroundDefinition *definitions, uint8_t definition_count,
    SfCombatDefense *defense, int32_t *physical_evasion) {
  SfPlayerProfile profile;
  int8_t affinities[SF_PLAYER_ELEMENT_COUNT];
  uint8_t element;
  if (!player) return;
  sf_player_profile_build(player, definitions, definition_count, &profile);
  if (defense) {
    memset(defense, 0, sizeof(*defense));
    sf_player_element_affinities(
      player, definitions, definition_count, affinities);
    defense->words[0] = 0;
    defense->words[1] = 0;
    defense->words[2] = profile.physical_attack;
    defense->words[3] = profile.physical_defense;
    defense->words[4] = profile.magical_defense;
    for (element = 0u; element < SF_PLAYER_ELEMENT_COUNT; ++element)
      defense->words[element + 5u] = affinities[element];
  }
  if (physical_evasion) *physical_evasion = profile.physical_evasion;
}

static bool sf_player_revival(SfPlayerState *player) {
  uint8_t index;
  for (index = 0u; index < player->special_items.count; ++index) {
    const SfInventoryItem *item = &player->special_items.items[index];
    SfInventoryItem consumed;
    if (item->category == 4u && item->definition_id == 98000000)
      return sf_special_items_take(
        &player->special_items, index, &consumed);
  }
  return false;
}

static bool sf_player_off_hand_enabled(
    const SfPlayerState *player,
    const SfItemGroundDefinition *definitions, uint8_t definition_count) {
  const SfInventoryItem *main = sf_equipment_item(
    &player->equipment, SF_EQUIPMENT_MAIN_HAND);
  const SfItemGroundDefinition *definition = main
    ? sf_equipment_find_definition(definitions, definition_count, main)
    : NULL;
  return !definition || !definition->suppresses_off_hand;
}

static void sf_player_damage_slot(
    SfPlayerState *player, SfEquipmentSlot slot, int32_t chance,
    bool enabled, uint32_t *random_state) {
  SfInventoryItem *item;
  if ((player->equipment.occupied & (uint16_t) (1u << slot)) == 0u) return;
  item = &player->equipment.items[slot];
  if (sf_retail_random_next(random_state) % 100 < chance &&
      enabled && item->durability > 0) --item->durability;
}

static void sf_player_damage_equipment(
    SfPlayerState *player,
    const SfItemGroundDefinition *definitions, uint8_t definition_count,
    uint32_t *random_state) {
  sf_player_damage_slot(
    player, SF_EQUIPMENT_HELMET, 20, true, random_state);
  sf_player_damage_slot(
    player, SF_EQUIPMENT_BODY, 30, true, random_state);
  sf_player_damage_slot(
    player, SF_EQUIPMENT_OFF_HAND, 30,
    sf_player_off_hand_enabled(player, definitions, definition_count),
    random_state);
  sf_player_damage_slot(
    player, SF_EQUIPMENT_BOOTS, 20, true, random_state);
}

static bool sf_player_damage_reaction(
    SfPlayerState *player, const SfCombatPacket *packet,
    SfWorldPoint origin, int32_t damage, const SfCombatDefense *defense,
    int32_t maximum_life, const SfCombatTables *tables,
    uint32_t *random_state) {
  int32_t affinity;
  int32_t affinity_chance = 0;
  int32_t affinity_duration = 0;
  int32_t damage_row;
  int32_t chance;
  int32_t duration;
  bool motion;
  if (packet->words[32] < 0 || packet->words[32] >= 8 || maximum_life <= 0)
    return false;
  affinity = defense->words[packet->words[32] + 5];
  if (affinity < 0 && (affinity < -10 ||
      !sf_combat_table_value(
        tables, SF_COMBAT_TABLE_PLAYER_AFFINITY,
        affinity + 10, 0, &affinity_chance) ||
      !sf_combat_table_value(
        tables, SF_COMBAT_TABLE_PLAYER_AFFINITY,
        affinity + 10, 1, &affinity_duration))) return false;
  damage_row = sf_damage_multiply(damage, 50) / maximum_life;
  if (damage_row > 49) damage_row = 49;
  if (damage_row < 0) return false;
  chance = packet->words[41];
  if (chance == -1 && !sf_combat_table_value(
        tables, SF_COMBAT_TABLE_REACTION, damage_row, 0, &chance))
    return false;
  chance = sf_damage_add(chance,
    sf_damage_add(packet->words[42], affinity_chance));
  if (chance < 0) chance = 0;
  if (chance <= sf_retail_random_next(random_state) % 100) return true;
  duration = packet->words[43];
  if (duration == -1 && !sf_combat_table_value(
        tables, SF_COMBAT_TABLE_REACTION, damage_row, 1, &duration))
    return false;
  duration = sf_damage_add(duration,
    sf_damage_add(packet->words[44], affinity_duration));
  if (duration < 1) duration = 1;
  motion = packet->words[40] != 0;
  if (!motion && duration > 15) duration = 15;
  if (player->damage.action != SF_PLAYER_HIT_ACTION ||
      player->damage.reaction_stage != 2u) {
    player->damage.action = SF_PLAYER_HIT_ACTION;
    player->damage.reaction_stage = 0u;
    player->damage.counter = 0;
    player->damage.action_locked = true;
    player->damage.reaction_duration = sf_damage_add(
      duration, packet->words[76]);
    if (player->damage.reaction_duration < 1)
      player->damage.reaction_duration = 1;
    player->damage.reaction_additive = packet->words[76];
    player->damage.reaction_motion = motion;
    if (!motion) sf_player_face_toward(player, origin);
  }
  return true;
}

static void sf_damage_consume_effect_random(
    const SfPlayerState *player, const SfCombatPacket *packet,
    uint32_t *random_state) {
  if (packet->words[3] == 1 && player->damage.action_locked &&
      player->damage.reaction_duration != 0)
    (void) sf_retail_random_next(random_state);
  if (packet->words[72] != 0 &&
      sf_retail_random_next(random_state) % 100 < 20) {
    (void) sf_retail_random_next(random_state);
    (void) sf_retail_random_next(random_state);
  }
}

SfPlayerDamageResult sf_player_receive_damage(
    SfPlayerState *player, const SfCombatPacket *packet,
    SfWorldPoint impact_origin,
    const SfItemGroundDefinition *definitions, uint8_t definition_count,
    const SfCombatTables *tables, uint32_t *random_state) {
  SfPlayerDamageResult result;
  SfCombatDefense defense;
  SfCombatDefense reaction_defense;
  SfCombatDamageResult damage;
  SfPlayerProfile profile;
  memset(&result, 0, sizeof(result));
  result.accepted = true;
  if (!player || !packet || !tables || !random_state) return result;
  sf_player_combat_defense(
    player, definitions, definition_count, &defense, NULL);
  sf_player_profile_build(player, definitions, definition_count, &profile);
  if (packet->words[4] < 1) {
    memset(&damage, 0, sizeof(damage));
    damage.valid = true;
  } else {
    damage = sf_combat_damage_resolve(
      packet, &defense, tables, random_state);
  }
  if (!damage.valid) return result;
  result.damage = damage.damage;
  player->current_life = sf_damage_subtract(
    player->current_life, damage.damage);
  if (player->current_life < 1 && sf_player_revival(player)) {
    player->current_life = profile.maximum_life;
    player->current_mana = profile.maximum_mana;
    result.revived = true;
    result.audio_sample = 17u;
  }
  if (player->current_life < 1) {
    if (player->damage.action != SF_PLAYER_HIT_ACTION ||
        player->damage.reaction_stage != 2u)
      player->damage.reaction_stage = 0u;
    player->damage.action = SF_PLAYER_DEATH_ACTION;
    player->damage.counter = 0;
    player->damage.action_locked = true;
    sf_player_cancel_movement(player);
  }
  sf_player_damage_equipment(
    player, definitions, definition_count, random_state);
  if (player->damage.action != SF_PLAYER_HIT_ACTION ||
      player->damage.reaction_stage != 2u)
    player->damage.reaction_duration = 0;
  if (player->current_life >= 1 && packet->words[38] == 1)
    (void) sf_retail_random_next(random_state);
  sf_player_combat_defense(
    player, definitions, definition_count, &reaction_defense, NULL);
  if (player->current_life > 0 && !sf_player_damage_reaction(
        player, packet, impact_origin, damage.damage, &reaction_defense,
        profile.maximum_life, tables, random_state)) return result;
  sf_damage_consume_effect_random(player, packet, random_state);
  if (player->damage.event_number == 0) player->damage.event_number = 4;
  result.valid = true;
  return result;
}
