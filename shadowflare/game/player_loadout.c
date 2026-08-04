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

#include "game/player.h"

void sf_player_refresh_visible_items(SfPlayerState *player) {
  uint8_t slot;
  uint8_t count = 0u;
  if (!player) return;
  for (slot = 0u; slot < SF_EQUIPMENT_SLOT_COUNT &&
       count < SF_PLAYER_VISIBLE_ITEM_LIMIT; ++slot) {
    const SfInventoryItem *item = sf_equipment_item(
      &player->equipment, (SfEquipmentSlot) slot);
    uint8_t existing;
    if (!item || item->category > 1u) continue;
    for (existing = 0u; existing < count; ++existing)
      if (player->visible_items[existing].category == item->category &&
          player->visible_items[existing].definition_id ==
            item->definition_id) break;
    if (existing < count) continue;
    player->visible_items[count].category = item->category;
    player->visible_items[count].definition_id = item->definition_id;
    ++count;
  }
  player->visible_item_count = count;
}

static const SfItemGroundDefinition *sf_player_item_definition(
    const SfItemGroundDefinition *definitions, uint8_t definition_count,
    uint8_t category, int32_t definition_id) {
  uint8_t index;
  for (index = 0u; index < definition_count; ++index)
    if (definitions[index].category == category &&
        definitions[index].definition_id == definition_id)
      return &definitions[index];
  return NULL;
}

static SfInventoryItem sf_player_new_item(
    const SfItemGroundDefinition *definition) {
  return (SfInventoryItem) {
    definition->definition_id, 1, definition->maximum_durability,
    definition->category, 0u, 0u,
    (uint8_t) definition->inventory_width,
    (uint8_t) definition->inventory_height, true};
}

bool sf_player_initialize_loadout(
    SfPlayerState *player, const SfItemGroundDefinition *definitions,
    uint8_t definition_count) {
  const SfItemGroundDefinition *leather;
  const SfItemGroundDefinition *tablet;
  const SfItemGroundDefinition *capsule;
  SfInventoryState inventory;
  SfEquipmentState equipment;
  SfBeltState belt;
  uint8_t row;
  if (!player || !definitions) return false;
  if (player->loadout_initialized) return true;
  leather = sf_player_item_definition(definitions, definition_count, 1u, 0);
  tablet = sf_player_item_definition(definitions, definition_count, 3u, 0);
  capsule = sf_player_item_definition(
    definitions, definition_count, 3u, 10000000);
  if (!leather || !tablet || !capsule) return false;
  sf_inventory_init(&inventory);
  sf_equipment_init(&equipment);
  sf_belt_init(&belt);
  if (!sf_equipment_place(
        &equipment, SF_EQUIPMENT_BODY, sf_player_new_item(leather),
        leather, player->level).accepted) return false;
  for (row = 0u; row < 4u; ++row) {
    if (!sf_inventory_place(
          &inventory, sf_player_new_item(tablet), 0, row).accepted ||
        !sf_inventory_place(
          &inventory, sf_player_new_item(capsule), 1, row).accepted ||
        !sf_belt_place(
          &belt, sf_player_new_item(tablet), row, 0, tablet).accepted ||
        !sf_belt_place(
          &belt, sf_player_new_item(capsule), row, 1, capsule).accepted)
      return false;
  }
  player->inventory = inventory;
  player->equipment = equipment;
  player->belt = belt;
  player->mine_count = 5;
  player->loadout_initialized = true;
  sf_player_refresh_visible_items(player);
  return true;
}

static bool sf_player_add_required_item(
    SfItemReference *items, uint8_t *count, uint8_t capacity,
    uint8_t category, int32_t definition_id) {
  uint8_t index;
  for (index = 0u; index < *count; ++index)
    if (items[index].category == category &&
        items[index].definition_id == definition_id) return true;
  if (*count >= capacity) return false;
  items[*count].category = category;
  items[*count].definition_id = definition_id;
  ++*count;
  return true;
}

bool sf_player_required_item_definitions(
    const SfPlayerState *player, SfItemReference *items,
    uint8_t capacity, uint8_t *item_count) {
  uint8_t index;
  if (!player || !items || !item_count || capacity == 0u) return false;
  *item_count = 0u;
  if (!player->loadout_initialized &&
      (!sf_player_add_required_item(items, item_count, capacity, 1u, 0) ||
       !sf_player_add_required_item(items, item_count, capacity, 3u, 0) ||
       !sf_player_add_required_item(
         items, item_count, capacity, 3u, 10000000))) return false;
  for (index = 0u; index < SF_EQUIPMENT_SLOT_COUNT; ++index) {
    const SfInventoryItem *item = sf_equipment_item(
      &player->equipment, (SfEquipmentSlot) index);
    if (item && !sf_player_add_required_item(
          items, item_count, capacity,
          item->category, item->definition_id)) return false;
  }
  for (index = 0u; index < player->inventory.count; ++index)
    if (!sf_player_add_required_item(
          items, item_count, capacity,
          player->inventory.items[index].category,
          player->inventory.items[index].definition_id)) return false;
  for (index = 0u; index < player->belt.count; ++index)
    if (!sf_player_add_required_item(
          items, item_count, capacity,
          player->belt.items[index].category,
          player->belt.items[index].definition_id)) return false;
  if (player->inventory_transfer.holding_item &&
      !sf_player_add_required_item(
        items, item_count, capacity,
        player->inventory_transfer.held_item.category,
        player->inventory_transfer.held_item.definition_id)) return false;
  return true;
}
