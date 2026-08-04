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
#include "assets/retail_paths.h"
#include "core/memory_budget.h"
#include "game/player.h"
#include "game/world_magic.h"
#include "render/renderer.h"
#include "ui/gameplay_character_panel.h"
#include "ui/gameplay_inventory.h"
#include "ui/gameplay_magic.h"
#include "ui/gameplay_magic_layout.h"
#include "ui/gameplay_panels_input.h"

#include <stdio.h>
#include <string.h>

typedef union SfMagicTestMemory {
  long double alignment;
  void *pointer;
  uint8_t bytes[SF_MAIN_ARENA_BYTES];
} SfMagicTestMemory;

static SfMagicTestMemory sf_magic_test_memory;
static uint16_t sf_magic_test_pixels[640u * 480u];

static int test_magic_owner(void) {
  SfPlayerMagicState magic;
  sf_player_magic_init(&magic);
  if (magic.levels[0] != 1 || magic.levels[21] != 1 ||
      magic.bar_slots[0] != -1 || magic.bar_slots[7] != -1 ||
      magic.selected_spell != -1 || magic.targeting) return 1;
  magic.availability[0] = 3;
  magic.availability[1] = 3;
  if (!sf_player_magic_assign(&magic, 0, 0) ||
      !sf_player_magic_assign(&magic, 1, 0) ||
      magic.bar_slots[0] != -1 || magic.bar_slots[1] != 0 ||
      !sf_player_magic_select(&magic, 0) || magic.targeting ||
      !sf_player_magic_set_targeting(&magic, true) ||
      magic.selected_spell != -1 || !magic.targeting) {
    fprintf(stderr, "The persistent Magic owner differs from retail\n");
    return 1;
  }
  return 0;
}

static int test_magic_input(void) {
  SfGameplayCharacterPanelUi panel;
  SfGameplayInventoryUi inventory;
  SfGameplayMagicRegion slots[SF_PLAYER_MAGIC_BAR_SLOT_COUNT];
  SfGameplayMagicRegion target;
  SfPlayerState player;
  SfGameInput input;
  sf_player_init(&player, 1u);
  player.magic.availability[0] = 3;
  player.magic.availability[1] = 3;
  sf_gameplay_character_panel_init(&panel);
  sf_gameplay_inventory_init(&inventory);
  memset(&input, 0, sizeof(input));
  input.magic_pressed = true;
  if (!sf_gameplay_panels_input_resolve(
        &panel, &inventory, &player, false, &input) ||
      panel.tab != SF_GAMEPLAY_CHARACTER_TAB_MAGIC ||
      input.interface_sound != 58u ||
      input.world_view_offset_x != -SF_GAMEPLAY_INVENTORY_VIEW_OFFSET)
    return 1;
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_primary_down = true;
  input.pointer_x = 280;
  input.pointer_y = 340;
  if (!sf_gameplay_panels_input_resolve(
        &panel, &inventory, &player, false, &input) ||
      panel.magic_page != 1u || !input.pointer_over_gameplay_ui) return 1;
  panel.magic_page = 0u;
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_primary_down = true;
  input.pointer_x = 30;
  input.pointer_y = 60;
  if (!sf_gameplay_panels_input_resolve(
        &panel, &inventory, &player, false, &input) ||
      panel.held_spell != 0 || input.interface_sound != 57u) return 1;
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_x = 29 + 2 * 32 + 4;
  input.pointer_y = 360;
  if (!sf_gameplay_panels_input_resolve(
        &panel, &inventory, &player, false, &input) ||
      panel.held_spell != -1 || input.magic_action != SF_MAGIC_ACTION_ASSIGN ||
      input.magic_spell != 0 || input.magic_bar_slot != 2) return 1;
  if (!sf_player_magic_assign(
        &player.magic, input.magic_bar_slot, input.magic_spell)) return 1;
  panel.tab = SF_GAMEPLAY_CHARACTER_TAB_CLOSED;
  sf_gameplay_magic_bar_layout(
    &player.magic, false, false, slots, &target);
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_x = slots[2].x + 1;
  input.pointer_y = slots[2].y + 1;
  if (!sf_gameplay_panels_input_resolve(
        &panel, &inventory, &player, false, &input) ||
      input.magic_action != SF_MAGIC_ACTION_SELECT || input.magic_spell != 0)
    return 1;
  sf_world_magic_update(&player, &input);
  if (player.magic.selected_spell != 0 || player.magic.targeting) return 1;
  sf_gameplay_magic_bar_layout(
    &player.magic, false, false, slots, &target);
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_x = target.x + 1;
  input.pointer_y = target.y + 1;
  if (!sf_gameplay_panels_input_resolve(
        &panel, &inventory, &player, false, &input) ||
      input.magic_action != SF_MAGIC_ACTION_TOGGLE_TARGETING) return 1;
  sf_world_magic_update(&player, &input);
  if (!player.magic.targeting || player.magic.selected_spell != -1) return 1;
  return 0;
}

static int test_magic_rendering(void) {
#if defined(OPENSHADOWFLARE_SOURCE_DIR)
  SfGameplayCharacterPanelUi panel;
  SfGameplayAssets assets;
  SfItemReference retained[SF_GROUND_ITEM_DEFINITION_LIMIT];
  SfPlayerState player;
  SfRenderer renderer;
  SfArena arena;
  uint8_t retained_count;
  char root[1024];
  char probe_path[1024];
  FILE *probe;
  size_t pixel;
  size_t changed = 0u;
  (void) snprintf(
    root, sizeof(root), "%s/tmp/ShadowFlare", OPENSHADOWFLARE_SOURCE_DIR);
  if (!sf_retail_path_join(
        probe_path, sizeof(probe_path), root,
        sf_retail_game_paths.magic_icons)) return 1;
  probe = fopen(probe_path, "rb");
  if (!probe) return 0;
  fclose(probe);
  sf_player_init(&player, 1u);
  if (!sf_player_required_item_definitions(
        &player, retained, SF_GROUND_ITEM_DEFINITION_LIMIT,
        &retained_count)) return 1;
  sf_arena_init(
    &arena, sf_magic_test_memory.bytes, sizeof(sf_magic_test_memory.bytes));
  if (!sf_gameplay_assets_load(
        &assets, root, 0, 0, player.gender, player.level,
        player.companions.type,
        sf_player_companion_level(&player.companions),
        player.appearance_parts, player.appearance_part_count,
        player.visible_items, player.visible_item_count,
        retained, retained_count, &arena)) return 1;
  player.magic.availability[0] = 3;
  player.magic.availability[1] = 1;
  player.magic.levels[0] = 2;
  player.magic.experience[0] = 1;
  player.magic.bar_slots[0] = 0;
  player.magic.targeting = true;
  sf_gameplay_character_panel_init(&panel);
  panel.tab = SF_GAMEPLAY_CHARACTER_TAB_MAGIC;
  if (!sf_renderer_init(
        &renderer, sf_magic_test_pixels, sizeof(sf_magic_test_pixels),
        640u, 480u)) return 1;
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_magic_draw(&renderer, &assets, &player, &panel, NULL);
  sf_gameplay_magic_bar_draw(
    &renderer, &assets, &player, true, false, NULL);
  for (pixel = 0u; pixel < 640u * 480u; ++pixel)
    if (sf_magic_test_pixels[pixel] != 0x1234u) ++changed;
  if (changed < 10000u) {
    fprintf(stderr, "The authored Magic interface was not rendered\n");
    return 1;
  }
#endif
  return 0;
}

int main(void) {
  return test_magic_owner() || test_magic_input() || test_magic_rendering();
}
