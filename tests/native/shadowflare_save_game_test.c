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

#include "assets/gameplay_assets.h"
#include "core/arena.h"
#include "core/memory_budget.h"
#include "data/save_game.h"
#include "data/save_payload.h"
#include "game/player_save.h"
#include "game/world_save.h"

#include <stdio.h>
#include <string.h>

#define FIXTURE_CAPACITY 4096u

typedef struct FixtureBytes {
  uint8_t bytes[FIXTURE_CAPACITY];
  size_t size;
} FixtureBytes;

typedef union GameplayMemory {
  long double alignment;
  void *pointer;
  uint8_t bytes[SF_MAIN_ARENA_BYTES];
} GameplayMemory;

static GameplayMemory gameplay_memory;

static bool append_bytes(FixtureBytes *output, const void *bytes, size_t size) {
  if (output->size + size > sizeof(output->bytes)) return false;
  memcpy(output->bytes + output->size, bytes, size);
  output->size += size;
  return true;
}

static bool append_i32(FixtureBytes *output, int32_t value) {
  const uint32_t data = (uint32_t) value;
  const uint8_t bytes[4] = {
    (uint8_t) data, (uint8_t) (data >> 8u),
    (uint8_t) (data >> 16u), (uint8_t) (data >> 24u)
  };
  return append_bytes(output, bytes, sizeof(bytes));
}

static void store_i32(uint8_t *bytes, size_t offset, int32_t value) {
  const uint32_t data = (uint32_t) value;
  bytes[offset] = (uint8_t) data;
  bytes[offset + 1u] = (uint8_t) (data >> 8u);
  bytes[offset + 2u] = (uint8_t) (data >> 16u);
  bytes[offset + 3u] = (uint8_t) (data >> 24u);
}

static bool append_item(
    FixtureBytes *output, int32_t category, int32_t definition_id,
    int32_t grid_x, int32_t grid_y, bool has_grid,
    int32_t durability, int32_t quantity) {
  static const int32_t state_sizes[5] = {200, 200, 192, 0, 4};
  uint8_t state[200] = {0};
  const int32_t state_size = state_sizes[category];
  if ((category == 0 || category == 1)) store_i32(state, 188u, durability);
  if (category == 4) store_i32(state, 0u, quantity);
  return append_i32(output, category) &&
    append_i32(output, definition_id) && append_i32(output, 1) &&
    (!has_grid || (append_i32(output, grid_x) && append_i32(output, grid_y))) &&
    append_i32(output, state_size) &&
    append_bytes(output, state, (size_t) state_size);
}

static bool append_equipment(FixtureBytes *payload) {
  uint8_t slot;
  for (slot = 0u; slot < SF_SAVED_EQUIPMENT_COUNT; ++slot) {
    const bool present = slot == 0u || slot == 2u || slot == 9u;
    if (!append_i32(payload, present ? 1 : 0)) return false;
    if (slot == 0u && !append_item(payload, 0, 0, 0, 0, false, 55, 1))
      return false;
    if (slot == 2u && !append_item(payload, 1, 0, 0, 0, false, 70, 1))
      return false;
    if (slot == 9u && !append_item(payload, 0, 0, 0, 0, false, 40, 1))
      return false;
  }
  return true;
}

static bool append_magic(FixtureBytes *payload) {
  uint8_t index;
  if (!append_i32(payload, 22)) return false;
  for (index = 0u; index < 22u; ++index)
    if (!append_i32(payload, index == 0u ? 3 : 0)) return false;
  for (index = 0u; index < 22u; ++index)
    if (!append_i32(payload, index == 0u ? 4 : 1)) return false;
  for (index = 0u; index < 22u; ++index)
    if (!append_i32(payload, index == 0u ? 5 : 0)) return false;
  for (index = 0u; index < 8u; ++index)
    if (!append_i32(payload, index == 0u ? 0 : -1)) return false;
  return true;
}

static bool append_companions(FixtureBytes *payload) {
  uint8_t index;
  if (!append_i32(payload, SF_COMPANION_COUNT)) return false;
  for (index = 0u; index < SF_COMPANION_COUNT; ++index)
    if (!append_i32(payload, index == 0u ? 4 : 1)) return false;
  for (index = 0u; index < SF_COMPANION_COUNT; ++index)
    if (!append_i32(payload, index == 0u ? 12 : 0)) return false;
  return true;
}

