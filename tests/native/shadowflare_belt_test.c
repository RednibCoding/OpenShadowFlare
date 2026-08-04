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
#include "core/memory_budget.h"
#include "game/belt.h"
#include "game/player_item.h"
#include "game/world.h"
#include "render/renderer.h"
#include "ui/gameplay_belt.h"
#include "ui/gameplay_inventory.h"
#include "ui/gameplay_inventory_input.h"

#include <stdio.h>
#include <string.h>

typedef union SfBeltTestMemory {
  long double alignment;
  void *pointer;
  uint8_t bytes[SF_MAIN_ARENA_BYTES];
} SfBeltTestMemory;

static SfBeltTestMemory sf_belt_test_memory;
static uint16_t sf_belt_test_pixels[640u * 480u];

static SfInventoryItem test_item(
    uint8_t category, int32_t definition_id,
    uint8_t width, uint8_t height, int32_t durability) {
  return (SfInventoryItem) {
    definition_id, 1, durability, category, 0u, 0u,
    width, height, true};
}

static SfItemGroundDefinition test_definition(
    uint8_t category, int32_t definition_id,
    int32_t width, int32_t height) {
  SfItemGroundDefinition result;
  memset(&result, 0, sizeof(result));
  result.category = category;
  result.definition_id = definition_id;
  result.inventory_width = width;
  result.inventory_height = height;
  return result;
}

static int test_belt_transactions(void) {
  const SfItemGroundDefinition wide = test_definition(3u, 10, 2, 1);
  const SfItemGroundDefinition tablet = test_definition(3u, 20, 1, 1);
  const SfItemGroundDefinition weapon = test_definition(0u, 30, 1, 1);
  SfBeltState belt;
  SfBeltState unchanged;
  SfInventoryPlacement placement;
  SfInventoryItem held;
  sf_belt_init(&belt);
  if (!sf_belt_place(
        &belt, test_item(3u, 10, 2u, 1u, 111), 0, 0, &wide).accepted ||
      !sf_belt_place(
        &belt, test_item(3u, 10, 2u, 1u, 222), 2, 0, &wide).accepted)
    return 1;
  unchanged = belt;
  placement = sf_belt_place(
    &belt, test_item(3u, 10, 2u, 1u, 333), 1, 0, &wide);
  if (placement.accepted || memcmp(&belt, &unchanged, sizeof(belt)) != 0) {
    fprintf(stderr, "A belt placement changed two overlapping items\n");
    return 1;
  }
  placement = sf_belt_place(
    &belt, test_item(3u, 20, 1u, 1u, 444), 0, 0, &tablet);
  if (!placement.accepted || !placement.holding_item ||
      placement.held_item.durability != 111 || belt.count != 2u ||
      !sf_belt_item_at(&belt, 0u, 0u)) {
    fprintf(stderr, "A belt swap lost its concrete item instance\n");
    return 1;
  }
  unchanged = belt;
  if (sf_belt_place(
        &belt, test_item(0u, 30, 1u, 1u, 1), 0, 1, &weapon).accepted ||
      memcmp(&belt, &unchanged, sizeof(belt)) != 0 ||
      !sf_belt_take_at(&belt, 3u, 0u, &held) || held.durability != 222) {
    fprintf(stderr, "The belt accepted a non-medicine or lost a wide item\n");
    return 1;
  }
  return 0;
}

static const SfItemGroundDefinition *find_definition(
    const SfGameplayAssets *assets, uint8_t category, int32_t id) {
  uint8_t index;
  for (index = 0u; index < assets->ground_items.definition_count; ++index) {
    const SfItemGroundDefinition *definition =
      &assets->ground_items.definitions[index];
    if (definition->category == category && definition->definition_id == id)
      return definition;
  }
  return NULL;
}

static void belt_pointer_input(
    SfGameInput *input, int16_t x, int16_t y, bool secondary) {
  memset(input, 0, sizeof(*input));
  input->pointer_active = true;
  input->pointer_x = x;
  input->pointer_y = y;
  if (secondary) input->pointer_secondary_pressed = true;
  else {
    input->pointer_primary_pressed = true;
    input->pointer_primary_down = true;
  }
}

static void belt_key_input(SfGameInput *input, int8_t pocket) {
  memset(input, 0, sizeof(*input));
  input->belt_pocket_key_pressed = true;
  input->belt_pocket_pressed = pocket;
}

