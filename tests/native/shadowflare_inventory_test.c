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
#include "game/inventory.h"
#include "game/item_condition.h"
#include "game/player_profile.h"
#include "game/world.h"
#include "render/renderer.h"
#include "ui/gameplay_inventory.h"
#include "ui/gameplay_inventory_input.h"
#include "ui/gameplay_item_condition.h"
#include "ui/gameplay_item_information.h"
#include "ui/gameplay_panels_input.h"
#include "ui/gameplay_special_items.h"
#include "ui/gameplay_status.h"

#include <stdio.h>
#include <string.h>

typedef union SfInventoryTestMemory {
  long double alignment;
  void *pointer;
  uint8_t bytes[SF_MAIN_ARENA_BYTES];
} SfInventoryTestMemory;

static SfInventoryTestMemory sf_inventory_test_memory;
static uint16_t sf_inventory_test_pixels[640u * 480u];
static uint16_t sf_inventory_test_comparison[640u * 480u];

static int test_inventory_transactions(void) {
  SfInventoryState inventory;
  SfInventoryState unchanged;
  SfInventoryItem held;
  SfInventoryPlacement placement;
  sf_inventory_init(&inventory);
  inventory.items[0] = (SfInventoryItem) {
    .definition_id = 100, .quantity = 1, .durability = 123,
    .category = 0u, .grid_x = 0u, .grid_y = 0u,
    .width = 1u, .height = 3u, .identified = true};
  inventory.items[1] = (SfInventoryItem) {
    .definition_id = 1000000, .quantity = 1, .durability = 456,
    .category = 1u, .grid_x = 2u, .grid_y = 0u,
    .width = 2u, .height = 2u};
  inventory.count = 2u;
  if (!sf_inventory_take(&inventory, 0u, &held) || inventory.count != 1u ||
      held.definition_id != 100 || held.durability != 123) {
    fprintf(stderr, "Taking an item did not preserve its concrete instance\n");
    return 1;
  }
  unchanged = inventory;
  placement = sf_inventory_place(&inventory, held, -1, 0);
  if (placement.accepted ||
      memcmp(&inventory, &unchanged, sizeof(inventory)) != 0) {
    fprintf(stderr, "An invalid placement was not transactional\n");
    return 1;
  }
  placement = sf_inventory_place(&inventory, held, 2, 0);
  if (!placement.accepted || !placement.holding_item ||
      placement.held_item.definition_id != 1000000 ||
      placement.held_item.durability != 456 || inventory.count != 1u ||
      inventory.items[0].definition_id != 100 ||
      inventory.items[0].grid_x != 2u || inventory.items[0].grid_y != 0u) {
    fprintf(stderr, "A one-item overlap did not swap onto the pointer\n");
    return 1;
  }
  sf_inventory_init(&inventory);
  inventory.items[0] = (SfInventoryItem) {
    .definition_id = 1, .quantity = 1, .category = 0u,
    .grid_x = 0u, .grid_y = 0u, .width = 1u, .height = 1u};
  inventory.items[1] = (SfInventoryItem) {
    .definition_id = 2, .quantity = 1, .category = 0u,
    .grid_x = 1u, .grid_y = 0u, .width = 1u, .height = 1u};
  inventory.count = 2u;
  unchanged = inventory;
  held = (SfInventoryItem) {
    .definition_id = 3, .quantity = 1, .category = 0u,
    .width = 2u, .height = 1u};
  placement = sf_inventory_place(&inventory, held, 0, 0);
  if (placement.accepted ||
      memcmp(&inventory, &unchanged, sizeof(inventory)) != 0) {
    fprintf(stderr, "A multi-item overlap partially changed the backpack\n");
    return 1;
  }
  sf_inventory_init(&inventory);
  inventory.items[0] = (SfInventoryItem) {
    .definition_id = 0, .quantity = 9990, .category = 4u,
    .grid_x = 0u, .grid_y = 0u, .width = 1u, .height = 1u};
  inventory.count = 1u;
  held = (SfInventoryItem) {
    .definition_id = 0, .quantity = 20, .category = 4u,
    .width = 1u, .height = 1u};
  placement = sf_inventory_place(&inventory, held, 0, 0);
  if (!placement.accepted || !placement.holding_item ||
      inventory.items[0].quantity != 10000 ||
      placement.held_item.quantity != 10) {
    fprintf(stderr, "A gold merge lost its retail cursor remainder\n");
    return 1;
  }
  return 0;
}

