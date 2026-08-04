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

#include "game/player_save.h"

#include <string.h>

static const SfItemGroundDefinition *sf_player_saved_definition(
    const SfItemGroundDefinition *definitions, uint8_t count,
    const SfSavedItem *saved) {
  uint8_t index;
  for (index = 0u; index < count; ++index)
    if (definitions[index].category == saved->category &&
        definitions[index].definition_id == saved->definition_id)
      return &definitions[index];
  return NULL;
}

static bool sf_player_saved_item(
    const SfSavedItem *saved, const SfItemGroundDefinition *definition,
    SfInventoryItem *item) {
  if (!saved || !definition || !item || !saved->present ||
      saved->quantity <= 0 || definition->inventory_width <= 0 ||
      definition->inventory_width > (int32_t) SF_INVENTORY_WIDTH ||
      definition->inventory_height <= 0 ||
      definition->inventory_height > (int32_t) SF_INVENTORY_HEIGHT)
    return false;
  *item = (SfInventoryItem) {
    saved->definition_id, saved->quantity, saved->durability,
    saved->category, 0u, 0u,
    (uint8_t) definition->inventory_width,
    (uint8_t) definition->inventory_height, saved->identified};
  return true;
}

static bool sf_player_restore_equipment(
    SfEquipmentState *equipment, const SfSavedPlayer *saved,
    const SfItemGroundDefinition *definitions, uint8_t definition_count) {
  static const SfEquipmentSlot slots[SF_SAVED_EQUIPMENT_COUNT] = {
    SF_EQUIPMENT_MAIN_HAND, SF_EQUIPMENT_HELMET, SF_EQUIPMENT_BODY,
    SF_EQUIPMENT_OFF_HAND, SF_EQUIPMENT_BOOTS,
    SF_EQUIPMENT_ACCESSORY_1, SF_EQUIPMENT_ACCESSORY_2,
    SF_EQUIPMENT_ACCESSORY_3, SF_EQUIPMENT_ACCESSORY_4,
    SF_EQUIPMENT_ALTERNATE_MAIN_HAND, SF_EQUIPMENT_ALTERNATE_OFF_HAND
  };
  uint8_t index;
  for (index = 0u; index < SF_SAVED_EQUIPMENT_COUNT; ++index) {
    const SfSavedItem *source = &saved->equipment[index];
    const SfItemGroundDefinition *definition;
    SfInventoryItem item;
    SfEquipmentPlacement placement;
    if (!source->present) continue;
    definition = sf_player_saved_definition(
      definitions, definition_count, source);
    if (!definition || !sf_player_saved_item(source, definition, &item))
      return false;
    placement = sf_equipment_place(
      equipment, slots[index], item, definition, saved->level);
    if (!placement.accepted || placement.holding_item) return false;
  }
  return true;
}

static bool sf_player_restore_backpack(
    SfInventoryState *inventory, const SfSavedPlayer *saved,
    const SfItemGroundDefinition *definitions, uint8_t definition_count) {
  uint8_t index;
  for (index = 0u; index < saved->backpack_count; ++index) {
    const SfSavedItem *source = &saved->backpack[index];
    const SfItemGroundDefinition *definition = sf_player_saved_definition(
      definitions, definition_count, source);
    SfInventoryItem item;
    SfInventoryPlacement placement;
    if (!definition || !sf_player_saved_item(source, definition, &item))
      return false;
    placement = sf_inventory_place(
      inventory, item, source->grid_x, source->grid_y);
    if (!placement.accepted || placement.holding_item) return false;
  }
  return true;
}

static bool sf_player_restore_belt(
    SfBeltState *belt, const SfSavedPlayer *saved,
    const SfItemGroundDefinition *definitions, uint8_t definition_count) {
  uint8_t index;
  for (index = 0u; index < saved->belt_count; ++index) {
    const SfSavedItem *source = &saved->belt[index];
    const SfItemGroundDefinition *definition = sf_player_saved_definition(
      definitions, definition_count, source);
    SfInventoryItem item;
    SfInventoryPlacement placement;
    if (!definition || !sf_player_saved_item(source, definition, &item))
      return false;
    placement = sf_belt_place(
      belt, item, source->grid_x, source->grid_y, definition);
    if (!placement.accepted || placement.holding_item) return false;
  }
  return true;
}

bool sf_player_restore_save(
    SfPlayerState *player, const SfSavedPlayer *saved,
    const SfItemGroundDefinition *definitions, uint8_t definition_count,
    int32_t experience_threshold) {
  SfInventoryState inventory;
  SfEquipmentState equipment;
  SfBeltState belt;
  int32_t speed_tier;
  if (!player || !saved || !definitions || experience_threshold < 0 ||
      saved->level <= 0 || saved->parameters.values[2] <= 0 ||
      saved->parameters.values[3] <= 0) return false;
  sf_inventory_init(&inventory);
  sf_equipment_init(&equipment);
  sf_belt_init(&belt);
  if (!sf_player_restore_equipment(
        &equipment, saved, definitions, definition_count) ||
      !sf_player_restore_backpack(
        &inventory, saved, definitions, definition_count) ||
      !sf_player_restore_belt(
        &belt, saved, definitions, definition_count)) return false;
  player->inventory = inventory;
  player->equipment = equipment;
  player->belt = belt;
  player->inventory_transfer.holding_item = false;
  player->initial_parameters = saved->parameters;
  player->initial_parameters.experience_threshold = experience_threshold;
  player->gender = saved->gender == 1 ? 1u : 0u;
  player->level = saved->level;
  player->experience = saved->experience;
  player->current_life = saved->current_life;
  player->current_mana = saved->current_mana;
  if (player->current_life <= 0) {
    player->current_life = saved->parameters.values[2];
    player->current_mana = saved->parameters.values[3];
  }
  speed_tier = (saved->parameters.values[1] + 32) / 32;
  if (speed_tier < 0) speed_tier = 0;
  if (speed_tier > 9) speed_tier = 9;
  player->walking_speed_tier = (uint8_t) speed_tier;
  player->parameters_initialized = true;
  player->loadout_initialized = true;
  sf_player_set_identity(player, saved->name, saved->job);
  sf_player_refresh_visible_items(player);
  return true;
}