static bool append_progress(FixtureBytes *payload) {
  static const uint8_t extension_signature[8] = {
    'O', 'S', 'F', 'S', 'T', '0', '1', 0
  };
  return append_i32(payload, 3) && append_i32(payload, 1) &&
    append_i32(payload, 2) && append_i32(payload, 3) &&
    append_i32(payload, 2) && append_i32(payload, 4) &&
    append_i32(payload, 5) && append_i32(payload, 4) &&
    append_i32(payload, 6) && append_i32(payload, 7) &&
    append_i32(payload, 8) && append_i32(payload, 9) &&
    append_magic(payload) && append_companions(payload) &&
    append_i32(payload, 7) && append_i32(payload, 1) &&
    append_i32(payload, 0) && append_i32(payload, 0) &&
    append_bytes(
      payload, extension_signature, sizeof(extension_signature)) &&
    append_i32(payload, 24) && append_i32(payload, 3) &&
    append_i32(payload, 1) && append_i32(payload, 7);
}

static void make_record(uint8_t record[SF_SAVED_PLAYER_RECORD_SIZE]) {
  static const uint16_t offsets[SF_PLAYER_INITIAL_PARAMETER_COUNT] = {
    0x28u, 0x2cu, 0x30u, 0x38u, 0x40u, 0x44u, 0x48u,
    0x54u, 0x58u, 0x4cu, 0x50u, 0x5cu, 0x60u
  };
  static const int32_t values[SF_PLAYER_INITIAL_PARAMETER_COUNT] = {
    100, 128, 200, 150, 12, 13, 14, 15, 16, 17, 18, 19, 20
  };
  uint8_t index;
  memset(record, 0, SF_SAVED_PLAYER_RECORD_SIZE);
  memcpy(record, "Save Hero", 9u);
  store_i32(record, 0x18u, 0);
  store_i32(record, 0x1cu, 16);
  store_i32(record, 0x24u, 4);
  store_i32(record, 0x34u, 123);
  store_i32(record, 0x3cu, 77);
  store_i32(record, 0x64u, 10000);
  store_i32(record, 0x68u, -10000);
  store_i32(record, 0xd8u, 42);
  store_i32(record, 0x140u, 0);
  store_i32(record, 0x144u, 4);
  store_i32(record, 0x148u, 12);
  store_i32(record, 0x14cu, 0);
  for (index = 0u; index < SF_PLAYER_INITIAL_PARAMETER_COUNT; ++index)
    store_i32(record, offsets[index], values[index]);
}

static void build_substitution(uint8_t table[256], uint8_t inverse[256]) {
  static const char hex[] =
    "be66b32f016e6dc81f98a546765c3d0eaa5e9dffeaa00d4b75f661855dbbdcfb"
    "8bc34f450490811e6bc9d373c6e724ba32f3c0ec57ccc4b6c1aeaf88f284ce4a"
    "fc3c9f1a56c5e2f547d9d78ccd97f07b3106e514e6da4826ac879ad8a6eb92cf0"
    "f9441b4742ad1701cd4b0c20908169bfd771d219e3635533ed0d562585f637cb58"
    "d2bd289b799a1306554409671febff4a95bf722605a6ffa1b79e917b1009c7e522"
    "9122c78059155e3a2b9f8509513807f1127cb374e5115efa7724d8349a469de20"
    "a367df1042396c2dc723e4ddedd6f959b2ad6a7dbceee03a3fca4c2568931833"
    "280b07038202438a86db383419642e7aabf1e8440cb88fa80a8ebde13b";
  uint16_t index;
  for (index = 0u; index < 256u; ++index) {
    const char high = hex[index * 2u];
    const char low = hex[index * 2u + 1u];
    table[index] = (uint8_t) (
      ((high >= 'a' ? high - 'a' + 10 : high - '0') << 4u) |
      (low >= 'a' ? low - 'a' + 10 : low - '0'));
  }
  for (index = 0u; index < 256u; ++index) inverse[table[index]] = (uint8_t) index;
}