static int test_special_item_transactions(void) {
  SfSpecialItemState items;
  SfInventoryItem first = {
    .definition_id = 1, .quantity = 1, .category = 0u,
    .width = 2u, .height = 2u};
  SfInventoryItem second = {
    .definition_id = 2, .quantity = 1, .category = 1u,
    .width = 1u, .height = 3u};
  SfInventoryPlacement placement;
  SfInventoryItem taken;
  SfInventoryItem gold = {
    .definition_id = 0, .quantity = 9990, .category = 4u,
    .width = 1u, .height = 1u};
  sf_special_items_init(&items);
  if (!sf_special_items_place(&items, first, 7, 8).accepted ||
      sf_special_items_item_at(&items, 8u, 9u) != 0) return 1;
  placement = sf_special_items_place(&items, second, 7, 7);
  if (!placement.accepted || !placement.holding_item ||
      placement.held_item.definition_id != 1 || items.count != 1u ||
      items.items[0].definition_id != 2) {
    fprintf(stderr, "Special Item did not perform its one-item swap\n");
    return 1;
  }
  if (!sf_special_items_take(&items, 0u, &taken) ||
      taken.definition_id != 2 || items.count != 0u) return 1;
  if (!sf_special_items_place(&items, gold, 0, 0).accepted) return 1;
  gold.quantity = 20;
  placement = sf_special_items_place(&items, gold, 0, 0);
  if (!placement.accepted || !placement.holding_item ||
      items.items[0].quantity != 10000 ||
      placement.held_item.quantity != 10) {
    fprintf(stderr, "Special Item lost its Gold stack remainder\n");
    return 1;
  }
  return 0;
}

static const SfItemGroundDefinition *find_dagger(
    const SfGameplayAssets *assets) {
  uint8_t index;
  for (index = 0u; index < assets->ground_items.definition_count; ++index) {
    const SfItemGroundDefinition *item =
      &assets->ground_items.definitions[index];
    if (item->category == 0u && item->definition_id == 100) return item;
  }
  return NULL;
}

static int resolve_take(
    SfGameplayInventoryUi *ui, SfWorldState *world, SfGameInput *input,
    int cell_x) {
  memset(input, 0, sizeof(*input));
  input->pointer_active = true;
  input->pointer_primary_pressed = true;
  input->pointer_primary_down = true;
  input->pointer_x = (int16_t) (
    SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT + cell_x * 32 + 4);
  input->pointer_y = SF_GAMEPLAY_INVENTORY_BACKPACK_TOP + 4;
  (void) sf_gameplay_inventory_input_resolve(
    ui, &world->player, false, input);
  if (input->inventory_action != SF_INVENTORY_ACTION_TAKE ||
      !input->pointer_over_gameplay_ui) return 1;
  sf_world_state_update(world, input);
  return world->player.inventory_transfer.holding_item ? 0 : 1;
}

