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

#include "data/combat_tables.h"
#include "game/combat_damage.h"
#include "game/companion_damage.h"
#include "game/enemy_direct_impact.h"
#include "game/player_damage.h"
#include "game/world_enemy_combat.h"

#include <stdio.h>
#include <string.h>

static int sf_test_tables(SfCombatTables *tables) {
  char path[1024];
  SfCombatPacket packet;
  SfCombatDefense defense;
  SfCombatDamageResult result;
  uint32_t random_state = 1u;
  (void) snprintf(
    path, sizeof(path),
    "%s/tmp/ShadowFlare/System/Game/Parameter/Table.Tbd",
    OPENSHADOWFLARE_SOURCE_DIR);
  if (!sf_combat_tables_load(path, tables)) {
    FILE *fixture = fopen(path, "rb");
    if (!fixture) return 0;
    fclose(fixture);
    fprintf(stderr, "The retail combat tables did not load\n");
    return 1;
  }
  memset(&packet, 0, sizeof(packet));
  memset(&defense, 0, sizeof(defense));
  packet.words[0] = 2;
  packet.words[4] = 100;
  packet.words[32] = 0;
  defense.words[3] = 20;
  defense.words[4] = 20;
  defense.words[5] = 0;
  result = sf_combat_damage_resolve(
    &packet, &defense, tables, &random_state);
  if (!result.valid || result.damage != 90 || random_state != 2745024u) {
    fprintf(stderr, "Table-11 damage lost its retail value or RNG order\n");
    return 1;
  }
  return 0;
}

static int sf_test_direct_packet(void) {
  SfMctEnemy definition;
  SfScenarioEnemy enemy;
  SfScenarioEnemyControllerContext context;
  SfEnemyDirectImpactResult result;
  uint32_t random_state = 1u;
  memset(&definition, 0, sizeof(definition));
  memset(&enemy, 0, sizeof(enemy));
  memset(&context, 0, sizeof(context));
  definition.id = 12;
  definition.pre_ai_values[6] = 3;
  definition.pre_ai_values[7] = 310;
  definition.post_ai_values[0] = 40;
  definition.post_ai_values[1] = 50;
  definition.post_ai_values[3] = 159;
  definition.post_ai_values[4] = 259;
  definition.post_ai_values[6] = 200;
  definition.post_ai_values[7] = 190;
  definition.post_ai_values[21] = -1;
  definition.post_ai_values[29] = 410;
  definition.post_ai_values[30] = 411;
  definition.post_ai_values[32] = 430;
  definition.post_ai_values[33] = 431;
  definition.post_ai_values[35] = 400;
  definition.post_ai_values[36] = 401;
  enemy.definition = &definition;
  enemy.position = (SfWorldPoint) {500, 600};
  enemy.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  enemy.direction = 1u;
  enemy.event_number = -1;
  context.player.valid = true;
  context.player.position = (SfWorldPoint) {600, 600};
  context.player.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  result = sf_enemy_direct_impact_resolve(
    &enemy, &context, 0, &random_state);
  if (!result.valid || !result.apply_damage || result.show_miss ||
      result.target != SF_ENEMY_IMPACT_TARGET_PLAYER ||
      result.hit_chance != 98 || result.hit_roll != 67 ||
      result.post_hit_event != 17 || result.post_hit_audio_sample != 6u ||
      result.packet.words[2] != 14000012 ||
      result.packet.words[34] != 21001 ||
      random_state != 3357800067u) {
    fprintf(stderr, "Direct impact lost its marker-time packet or hit roll\n");
    return 1;
  }
  random_state = 1u;
  result = sf_enemy_direct_impact_resolve(
    &enemy, &context, 1, &random_state);
  if (!result.valid || !result.apply_damage ||
      result.packet.words[4] != 50 || result.packet.words[36] != 190 ||
      result.packet.words[40] != 401 || result.packet.words[41] != 411 ||
      result.packet.words[43] != 431) {
    fprintf(stderr, "Direct variant one lost its own retail packet columns\n");
    return 1;
  }
  return 0;
}