static bool write_fixture(
    const char *path, const uint8_t record[SF_SAVED_PLAYER_RECORD_SIZE],
    const FixtureBytes *payload) {
  static const uint8_t signature[16] = {
    'S', 'h', 'a', 'd', 'o', 'w', 'F', 'l',
    'a', 'r', 'e', '0', '0', '0', '5', 0
  };
  uint8_t table[256];
  uint8_t inverse[256];
  uint32_t checksum = 0u;
  uint8_t key = 0x5au;
  uint8_t header[9];
  size_t index;
  FILE *file;
  build_substitution(table, inverse);
  for (index = 0u; index < payload->size; ++index)
    checksum += payload->bytes[index] < 128u ? payload->bytes[index] :
      (uint32_t) ((int32_t) payload->bytes[index] - 256);
  store_i32(header, 0u, (int32_t) payload->size);
  header[4] = key;
  store_i32(header, 5u, (int32_t) checksum);
  file = fopen(path, "wb");
  if (!file) return false;
  if (fwrite(signature, 1u, sizeof(signature), file) != sizeof(signature) ||
      fwrite(record, 1u, SF_SAVED_PLAYER_RECORD_SIZE, file) !=
        SF_SAVED_PLAYER_RECORD_SIZE ||
      fwrite(header, 1u, sizeof(header), file) != sizeof(header)) goto failed;
  for (index = 0u; index < payload->size; ++index) {
    const uint8_t encoded = inverse[payload->bytes[index] ^ key];
    if (fwrite(&encoded, 1u, 1u, file) != 1u) goto failed;
  }
  return fclose(file) == 0;
failed:
  fclose(file);
  return false;
}

static SfItemGroundDefinition definition(
    uint8_t category, int32_t id, int32_t subtype,
    int32_t width, int32_t height) {
  SfItemGroundDefinition item;
  memset(&item, 0, sizeof(item));
  item.category = category;
  item.definition_id = id;
  item.subtype = subtype;
  item.inventory_width = width;
  item.inventory_height = height;
  item.required_level = 1;
  item.maximum_durability = 100;
  return item;
}

static void make_progress_script(SfScsScript *script) {
  memset(script, 0, sizeof(*script));
  script->temporary_flags[0] = (SfScsFlag) {123, 0};
  script->temporary_flag_count = 1u;
  script->statuses[0] = (SfScsStatus) {5, 0, 0, false};
  script->status_count = 1u;
  script->sentences[0] = (SfScsSentence) {0u, 1u};
  script->sentence_count = 1u;
  script->commands[0] = (SfScsCommand) {1, 0u, 2u};
  script->command_count = 1u;
  script->operands[0] = (SfScsOperand) {4, 123};
  script->operands[1] = (SfScsOperand) {11, 3};
  script->operand_count = 2u;
}