static int test_live_belt(const char *root, SfArena *arena) {
  SfGameplayAssets assets;
  SfPlayerState loader_player;
  SfItemReference retained[SF_GROUND_ITEM_DEFINITION_LIMIT];
  uint8_t retained_count;
  SfWorldState world;
  SfGameplayInventoryUi inventory;
  SfRenderer renderer;
  SfGameInput input;
  SfWorldPoint destination;
  const SfItemGroundDefinition *tablet;
  const SfItemGroundDefinition *capsule;
  const SfPcmU8 *medicine_sound;
  uint8_t x;
  uint8_t y;
  uint8_t row;
  size_t changed = 0u;
  size_t pixel;

  sf_player_init(&loader_player, 1u);
  if (!sf_player_required_item_definitions(
        &loader_player, retained, SF_GROUND_ITEM_DEFINITION_LIMIT,
        &retained_count) || retained_count >= SF_GROUND_ITEM_DEFINITION_LIMIT)
    return 1;
  retained[retained_count++] = (SfItemReference) {1, 4u};
  if (!sf_gameplay_assets_load(
        &assets, root, 0, 0, loader_player.gender, loader_player.level,
        loader_player.companions.type,
        sf_player_companion_level(&loader_player.companions),
        loader_player.appearance_parts, loader_player.appearance_part_count,
        loader_player.visible_items, loader_player.visible_item_count,
        retained, retained_count, arena)) {
    fprintf(stderr, "The belt fixture did not fit the game arena\n");
    return 1;
  }
  tablet = find_definition(&assets, 3u, 0);
  capsule = find_definition(&assets, 3u, 10000000);
  medicine_sound = sf_ground_item_sound(&assets.ground_items, 16u);
  if (!tablet || strcmp(tablet->name, "Tablet") != 0 ||
      tablet->restore_life != 200 || tablet->inventory_width != 1 ||
      tablet->inventory_height != 1 || !capsule ||
      strcmp(capsule->name, "Capsule") != 0 ||
      capsule->restore_mana != 200 || !medicine_sound ||
      medicine_sound->sample_rate != 12000u ||
      medicine_sound->frame_count != 2488u) {
    fprintf(stderr, "Retail medicine data or its 16-bit sound decoded wrong\n");
    return 1;
  }

  sf_world_state_init(&world, 0, 0, loader_player.gender);
  if (!sf_player_apply_initial_parameters(
        &world.player, &assets.player_parameters)) return 1;
  sf_world_state_bind_collision(&world, &assets.ground, &assets.objects);
  if (!sf_world_state_bind_ground_items(
        &world, assets.ground_items.definitions,
        assets.ground_items.definition_count)) return 1;
  sf_world_state_enter(
    &world, assets.entry.world_x, assets.entry.world_y,
    (uint8_t) assets.entry.direction);
  if (world.player.inventory.count != 8u || world.player.belt.count != 8u ||
      world.player.mine_count != 5 || world.player.maximum_mines != 10 ||
      !sf_equipment_item(&world.player.equipment, SF_EQUIPMENT_BODY)) {
    fprintf(stderr, "The retail new-player loadout was not initialized\n");
    return 1;
  }
  for (row = 0u; row < 4u; ++row) {
    const SfInventoryItem *belt_tablet = sf_belt_item_at(
      &world.player.belt, row, 0u);
    const SfInventoryItem *belt_capsule = sf_belt_item_at(
      &world.player.belt, row, 1u);
    if (!belt_tablet || belt_tablet->definition_id != 0 ||
        !belt_capsule || belt_capsule->definition_id != 10000000 ||
        sf_inventory_item_at(&world.player.inventory, 0u, row) < 0 ||
        sf_inventory_item_at(&world.player.inventory, 1u, row) < 0) {
      fprintf(stderr, "Starter medicine is not in the retail cells\n");
      return 1;
    }
  }
  if (!sf_gameplay_belt_pocket_at(357, 413, &x, &y) || x != 0u || y != 0u ||
      !sf_gameplay_belt_pocket_at(532, 476, &x, &y) || x != 3u || y != 1u ||
      sf_gameplay_belt_pocket_at(404, 445, &x, &y) ||
      sf_gameplay_belt_pocket_at(533, 476, &x, &y)) {
    fprintf(stderr, "The retail belt pocket hit rectangles changed\n");
    return 1;
  }

  if (!sf_renderer_init(
        &renderer, sf_belt_test_pixels,
        sizeof(sf_belt_test_pixels), 640u, 480u)) return 1;
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_belt_draw(&renderer, &assets, &world.player, NULL);
  for (pixel = 0u; pixel < 640u * 480u; ++pixel)
    if (sf_belt_test_pixels[pixel] != 0x1234u) ++changed;
  if (changed < 50u) {
    fprintf(stderr, "Belt item icons were not drawn\n");
    return 1;
  }

  sf_gameplay_inventory_init(&inventory);
  destination = world.player.destination;
  belt_pointer_input(&input, 361, 417, false);
  (void) sf_gameplay_inventory_input_resolve(
    &inventory, &world.player, false, &input);
  if (input.inventory_action != SF_INVENTORY_ACTION_TAKE_BELT ||
      !input.pointer_over_gameplay_ui) return 1;
  sf_world_state_update(&world, &input);
  if (!world.player.inventory_transfer.holding_item ||
      world.player.belt.count != 7u ||
      world.ground_items.sound_count != 1u ||
      world.ground_items.sound_samples[0] != 48u ||
      world.player.destination.x != destination.x ||
      world.player.destination.y != destination.y) {
    fprintf(stderr, "Taking a belt item leaked into world movement\n");
    return 1;
  }
  belt_pointer_input(&input, 361, 417, false);
  (void) sf_gameplay_inventory_input_resolve(
    &inventory, &world.player, false, &input);
  sf_world_state_update(&world, &input);
  if (world.player.inventory_transfer.holding_item ||
      world.player.belt.count != 8u ||
      world.ground_items.sound_count != 1u ||
      world.ground_items.sound_samples[0] != 48u) return 1;

  belt_key_input(&input, 0);
  (void) sf_gameplay_inventory_input_resolve(
    &inventory, &world.player, false, &input);
  sf_world_state_update(&world, &input);
  if (world.player.belt.count != 8u || world.ground_items.sound_count != 0u)
    return 1;
  world.player.current_life -= 10;
  belt_key_input(&input, 0);
  (void) sf_gameplay_inventory_input_resolve(
    &inventory, &world.player, false, &input);
  sf_world_state_update(&world, &input);
  if (world.player.belt.count != 7u ||
      world.player.current_life != world.player.initial_parameters.values[2] ||
      world.ground_items.sound_count != 1u ||
      world.ground_items.sound_samples[0] != 16u) {
    fprintf(stderr, "A belt Tablet did not follow retail consumption rules\n");
    return 1;
  }
  world.player.current_mana -= 10;
  belt_pointer_input(&input, 409, 449, true);
  (void) sf_gameplay_inventory_input_resolve(
    &inventory, &world.player, false, &input);
  sf_world_state_update(&world, &input);
  if (world.player.belt.count != 6u ||
      world.player.current_mana != world.player.initial_parameters.values[3] ||
      world.ground_items.sound_count != 1u ||
      world.ground_items.sound_samples[0] != 16u) return 1;

  inventory.open = true;
  world.player.current_life -= 10;
  belt_pointer_input(
    &input, SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT + 4,
    SF_GAMEPLAY_INVENTORY_BACKPACK_TOP + 4, true);
  (void) sf_gameplay_inventory_input_resolve(
    &inventory, &world.player, false, &input);
  sf_world_state_update(&world, &input);
  if (world.player.inventory.count != 7u ||
      world.player.current_life != world.player.initial_parameters.values[2] ||
      world.ground_items.sound_count != 1u ||
      world.ground_items.sound_samples[0] != 16u) {
    fprintf(stderr, "A backpack Tablet did not follow retail use rules\n");
    return 1;
  }

  if (!sf_ground_items_create_instance(
        &world.ground_items, 4u, 1, 1, 0, true,
        world.player.position)) return 1;
  world.ground_items.items[0].bounce_state = 2u;
  memset(&input, 0, sizeof(input));
  input.pointer_primary_pressed = true;
  input.world_pointer_resolved = true;
  input.pointed_ground_item_id = world.ground_items.items[0].id;
  input.pointed_actor_id = -1;
  input.pointed_enemy_id = -1;
  sf_world_state_update(&world, &input);
  if (world.player.mine_count != 6 || world.ground_items.count != 0u ||
      world.ground_items.sound_count != 1u ||
      world.ground_items.sound_samples[0] != 48u) {
    fprintf(stderr, "A picked-up mine entered the bag instead of its counter\n");
    return 1;
  }
  world.player.mine_count = world.player.maximum_mines;
  if (!sf_ground_items_create_instance(
        &world.ground_items, 4u, 1, 1, 0, true,
        world.player.position)) return 1;
  world.ground_items.items[0].bounce_state = 2u;
  memset(&input, 0, sizeof(input));
  input.pointer_primary_pressed = true;
  input.world_pointer_resolved = true;
  input.pointed_ground_item_id = world.ground_items.items[0].id;
  input.pointed_actor_id = -1;
  input.pointed_enemy_id = -1;
  sf_world_state_update(&world, &input);
  if (world.ground_items.count != 1u ||
      world.ground_items.items[0].bounce_state != 0u ||
      world.ground_items.items[0].vertical_velocity != 1600 ||
      world.ground_items.sound_count != 0u) {
    fprintf(stderr, "A mine beyond the retail maximum did not bounce back\n");
    return 1;
  }
  return 0;
}

int main(void) {
#if defined(OPENSHADOWFLARE_SOURCE_DIR)
  SfArena arena;
  char root[1024];
  char probe_path[1024];
  FILE *probe;
  if (test_belt_transactions()) return 1;
  (void) snprintf(
    root, sizeof(root), "%s/tmp/ShadowFlare", OPENSHADOWFLARE_SOURCE_DIR);
  (void) snprintf(
    probe_path, sizeof(probe_path),
    "%s/tmp/ShadowFlare/System/Game/Parameter/Item.Ibn",
    OPENSHADOWFLARE_SOURCE_DIR);
  probe = fopen(probe_path, "rb");
  if (!probe) return 0;
  fclose(probe);
  sf_arena_init(
    &arena, sf_belt_test_memory.bytes, sizeof(sf_belt_test_memory.bytes));
  if (test_live_belt(root, &arena)) return 1;
#endif
  return 0;
}
