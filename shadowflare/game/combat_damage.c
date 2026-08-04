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

#include "game/combat_damage.h"

#include "core/retail_random.h"

#include <string.h>

static int32_t sf_combat_multiply(int32_t left, int32_t right) {
  return (int32_t) ((uint32_t) left * (uint32_t) right);
}

static int32_t sf_combat_subtract(int32_t left, int32_t right) {
  return (int32_t) ((uint32_t) left - (uint32_t) right);
}

static SfCombatDamageResult sf_combat_finished(int32_t damage) {
  SfCombatDamageResult result;
  memset(&result, 0, sizeof(result));
  result.valid = true;
  result.damage = damage < 1 ? 1 : damage;
  return result;
}

static int32_t sf_combat_paired_element(int32_t element) {
  return (element / 2) * 2 - (element % 2) + 1;
}

static int32_t sf_combat_element_factor(
    int32_t packet_element, int32_t defense_element,
    uint32_t *random_state) {
  if (sf_combat_paired_element(defense_element) == packet_element)
    return sf_retail_random_next(random_state) % 4 + 10;
  if (defense_element == packet_element)
    return sf_retail_random_next(random_state) % 4 + 7;
  return 10;
}

static SfCombatDamageResult sf_combat_elemental_damage(
    int32_t base, int32_t element_factor, int32_t defense,
    uint32_t *random_state) {
  const int32_t random_factor =
    sf_retail_random_next(random_state) % 3 + 9;
  const int32_t scaled = sf_combat_multiply(
    sf_combat_multiply(random_factor, base), element_factor) / 100;
  return sf_combat_finished(sf_combat_subtract(scaled, defense));
}

static bool sf_combat_defense_scale(
    const SfCombatPacket *packet, const SfCombatDefense *defense,
    const SfCombatTables *tables, int32_t *scale) {
  const int32_t index = packet->words[32] + 5;
  return index >= 0 && index < (int32_t) SF_COMBAT_DEFENSE_WORD_COUNT &&
    sf_combat_table_value(
      tables, SF_COMBAT_TABLE_DEFENSE_SCALE,
      defense->words[index] + 10, 0, scale);
}

SfCombatDamageResult sf_combat_damage_resolve(
    const SfCombatPacket *packet, const SfCombatDefense *defense,
    const SfCombatTables *tables, uint32_t *random_state) {
  SfCombatDamageResult invalid;
  uint32_t key;
  int32_t factor;
  int32_t scale;
  int32_t random_factor;
  memset(&invalid, 0, sizeof(invalid));
  if (!packet || !defense || !tables || !random_state) return invalid;
  if (packet->words[37] == 1) {
    invalid.valid = true;
    invalid.damage = packet->words[4];
    return invalid;
  }
  if (packet->words[1] == 3) {
    if (defense->words[0] == 0) {
      if (!sf_combat_defense_scale(packet, defense, tables, &scale))
        return invalid;
      random_factor = sf_retail_random_next(random_state) % 3 + 9;
      return sf_combat_finished(sf_combat_subtract(
        sf_combat_multiply(random_factor, packet->words[4]) / 10,
        sf_combat_multiply(defense->words[4], scale) / 10));
    }
    if (defense->words[0] != 1 && defense->words[0] != 2)
      return sf_combat_finished(1);
    factor = sf_combat_element_factor(
      packet->words[32], defense->words[13], random_state);
    return sf_combat_elemental_damage(
      packet->words[4], factor, defense->words[4], random_state);
  }
  key = (uint16_t) packet->words[0] |
    ((uint32_t) (uint16_t) defense->words[0] << 16u);
  if (key == UINT32_C(0x00000002)) {
    if (!sf_combat_defense_scale(packet, defense, tables, &scale))
      return invalid;
    random_factor = sf_retail_random_next(random_state) % 3 + 9;
    return sf_combat_finished(sf_combat_subtract(
      sf_combat_multiply(random_factor, packet->words[4]) / 10,
      sf_combat_multiply(defense->words[3], scale) / 10));
  }
  if (key == UINT32_C(0x00010002) || key == UINT32_C(0x00020001)) {
    factor = sf_combat_element_factor(
      packet->words[32], defense->words[13], random_state);
    return sf_combat_elemental_damage(
      packet->words[4], factor, defense->words[3], random_state);
  }
  if (key == UINT32_C(0x00020000)) {
    const int32_t packet_index =
      sf_combat_paired_element(defense->words[13]) * 2 + 6;
    invalid.requests_source_lookup = true;
    invalid.source_character_number = packet->words[2];
    random_factor = packet->words[1] == 1
      ? sf_retail_random_next(random_state) % 3 + 10
      : sf_retail_random_next(random_state) % 3 + 9;
    if (defense->words[13] < 0 || defense->words[13] > 7 ||
        packet_index < 0 ||
        packet_index >= (int32_t) SF_COMBAT_PACKET_WORD_COUNT ||
        !sf_combat_table_value(
          tables, SF_COMBAT_TABLE_PHYSICAL_SCALE,
          packet->words[packet_index] + 10, 0, &scale))
      return invalid;
    invalid = sf_combat_finished(sf_combat_subtract(
      sf_combat_multiply(
        sf_combat_multiply(packet->words[4], random_factor), scale) / 100,
      defense->words[3]));
    invalid.requests_source_lookup = true;
    invalid.source_character_number = packet->words[2];
    return invalid;
  }
  return sf_combat_finished(1);
}