static int sf_test_receivers(const SfCombatTables *tables) {
  SfCombatPacket packet;
  SfPlayerState player;
  SfCompanionState companion;
  SfCompanionProfile companion_profile;
  SfPlayerDamageResult player_result;
  SfCompanionDamageResult companion_result;
  uint32_t player_random = 1u;
  uint32_t companion_random = 1u;
  memset(&packet, 0, sizeof(packet));
  packet.words[0] = 2;
  packet.words[4] = 25;
  packet.words[32] = 0;
  packet.words[37] = 1;
  packet.words[41] = 0;
  packet.words[43] = 1;
  sf_player_init(&player, 1u);
  player.initial_parameters.values[2] = 100;
  player.initial_parameters.values[3] = 80;
  player.initial_parameters.values[6] = 20;
  player.initial_parameters.values[8] = 10;
  player.initial_parameters.values[10] = 10;
  player.current_life = 100;
  player.current_mana = 80;
  player_result = sf_player_receive_damage(
    &player, &packet, (SfWorldPoint) {100, 0},
    NULL, 0u, tables, &player_random);
  if (!player_result.valid || !player_result.accepted ||
      player_result.damage != 25 || player.current_life != 75 ||
      player.damage.event_number != 4) {
    fprintf(stderr, "The player did not accept direct retail damage\n");
    return 1;
  }
  memset(&companion_profile, 0, sizeof(companion_profile));
  companion_profile.type = 0;
  companion_profile.resource_id = 0;
  companion_profile.values[3] = 100;
  companion_profile.values[7] = 5;
  companion_profile.values[8] = 10;
  companion_profile.values[11] = 5;
  companion_profile.values[13] = 0;
  if (!sf_companion_init(
        &companion, &companion_profile,
        (SfWorldPoint) {0, 0}, 1u, false)) return 1;
  companion_result = sf_companion_receive_damage(
    &companion, &packet, (SfWorldPoint) {100, 0},
    tables, &companion_random);
  if (!companion_result.valid || !companion_result.accepted ||
      companion_result.damage != 25 || companion.current_life != 75 ||
      companion.damage.event_number != 4) {
    fprintf(stderr, "The companion did not accept direct retail damage\n");
    return 1;
  }
  return 0;
}

static int sf_test_live_dispatch(const SfCombatTables *tables) {
  SfWorldState world;
  SfMctEnemy definition;
  SfScenarioEnemy enemy;
  SfScenarioEnemyControllerContext context;
  const int32_t initial_life = 100;
  memset(&definition, 0, sizeof(definition));
  memset(&enemy, 0, sizeof(enemy));
  memset(&context, 0, sizeof(context));
  sf_world_state_init(&world, 1, 0, 1u);
  world.player.initial_parameters.values[2] = initial_life;
  world.player.initial_parameters.values[3] = 80;
  world.player.initial_parameters.values[6] = 20;
  world.player.initial_parameters.values[8] = 10;
  world.player.initial_parameters.values[10] = 1;
  world.player.current_life = initial_life;
  world.player.current_mana = 80;
  world.player.position = (SfWorldPoint) {600, 600};
  world.player.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  world.combat_tables = tables;
  definition.id = 12;
  definition.pre_ai_values[6] = 0;
  definition.post_ai_values[0] = 40;
  definition.post_ai_values[3] = 159;
  definition.post_ai_values[6] = 200;
  definition.post_ai_values[21] = -1;
  definition.post_ai_values[29] = 0;
  definition.post_ai_values[32] = 1;
  enemy.definition = &definition;
  enemy.position = (SfWorldPoint) {500, 600};
  enemy.judgement = (SfObjectBounds) {-10, -10, 10, 10};
  enemy.direction = 1u;
  enemy.event_number = -1;
  enemy.presentation_action = 1u;
  enemy.direct_impact_pending = true;
  context.random_state = &world.random_state;
  sf_world_enemy_combat_targets(&world, &context);
  if (!sf_world_enemy_combat_apply_direct(&world, &enemy, &context) ||
      world.player.current_life >= initial_life ||
      enemy.event_number != 17 || world.sounds.count == 0u ||
      world.sounds.samples[world.sounds.count - 1u] != 6u) {
    fprintf(stderr, "The live CAF marker did not reach the player receiver\n");
    return 1;
  }
  return 0;
}

int main(void) {
  SfCombatTables tables;
  return sf_test_tables(&tables) || sf_test_direct_packet() ||
    sf_test_receivers(&tables) || sf_test_live_dispatch(&tables);
}
