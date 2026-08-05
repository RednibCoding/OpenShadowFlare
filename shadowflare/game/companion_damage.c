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

#include "game/companion_damage.h"

#include "core/retail_random.h"
#include "game/combat_damage.h"
#include "game/movement.h"

#include <string.h>

#define SF_COMPANION_HIT_ACTION 5
#define SF_COMPANION_DEATH_ACTION 6

static int32_t sf_companion_add(int32_t left, int32_t right) {
  return (int32_t) ((uint32_t) left + (uint32_t) right);
}

static int32_t sf_companion_subtract(int32_t left, int32_t right) {
  return (int32_t) ((uint32_t) left - (uint32_t) right);
}

static int32_t sf_companion_multiply(int32_t left, int32_t right) {
  return (int32_t) ((uint32_t) left * (uint32_t) right);
}

void sf_companion_combat_defense(
    const SfCompanionState *companion,
    SfCombatDefense *defense, int32_t *physical_evasion) {
  if (!companion) return;
  if (defense) {
    memset(defense, 0, sizeof(*defense));
    defense->words[0] = 1;
    defense->words[1] = SF_COMPANION_CHARACTER_NUMBER;
    defense->words[3] = companion->profile.values[7];
    defense->words[4] = companion->profile.values[11];
    defense->words[13] = companion->profile.values[13];
  }
  if (physical_evasion) *physical_evasion = companion->profile.values[8];
}

static bool sf_companion_ignored_action(int32_t action) {
  return action == 7 || action == 8 || action == 10;
}

static bool sf_companion_reaction(
    SfCompanionState *companion, const SfCombatPacket *packet,
    SfWorldPoint origin, int32_t damage, const SfCombatTables *tables,
    uint32_t *random_state) {
  const int32_t native = companion->profile.values[13];
  const int32_t maximum_life = companion->profile.values[3];
  const int32_t pair_base = (native / 2) * 2 - (native % 2);
  int32_t affinity_chance = 0;
  int32_t affinity_duration = 0;
  int32_t affinity_row = -1;
  int32_t damage_row;
  int32_t chance;
  int32_t duration;
  bool motion;
  if (native < 0 || native >= 8 || maximum_life <= 0) return false;
  if (packet->words[0] == 0) {
    const int32_t strength = packet->words[pair_base + 7];
    if (strength >= 1 && strength <= 10) affinity_row = strength - 1;
  } else if (packet->words[32] == pair_base + 1) {
    affinity_row = 5;
  }
  if (affinity_row >= 0 && (!sf_combat_table_value(
        tables, SF_COMBAT_TABLE_COMPANION_AFFINITY,
        affinity_row, 0, &affinity_chance) ||
      !sf_combat_table_value(
        tables, SF_COMBAT_TABLE_COMPANION_AFFINITY,
        affinity_row, 1, &affinity_duration))) return false;
  damage_row = sf_companion_multiply(damage, 50) / maximum_life;
  if (damage_row > 49) damage_row = 49;
  if (damage_row < 0) return false;
  chance = packet->words[41];
  if (chance == -1 && !sf_combat_table_value(
        tables, SF_COMBAT_TABLE_REACTION, damage_row, 0, &chance))
    return false;
  chance = sf_companion_add(chance,
    sf_companion_add(packet->words[42], affinity_chance));
  if (chance < 0) chance = 0;
  if (chance <= sf_retail_random_next(random_state) % 100) return true;
  duration = packet->words[43];
  if (duration == -1 && !sf_combat_table_value(
        tables, SF_COMBAT_TABLE_REACTION, damage_row, 1, &duration))
    return false;
  duration = sf_companion_add(duration,
    sf_companion_add(packet->words[44], affinity_duration));
  if (duration < 1) duration = 1;
  motion = packet->words[40] != 0;
  if (!motion && duration > 15) duration = 15;
  if (companion->damage.action != SF_COMPANION_HIT_ACTION ||
      companion->damage.reaction_stage != 2u) {
    companion->damage.action = SF_COMPANION_HIT_ACTION;
    companion->damage.reaction_stage = 0u;
    companion->damage.counter = 0;
    companion->damage.action_locked = true;
    companion->damage.reaction_duration = sf_companion_add(
      duration, packet->words[76]);
    if (companion->damage.reaction_duration < 1)
      companion->damage.reaction_duration = 1;
    companion->damage.reaction_additive = packet->words[76];
    companion->damage.reaction_motion = motion;
    if (!motion) companion->direction = sf_movement_direction(
      companion->position, origin);
  }
  return true;
}

static void sf_companion_effect_random(
    const SfCompanionState *companion, const SfCombatPacket *packet,
    uint32_t *random_state) {
  if (packet->words[3] == 1 && companion->damage.action_locked &&
      companion->damage.reaction_duration != 0)
    (void) sf_retail_random_next(random_state);
  if (packet->words[72] != 0 &&
      sf_retail_random_next(random_state) % 100 < 20) {
    (void) sf_retail_random_next(random_state);
    (void) sf_retail_random_next(random_state);
  }
}

SfCompanionDamageResult sf_companion_receive_damage(
    SfCompanionState *companion, const SfCombatPacket *packet,
    SfWorldPoint impact_origin, const SfCombatTables *tables,
    uint32_t *random_state) {
  SfCompanionDamageResult result;
  SfCombatDefense defense;
  SfCombatDamageResult damage;
  memset(&result, 0, sizeof(result));
  if (!companion || !packet || !tables || !random_state ||
      companion->current_life < 1 ||
      sf_companion_ignored_action(companion->damage.action)) return result;
  result.accepted = true;
  sf_companion_combat_defense(companion, &defense, NULL);
  if (packet->words[4] < 1) {
    memset(&damage, 0, sizeof(damage));
    damage.valid = true;
  } else {
    damage = sf_combat_damage_resolve(
      packet, &defense, tables, random_state);
  }
  if (!damage.valid) return result;
  result.damage = damage.damage;
  companion->current_life = sf_companion_subtract(
    companion->current_life, damage.damage);
  if (companion->current_life <= 0) {
    if (companion->damage.action != SF_COMPANION_HIT_ACTION ||
        companion->damage.reaction_stage != 2u)
      companion->damage.reaction_stage = 0u;
    companion->damage.action = SF_COMPANION_DEATH_ACTION;
    companion->damage.counter = 0;
    companion->damage.action_locked = true;
    companion->motion = SF_COMPANION_IDLE;
  }
  if (companion->damage.action != SF_COMPANION_HIT_ACTION ||
      companion->damage.reaction_stage != 2u)
    companion->damage.reaction_duration = 0;
  if (companion->current_life > 0 && !sf_companion_reaction(
        companion, packet, impact_origin, damage.damage,
        tables, random_state)) return result;
  sf_companion_effect_random(companion, packet, random_state);
  if (companion->current_life <= 0) {
    companion->damage.action = SF_COMPANION_DEATH_ACTION;
    companion->damage.counter = 0;
    companion->damage.action_locked = true;
  }
  if (companion->damage.event_number == 0)
    companion->damage.event_number = 4;
  result.valid = true;
  return result;
}