int main(int argument_count, char **arguments) {
  static const char path[] = "shadowflare_save_player_fixture.Ssv";
  uint8_t record[SF_SAVED_PLAYER_RECORD_SIZE];
  FixtureBytes payload = {{0}, 0u};
  SfSavedPlayer saved;
  SfSavedGame saved_game;
  SfItemReference required[64];
  uint8_t required_count;
  SfItemGroundDefinition definitions[5];
  SfPlayerState player;
  SfWorldState world;
  SfMctScenario scenario;
  SfScsScript script;
  const SfInventoryItem *item;
  int8_t item_index;
  int result = 1;
  if (argument_count == 2 || argument_count == 3) {
    SfSavePayloadReader reader;
    uint8_t header_record[SF_SAVE_PLAYER_RECORD_SIZE];
    bool envelope;
    if (!sf_save_payload_open(
          &reader, arguments[1], header_record, &envelope)) return 1;
    printf(
      "payload %u, content %u, extension %u v%u\n",
      reader.remaining, sf_save_payload_content_remaining(&reader),
      reader.extension_size, reader.extension_version);
    sf_save_payload_close(&reader);
    if (!sf_save_game_load_path(arguments[1], &saved_game)) {
      fprintf(stderr, "Could not decode %s\n", arguments[1]);
      return 1;
    }
    saved = saved_game.player;
    if (!sf_saved_player_required_items(
          &saved, required, 64u, &required_count)) return 1;
    if (argument_count == 3) {
      SfGameplayAssets assets;
      SfArena arena;
      uint8_t parts[2] = {0u, 1u};
      sf_arena_init(
        &arena, gameplay_memory.bytes, sizeof(gameplay_memory.bytes));
      sf_world_state_init(
        &world, 0, 0, saved.gender == 1 ? 1u : 0u);
      if (required_count > SF_GROUND_ITEM_DEFINITION_LIMIT ||
          !sf_world_prepare_save_load(&world, &saved_game) ||
          !sf_gameplay_assets_load(
            &assets, arguments[2], world.scenario_id, world.entry_key,
            saved.gender == 1 ? 1u : 0u, saved.level,
            saved.companion_type, saved.companion_level,
            parts, 2u, NULL, 0u,
            required, required_count, &arena)) {
        fprintf(stderr, "Could not load gameplay assets for %s\n", saved.name);
        return 1;
      }
      sf_player_init(&player, saved.gender == 1 ? 1u : 0u);
      if (!sf_player_restore_save(
            &player, &saved, assets.ground_items.definitions,
            assets.ground_items.definition_count,
            assets.player_parameters.experience_threshold)) {
        fprintf(stderr, "Could not restore gameplay owners for %s\n", saved.name);
        return 1;
      }
      sf_world_state_init(&world, 0, 0, player.gender);
      world.player = player;
      if (!sf_world_prepare_save_load(&world, &saved_game) ||
          !sf_world_bind_saved_scenario(
            &world, &assets.scenario, assets.script, &saved_game)) {
        fprintf(stderr, "Could not restore world progress for %s\n", saved.name);
        return 1;
      }
    }
    {
      uint8_t learned = 0u;
      uint8_t spell;
      for (spell = 0u; spell < SF_SAVED_SPELL_COUNT; ++spell)
        if (saved_game.magic.availability[spell] == 3) ++learned;
      printf(
      "%s: level %d, %u backpack, %u belt, %u special, %u definitions, "
      "flags %u/%u/%u, %u spells, mines %d, world %d:%d, run %d\n",
      saved.name, saved.level, saved.backpack_count, saved.belt_count,
      saved.special_item_count, required_count, saved_game.progress.quest_count,
      saved_game.progress.transport_count, saved_game.progress.script_count,
      learned, saved_game.world.mine_count, saved_game.world.scenario_id,
      saved_game.world.entry_value, saved_game.world.running ? 1 : 0);
    }
    return 0;
  }
  make_record(record);
  if (!append_bytes(&payload, record, sizeof(record)) ||
      !append_equipment(&payload) ||
      !append_i32(&payload, 2) ||
      !append_item(&payload, 3, 0, 0, 0, true, 0, 1) ||
      !append_item(&payload, 4, 0, 2, 1, true, 0, 455) ||
      !append_i32(&payload, 1) ||
      !append_item(&payload, 3, 10000000, 3, 1, true, 0, 1) ||
      !append_i32(&payload, 1) ||
      !append_item(&payload, 4, 0, 3, 2, true, 0, 75) ||
      !append_progress(&payload) ||
      !write_fixture(path, record, &payload)) {
    fprintf(stderr, "Could not create the retail save fixture\n");
    goto done;
  }
  if (!sf_save_game_load_path(path, &saved_game)) {
    fprintf(stderr, "The retail game stream was not restored\n");
    goto done;
  }
  saved = saved_game.player;
  if (
      strcmp(saved.name, "Save Hero") != 0 || saved.gender != 0 ||
      saved.job != 16 || saved.level != 4 || saved.current_life != 123 ||
      saved.current_mana != 77 || saved.experience != 42 ||
      saved.element_x != 10000 || saved.element_y != -10000 ||
      saved.companion_type != 0 || saved.companion_level != 4 ||
      saved.companion_experience != 12 ||
      saved.backpack_count != 2u || saved.belt_count != 1u ||
      saved.special_item_count != 1u ||
      !saved.equipment[9].present || saved.equipment[9].durability != 40 ||
      !saved_game.progress.present || saved_game.progress.quest_count != 3u ||
      saved_game.progress.transport_count != 2u ||
      saved_game.progress.script_count != 4u ||
      saved_game.progress.script_values[3] != 9 ||
      !saved_game.magic.present || saved_game.magic.availability[0] != 3 ||
      saved_game.magic.levels[0] != 4 ||
      saved_game.magic.experience[0] != 5 ||
      saved_game.magic.bar_slots[0] != 0 ||
      !saved_game.companions.present || saved_game.companions.count != 6u ||
      saved_game.companions.levels[0] != 4 ||
      saved_game.companions.experience[0] != 12 ||
      !saved_game.world.present || !saved_game.world.running ||
      saved_game.world.mine_count != 7 || saved_game.world.scenario_id != 0 ||
      saved_game.world.entry_value != 0 ||
      !sf_saved_player_required_items(
        &saved, required, 8u, &required_count) || required_count != 5u) {
    fprintf(stderr, "The retail player/item stream was not restored\n");
    goto done;
  }
  definitions[0] = definition(0u, 0, 0, 1, 4);
  definitions[1] = definition(1u, 0, 1, 3, 3);
  definitions[2] = definition(3u, 0, 0, 1, 1);
  definitions[3] = definition(4u, 0, 0, 1, 1);
  definitions[4] = definition(3u, 10000000, 0, 1, 1);
  sf_player_init(&player, 1u);
  if (!sf_player_restore_save(&player, &saved, definitions, 5u, 100) ||
      !sf_player_restore_magic(&player, &saved_game.magic) ||
      !sf_player_restore_companions(
        &player, &saved, &saved_game.companions) ||
      strcmp(player.name, "Save Hero") != 0 || player.gender != 0u ||
      player.job != 16 || player.level != 4 || player.current_life != 123 ||
      player.current_mana != 77 || player.experience != 42 ||
      player.element_x != 10000 || player.element_y != -10000 ||
      !sf_player_magic_learned(&player.magic, 0) ||
      player.magic.levels[0] != 4 || player.magic.experience[0] != 5 ||
      player.magic.bar_slots[0] != 0 ||
      player.companions.type != 0 ||
      sf_player_companion_level(&player.companions) != 4 ||
      player.companions.experience[0] != 12 ||
      !player.loadout_initialized || player.inventory.count != 2u ||
      player.belt.count != 1u || player.special_items.count != 1u ||
      !(player.equipment.occupied &
        (uint16_t) (1u << SF_EQUIPMENT_ALTERNATE_MAIN_HAND))) {
    fprintf(stderr, "The decoded save did not populate player owners\n");
    goto done;
  }
  sf_world_state_init(&world, 0, 0, player.gender);
  world.player = player;
  memset(&scenario, 0, sizeof(scenario));
  make_progress_script(&script);
  if (!sf_world_prepare_save_load(&world, &saved_game) ||
      world.player.mine_count != 7 ||
      world.player.pace != SF_PLAYER_PACE_RUN ||
      !sf_world_bind_saved_scenario(
        &world, &scenario, &script, &saved_game) ||
      world.actor_script_state.temporary_values[0] != 9 ||
      world.actor_script_state.progress.quest_values[2] != 3 ||
      world.actor_script_state.progress.persistent_values[3] != 9) {
    fprintf(stderr, "Saved progress did not reach the world owner\n");
    goto done;
  }
  item_index = sf_inventory_item_at(&player.inventory, 2u, 1u);
  item = item_index >= 0 ? &player.inventory.items[(uint8_t) item_index] : NULL;
  if (!item || item->category != 4u || item->quantity != 455 ||
      !sf_belt_item_at(&player.belt, 3u, 1u) ||
      sf_special_items_item_at(&player.special_items, 3u, 2u) < 0 ||
      player.special_items.items[0].quantity != 75) {
    fprintf(stderr, "Saved backpack or belt placement changed\n");
    goto done;
  }
  saved.current_life = 0;
  saved.current_mana = 0;
  if (!sf_player_restore_save(&player, &saved, definitions, 5u, 100) ||
      player.current_life != 200 || player.current_mana != 150) {
    fprintf(stderr, "A dead legacy save was not repaired on entry\n");
    goto done;
  }
  result = 0;
done:
  (void) remove(path);
  return result;
}