static int test_live_inventory(
    const char *root, SfArena *arena) {
  SfGameplayAssets assets;
  SfGameplayInventoryUi ui;
  SfGameplayCharacterPanelUi status;
  SfPlayerProfile profile;
  SfPlayerState loader_player;
  SfItemReference retained_items[SF_GROUND_ITEM_DEFINITION_LIMIT];
  uint8_t retained_item_count;
  SfWorldState world;
  SfRenderer renderer;
  SfGameInput input;
  SfWorldPoint destination;
  SfWorldPoint offset;
  const SfItemGroundDefinition *dagger;
  const SfGroundItem *dropped;
  SfInventoryItem condition_item;
  char information[768];
  int32_t dropped_id;
  size_t changed = 0u;
  size_t pixel;
  sf_player_init(&loader_player, 1u);
  if (!sf_player_required_item_definitions(
        &loader_player, retained_items, SF_GROUND_ITEM_DEFINITION_LIMIT,
        &retained_item_count) ||
      !sf_gameplay_assets_load(
        &assets, root, 0, 0, loader_player.gender, loader_player.level,
        loader_player.appearance_parts, loader_player.appearance_part_count,
        loader_player.visible_items, loader_player.visible_item_count,
        retained_items, retained_item_count, arena)) {
    fprintf(stderr, "Inventory fixture could not load Remote Town assets\n");
    return 1;
  }
  dagger = find_dagger(&assets);
  if (!dagger) {
    fprintf(stderr, "Inventory fixture could not find the Dagger\n");
    return 1;
  }
  if (dagger->base_price != 400 || dagger->maximum_durability != 300 ||
      dagger->parameter_bonuses[0] != 10 ||
      dagger->parameter_bonuses[1] != 120 ||
      dagger->parameter_bonuses[8] != 50 ||
      dagger->description[0] == '\0') {
    fprintf(stderr, "The streamed Dagger information is incomplete\n");
    return 1;
  }
  sf_world_state_init(&world, 0, 0, loader_player.gender);
  if (!sf_player_apply_initial_parameters(
        &world.player, &assets.player_parameters)) {
    fprintf(stderr, "Inventory fixture could not initialize the player\n");
    return 1;
  }
  sf_world_state_bind_collision(&world, &assets.ground, &assets.objects);
  sf_world_state_bind_ground_items(
    &world, assets.ground_items.definitions,
    assets.ground_items.definition_count);
  sf_inventory_init(&world.player.inventory);
  sf_world_state_enter(
    &world, assets.entry.world_x, assets.entry.world_y,
    (uint8_t) assets.entry.direction);
  if (!sf_inventory_store(&world.player.inventory, dagger, 1)) {
    fprintf(stderr, "Inventory fixture could not store its Dagger\n");
    return 1;
  }
  sf_gameplay_inventory_init(&ui);
  sf_gameplay_character_panel_init(&status);
  ui.open = true;
  sf_player_profile_build(
    &world.player, assets.ground_items.definitions,
    assets.ground_items.definition_count, &profile);
  if (profile.physical_attack !=
        world.player.initial_parameters.values[5] ||
      profile.maximum_life != world.player.initial_parameters.values[2]) {
    fprintf(stderr, "The base player profile changed without equipment\n");
    return 1;
  }
  if (!sf_gameplay_item_information_text(
        information, sizeof(information), &world.player.inventory.items[0],
        dagger) || strstr(information, "[Dagger]\n\n") != information ||
      !strstr(information, "Attack                    :       10\n") ||
      !strstr(information, "Sale Price                :      100\n")) {
    fprintf(stderr, "The Dagger information text does not match retail\n");
    return 1;
  }
  condition_item = world.player.inventory.items[0];
  condition_item.durability = 30;
  if (sf_item_condition_warning_visible(&condition_item, dagger, 0u) ||
      sf_item_condition_warning_blinks(&condition_item, dagger)) {
    fprintf(stderr, "Ten-percent durability incorrectly shows a warning\n");
    return 1;
  }
  condition_item.durability = 29;
  if (!sf_item_condition_warning_visible(&condition_item, dagger, 0u) ||
      !sf_item_condition_warning_visible(&condition_item, dagger, 7u) ||
      sf_item_condition_warning_visible(&condition_item, dagger, 8u) ||
      !sf_item_condition_warning_blinks(&condition_item, dagger)) {
    fprintf(stderr, "The low-condition warning does not blink 8-on/8-off\n");
    return 1;
  }
  condition_item.durability = 0;
  if (!sf_item_condition_warning_visible(&condition_item, dagger, 8u) ||
      sf_item_condition_warning_blinks(&condition_item, dagger) ||
      !sf_njp_decoded_pattern(&assets.inventory_panel, 16u)) {
    fprintf(stderr, "A broken item did not keep the authored warning visible\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_x = SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT + 4;
  input.pointer_y = SF_GAMEPLAY_INVENTORY_BACKPACK_TOP + 4;
  (void) sf_gameplay_inventory_input_resolve(
    &ui, &world.player, false, &input);
  (void) sf_gameplay_inventory_input_resolve(
    &ui, &world.player, false, &input);
  if (ui.item_hover_updates != 2u) {
    fprintf(stderr, "The retail three-update item hover opened too early\n");
    return 1;
  }
  if (!sf_gameplay_inventory_input_resolve(
        &ui, &world.player, false, &input) ||
      ui.item_hover_updates != 3u) {
    fprintf(stderr, "The item information did not open on update three\n");
    return 1;
  }
  if (!sf_renderer_init(
        &renderer, sf_inventory_test_pixels,
        sizeof(sf_inventory_test_pixels), 640u, 480u)) return 1;
  condition_item.durability = 30;
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_item_condition_draw(
    &renderer, &assets, &condition_item, 100, 100, 0u, NULL);
  memcpy(
    sf_inventory_test_comparison, sf_inventory_test_pixels,
    sizeof(sf_inventory_test_pixels));
  condition_item.durability = 29;
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_item_condition_draw(
    &renderer, &assets, &condition_item, 100, 100, 0u, NULL);
  if (memcmp(
        sf_inventory_test_comparison, sf_inventory_test_pixels,
        sizeof(sf_inventory_test_pixels)) == 0) {
    fprintf(stderr, "The authored condition marker was not drawn\n");
    return 1;
  }
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_item_information_draw(
    &renderer, &assets, &world.player, &ui);
  changed = 0u;
  for (pixel = 0u; pixel < 640u * 480u; ++pixel)
    if (sf_inventory_test_pixels[pixel] != 0x1234u) ++changed;
  if (changed < 1000u) {
    fprintf(stderr, "The item information overlay was not rendered\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.status_pressed = true;
  if (!sf_gameplay_panels_input_resolve(
        &status, &ui, &world.player, false, &input) ||
      status.tab != SF_GAMEPLAY_CHARACTER_TAB_STATUS ||
      input.world_view_offset_x != 0) {
    fprintf(stderr, "Status did not open beside Inventory\n");
    return 1;
  }
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_status_draw(
    &renderer, &assets, &world.player, &status, NULL);
  changed = 0u;
  for (pixel = 0u; pixel < 640u * 480u; ++pixel)
    if (sf_inventory_test_pixels[pixel] != 0x1234u) ++changed;
  if (changed < 1000u) {
    fprintf(stderr, "The authored Status panel was not drawn\n");
    return 1;
  }
  world.player.inventory_transfer.held_item =
    world.player.inventory.items[0];
  world.player.inventory_transfer.holding_item = true;
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_primary_down = true;
  input.pointer_x = 100;
  input.pointer_y = 100;
  (void) sf_gameplay_panels_input_resolve(
    &status, &ui, &world.player, false, &input);
  if (input.inventory_action != SF_INVENTORY_ACTION_NONE ||
      !input.pointer_over_gameplay_ui) {
    fprintf(stderr, "A Status click leaked into an item drop\n");
    return 1;
  }
  world.player.inventory_transfer.holding_item = false;
  memset(&input, 0, sizeof(input));
  input.cancel_pressed = true;
  if (!sf_gameplay_panels_input_resolve(
        &status, &ui, &world.player, false, &input) ||
      status.tab != SF_GAMEPLAY_CHARACTER_TAB_CLOSED ||
      ui.open || input.cancel_pressed) {
    fprintf(stderr, "Escape did not close both live panels\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_x = 550;
  input.pointer_y = 425;
  if (!sf_gameplay_panels_input_resolve(
        &status, &ui, &world.player, false, &input) ||
      status.tab != SF_GAMEPLAY_CHARACTER_TAB_STATUS ||
      !input.pointer_over_gameplay_ui) {
    fprintf(stderr, "The authored STATUS button did not open Status\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.status_pressed = true;
  (void) sf_gameplay_panels_input_resolve(
    &status, &ui, &world.player, false, &input);
  if (status.tab != SF_GAMEPLAY_CHARACTER_TAB_CLOSED) return 1;
  ui.open = true;
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_x = 100;
  input.pointer_y = 200;
  input.special_items_pressed = true;
  if (!sf_gameplay_inventory_input_resolve(
        &ui, &world.player, false, &input) || !ui.special_open ||
      input.world_view_offset_x != 0) {
    fprintf(stderr, "Special Item did not open beside Inventory\n");
    return 1;
  }
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_special_items_draw(
    &renderer, &assets, &world.player, &ui, 0u, NULL);
  changed = 0u;
  for (pixel = 0u; pixel < 640u * 480u; ++pixel)
    if (sf_inventory_test_pixels[pixel] != 0x1234u) ++changed;
  if (changed < 1000u) {
    fprintf(stderr, "The authored Special Item panel was not drawn\n");
    return 1;
  }
  destination = world.player.destination;
  if (resolve_take(&ui, &world, &input, 0)) return 1;
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_primary_down = true;
  input.pointer_x = SF_GAMEPLAY_SPECIAL_LEFT + 16;
  input.pointer_y = SF_GAMEPLAY_SPECIAL_TOP + 48;
  (void) sf_gameplay_inventory_input_resolve(
    &ui, &world.player, false, &input);
  if (input.inventory_action != SF_INVENTORY_ACTION_PLACE_SPECIAL ||
      input.special_grid_x != 0 || input.special_grid_y != 0) return 1;
  sf_world_state_update(&world, &input);
  if (world.player.inventory_transfer.holding_item ||
      world.player.special_items.count != 1u ||
      world.player.inventory.count != 0u ||
      world.player.destination.x != destination.x ||
      world.player.destination.y != destination.y) {
    fprintf(stderr, "Backpack to Special Item transfer lost state\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_primary_down = true;
  input.pointer_x = SF_GAMEPLAY_SPECIAL_LEFT + 4;
  input.pointer_y = SF_GAMEPLAY_SPECIAL_TOP + 4;
  (void) sf_gameplay_inventory_input_resolve(
    &ui, &world.player, false, &input);
  if (input.inventory_action != SF_INVENTORY_ACTION_TAKE_SPECIAL) return 1;
  sf_world_state_update(&world, &input);
  if (!world.player.inventory_transfer.holding_item ||
      world.player.special_items.count != 0u) return 1;
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_primary_down = true;
  input.pointer_x = SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT + 16;
  input.pointer_y = SF_GAMEPLAY_INVENTORY_BACKPACK_TOP + 48;
  (void) sf_gameplay_inventory_input_resolve(
    &ui, &world.player, false, &input);
  if (input.inventory_action != SF_INVENTORY_ACTION_PLACE) return 1;
  sf_world_state_update(&world, &input);
  if (world.player.inventory_transfer.holding_item ||
      world.player.inventory.count != 1u) return 1;
  memset(&input, 0, sizeof(input));
  input.special_items_pressed = true;
  (void) sf_gameplay_inventory_input_resolve(
    &ui, &world.player, false, &input);
  if (ui.special_open) return 1;
  destination = world.player.destination;
  if (resolve_take(&ui, &world, &input, 0) ||
      world.player.inventory.count != 0u ||
      world.player.destination.x != destination.x ||
      world.player.destination.y != destination.y) {
    fprintf(stderr, "Taking a backpack item leaked into movement\n");
    return 1;
  }
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_inventory_draw_held(
    &renderer, &assets, &world.player, &ui, 0u);
  for (pixel = 0u; pixel < 640u * 480u; ++pixel)
    if (sf_inventory_test_pixels[pixel] != 0x1234u) ++changed;
  if (changed < 10u) {
    fprintf(stderr, "The full held icon was not drawn at the pointer\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_primary_down = true;
  input.pointer_x = SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT + 4 * 32 + 16;
  input.pointer_y = SF_GAMEPLAY_INVENTORY_BACKPACK_TOP + 48;
  (void) sf_gameplay_inventory_input_resolve(
    &ui, &world.player, false, &input);
  if (input.inventory_action != SF_INVENTORY_ACTION_PLACE ||
      input.inventory_grid_x != 4 || input.inventory_grid_y != 0) return 1;
  sf_world_state_update(&world, &input);
  if (world.player.inventory_transfer.holding_item ||
      sf_inventory_item_at(&world.player.inventory, 4u, 0u) != 0) {
    fprintf(stderr, "The held Dagger was not placed in its chosen cells\n");
    return 1;
  }
  if (resolve_take(&ui, &world, &input, 4)) return 1;
  destination = world.player.destination;
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_primary_down = true;
  input.pointer_x = 100;
  input.pointer_y = 200;
  if (!sf_gameplay_inventory_input_resolve(
        &ui, &world.player, false, &input) ||
      input.inventory_action != SF_INVENTORY_ACTION_DROP_WORLD ||
      !input.pointer_over_gameplay_ui) return 1;
  sf_world_state_update(&world, &input);
  if (world.player.inventory_transfer.holding_item ||
      world.ground_items.count != 1u ||
      world.player.destination.x != destination.x ||
      world.player.destination.y != destination.y ||
      !world.pointer.inventory_pointer_guard ||
      world.ground_items.sound_count != 1u ||
      world.ground_items.sound_samples[0] != 48u) {
    fprintf(stderr, "The world drop did not consume its pointer command\n");
    return 1;
  }
  dropped = &world.ground_items.items[0];
  dropped_id = dropped->id;
  offset.x = dropped->position.x - world.player.position.x;
  offset.y = dropped->position.y - world.player.position.y;
  if (dropped->definition_id != 100 || dropped->durability != 300 ||
      !dropped->identified ||
      (offset.x != -200 && offset.x != 0 && offset.x != 200) ||
      (offset.y != -200 && offset.y != 0 && offset.y != 200) ||
      (offset.x == 0 && offset.y == 0)) {
    fprintf(stderr, "The concrete item did not use the retail drop ring\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_down = true;
  input.pointer_x = 100;
  input.pointer_y = 200;
  input.world_view_offset_x = SF_GAMEPLAY_INVENTORY_VIEW_OFFSET;
  sf_world_state_update(&world, &input);
  if (world.player.destination.x != destination.x ||
      world.player.destination.y != destination.y ||
      !world.pointer.inventory_pointer_guard) {
    fprintf(stderr, "The drop click became movement before release\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  sf_world_state_update(&world, &input);
  if (world.pointer.inventory_pointer_guard) return 1;
  memset(&input, 0, sizeof(input));
  input.pointer_primary_pressed = true;
  input.world_pointer_resolved = true;
  input.pointed_actor_id = -1;
  input.pointed_ground_item_id = dropped_id;
  sf_world_state_update(&world, &input);
  if (world.ground_items.count != 0u || world.player.inventory.count != 1u ||
      world.player.inventory.items[0].definition_id != 100 ||
      world.player.inventory.items[0].durability != 300 ||
      !world.player.inventory.items[0].identified) {
    fprintf(stderr, "The concrete dropped item did not survive its round trip\n");
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
  if (test_inventory_transactions() || test_special_item_transactions())
    return 1;
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
    &arena, sf_inventory_test_memory.bytes,
    sizeof(sf_inventory_test_memory.bytes));
  if (test_live_inventory(root, &arena)) return 1;
#endif
  return 0;
}
