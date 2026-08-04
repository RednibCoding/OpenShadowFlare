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
#include "game/equipment.h"
#include "game/world.h"
#include "render/renderer.h"
#include "screens/gameplay_player.h"
#include "ui/gameplay_equipment_layout.h"
#include "ui/gameplay_inventory.h"
#include "ui/gameplay_inventory_input.h"

#include <stdio.h>
#include <string.h>

typedef union SfEquipmentTestMemory {
  long double alignment;
  void *pointer;
  uint8_t bytes[SF_MAIN_ARENA_BYTES];
} SfEquipmentTestMemory;

static SfEquipmentTestMemory sf_equipment_test_memory;
static uint16_t sf_equipment_pixels[640u * 480u];
static uint16_t sf_equipment_comparison[640u * 480u];

static SfInventoryItem test_item(
    uint8_t category, int32_t id, uint8_t width, uint8_t height) {
  const SfInventoryItem item = {
    id, 1, 100, category, 0u, 0u, width, height, true};
  return item;
}

static int test_equipment_rules(void) {
  SfEquipmentState equipment;
  SfEquipmentPlacement placement;
  SfItemGroundDefinition definitions[3];
  SfInventoryItem taken;
  SfInventoryItem replacement;
  memset(definitions, 0, sizeof(definitions));
  definitions[0].category = 0u;
  definitions[0].definition_id = 1;
  definitions[0].subtype = 1;
  definitions[0].required_level = 1;
  definitions[0].weight = 30;
  definitions[0].appearance_part = 12;
  definitions[0].suppresses_off_hand = true;
  definitions[1].category = 1u;
  definitions[1].definition_id = 2;
  definitions[1].subtype = 2;
  definitions[1].required_level = 1;
  definitions[1].weight = 40;
  definitions[1].appearance_part = 9;
  definitions[2].category = 2u;
  definitions[2].definition_id = 3;
  definitions[2].inventory_width = 1;
  definitions[2].required_level = 2;
  sf_equipment_init(&equipment);
  placement = sf_equipment_place(
    &equipment, SF_EQUIPMENT_HELMET,
    test_item(1u, 2, 2u, 2u), &definitions[1], 1);
  if (placement.accepted) {
    fprintf(stderr, "A shield entered the helmet slot\n");
    return 1;
  }
  placement = sf_equipment_place(
    &equipment, SF_EQUIPMENT_ACCESSORY_1,
    test_item(2u, 3, 1u, 1u), &definitions[2], 1);
  if (placement.accepted) {
    fprintf(stderr, "Equipment ignored its required level\n");
    return 1;
  }
  if (!sf_equipment_place(
        &equipment, SF_EQUIPMENT_MAIN_HAND,
        test_item(0u, 1, 1u, 4u), &definitions[0], 1).accepted ||
      !sf_equipment_place(
        &equipment, SF_EQUIPMENT_OFF_HAND,
        test_item(1u, 2, 2u, 2u), &definitions[1], 1).accepted ||
      sf_equipment_total_weight(&equipment, definitions, 3u) != 30 ||
      sf_equipment_part_definition(
        &equipment, definitions, 3u, 9u) != NULL) {
    fprintf(stderr, "A two-handed weapon did not suppress its off hand\n");
    return 1;
  }
  if (!sf_equipment_take(
        &equipment, SF_EQUIPMENT_MAIN_HAND, &taken) ||
      taken.definition_id != 1 ||
      sf_equipment_total_weight(&equipment, definitions, 3u) != 40) {
    fprintf(stderr, "Taking equipment did not restore the off hand\n");
    return 1;
  }
  if (!sf_equipment_place(
        &equipment, SF_EQUIPMENT_MAIN_HAND,
        test_item(0u, 1, 1u, 4u), &definitions[0], 1).accepted)
    return 1;
  replacement = test_item(0u, 1, 1u, 4u);
  replacement.durability = 77;
  placement = sf_equipment_place(
    &equipment, SF_EQUIPMENT_MAIN_HAND,
    replacement, &definitions[0], 1);
  if (!placement.accepted || !placement.holding_item ||
      placement.held_item.durability != 100 ||
      sf_equipment_item(
        &equipment, SF_EQUIPMENT_MAIN_HAND)->durability != 77) {
    fprintf(stderr, "Equipment replacement did not swap with the pointer\n");
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

static int test_live_equipment(const char *root, SfArena *arena) {
  SfGameplayAssets assets;
  SfGameplayInventoryUi ui;
  SfPlayerState loader_player;
  SfWorldState world;
  SfWorldRenderView view;
  SfGameInput input;
  SfRenderer renderer;
  const SfItemGroundDefinition *leather;
  const SfItemGroundDefinition *short_sword;
  const SfItemGroundDefinition *round_shield;
  const SfInventoryItem *equipped;
  SfWorldPoint destination;
  int origin_x = 0;
  int origin_y = 0;
  uint8_t part;
  bool leather_part_loaded = false;
  sf_player_init(&loader_player, 1u);
  if (!sf_gameplay_assets_load(
        &assets, root, 0, 0, loader_player.gender,
        loader_player.appearance_parts, loader_player.appearance_part_count,
        loader_player.visible_items, loader_player.visible_item_count, arena))
    return 1;
  leather = find_definition(&assets, 1u, 0);
  short_sword = find_definition(&assets, 0u, 0);
  round_shield = find_definition(&assets, 1u, 1000000);
  if (!leather || leather->appearance_part != 5 ||
      leather->appearance_red_strength != 1000 ||
      leather->maximum_durability != 100 || !short_sword ||
      short_sword->subtype != 0 || short_sword->appearance_part != 12 ||
      round_shield == NULL || round_shield->subtype != 2 ||
      round_shield->appearance_part != 9 ||
      round_shield->appearance_red_strength != 900 ||
      round_shield->appearance_green_strength != 800 ||
      round_shield->appearance_blue_strength != 500) return 1;
  for (part = 0u; part < assets.player.animations[0][1].part_count; ++part)
    if (assets.player.animations[0][1].parts[part].source_index == 5u)
      leather_part_loaded = true;
  if (!leather_part_loaded) {
    fprintf(stderr, "The starter body animation part was not retained\n");
    return 1;
  }
  sf_world_state_init(&world, 0, 0, loader_player.gender);
  if (!sf_player_apply_initial_parameters(
        &world.player, &assets.player_parameters)) return 1;
  sf_world_state_bind_collision(&world, &assets.ground, &assets.objects);
  sf_world_state_bind_ground_items(
    &world, assets.ground_items.definitions,
    assets.ground_items.definition_count);
  sf_world_state_enter(
    &world, assets.entry.world_x, assets.entry.world_y,
    (uint8_t) assets.entry.direction);
  equipped = sf_equipment_item(&world.player.equipment, SF_EQUIPMENT_BODY);
  if (!equipped || equipped->definition_id != 0 ||
      equipped->durability != 100 || !equipped->identified ||
      sf_equipment_total_weight(
        &world.player.equipment, assets.ground_items.definitions,
        assets.ground_items.definition_count) != 70) {
    fprintf(stderr, "The new hero did not own the retail starter body\n");
    return 1;
  }
  sf_gameplay_equipment_item_origin(
    SF_EQUIPMENT_BODY, equipped, &origin_x, &origin_y);
  if (origin_x != 560 || origin_y != 88 ||
      sf_gameplay_equipment_slot_at(580, 120) != SF_EQUIPMENT_BODY) {
    fprintf(stderr, "The retail equipment regions are misplaced\n");
    return 1;
  }
  if (!sf_renderer_init(
        &renderer, sf_equipment_pixels, sizeof(sf_equipment_pixels),
        640u, 480u)) return 1;
  sf_world_render_view(&world, 1000u, &view);
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_player_draw(
    &renderer, &assets.player, &world, &view, false, NULL);
  memcpy(sf_equipment_comparison, sf_equipment_pixels,
    sizeof(sf_equipment_pixels));
  sf_gameplay_inventory_init(&ui);
  ui.open = true;
  destination = world.player.destination;
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_primary_down = true;
  input.pointer_x = 580;
  input.pointer_y = 120;
  (void) sf_gameplay_inventory_input_resolve(
    &ui, &world.player, false, &input);
  if (input.inventory_action != SF_INVENTORY_ACTION_TAKE_EQUIPMENT ||
      input.equipment_slot != SF_EQUIPMENT_BODY ||
      !input.pointer_over_gameplay_ui) return 1;
  sf_world_state_update(&world, &input);
  if (!world.player.inventory_transfer.holding_item ||
      sf_equipment_item(&world.player.equipment, SF_EQUIPMENT_BODY) ||
      world.player.visible_item_count != 0u ||
      world.player.destination.x != destination.x ||
      world.player.destination.y != destination.y ||
      world.ground_items.sound_count != 1u ||
      world.ground_items.sound_samples[0] != 47u) {
    fprintf(stderr, "Taking equipped body armor lost state or leaked input\n");
    return 1;
  }
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_player_draw(
    &renderer, &assets.player, &world, &view, false, NULL);
  if (memcmp(
        sf_equipment_comparison, sf_equipment_pixels,
        sizeof(sf_equipment_pixels)) == 0) {
    fprintf(stderr, "Taking body armor did not change player appearance\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_primary_down = true;
  input.pointer_x = 580;
  input.pointer_y = 120;
  (void) sf_gameplay_inventory_input_resolve(
    &ui, &world.player, false, &input);
  if (input.inventory_action != SF_INVENTORY_ACTION_PLACE_EQUIPMENT)
    return 1;
  sf_world_state_update(&world, &input);
  if (world.player.inventory_transfer.holding_item ||
      !sf_equipment_item(&world.player.equipment, SF_EQUIPMENT_BODY) ||
      world.player.visible_item_count != 1u ||
      world.ground_items.sound_count != 1u ||
      world.ground_items.sound_samples[0] != 49u) {
    fprintf(stderr, "Equipping body armor did not use the retail transfer\n");
    return 1;
  }
  return 0;
}

int main(void) {
  if (test_equipment_rules()) return 1;
#if defined(OPENSHADOWFLARE_SOURCE_DIR)
  {
    SfArena arena;
    char root[1024];
    char probe_path[2048];
    FILE *probe;
    (void) snprintf(
      root, sizeof(root), "%s/tmp/ShadowFlare", OPENSHADOWFLARE_SOURCE_DIR);
    (void) snprintf(
      probe_path, sizeof(probe_path),
      "%s/System/Game/Parameter/Item.Ibn", root);
    probe = fopen(probe_path, "rb");
    if (!probe) return 0;
    fclose(probe);
    sf_arena_init(
      &arena, sf_equipment_test_memory.bytes,
      sizeof(sf_equipment_test_memory.bytes));
    if (test_live_equipment(root, &arena)) return 1;
  }
#endif
  return 0;
}
