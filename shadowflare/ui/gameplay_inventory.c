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

#include "ui/gameplay_inventory.h"

#include "game/inventory.h"
#include "ui/gameplay_equipment_layout.h"

#include <stdio.h>
#include <string.h>

void sf_gameplay_inventory_init(SfGameplayInventoryUi *inventory) {
  if (!inventory) return;
  memset(inventory, 0, sizeof(*inventory));
  inventory->hovered_item_index = -1;
  inventory->hovered_equipment_slot = -1;
}

static bool sf_inventory_intersects_panel(const SfRect *clip) {
  if (!clip) return true;
  return clip->x < 640 && clip->x + clip->width > 320 &&
    clip->y < 412 && clip->y + clip->height > 0;
}

static void sf_inventory_draw_panel_pattern(
    SfRenderer *renderer, const SfNjpDecodedResource *resource,
    uint8_t source_pattern) {
  const SfNjpDecodedPattern *pattern =
    sf_njp_decoded_pattern(resource, source_pattern);
  uint8_t reference;
  if (!pattern || pattern->palette >= resource->palette_count) return;
  for (reference = 0u; reference < pattern->reference_count; ++reference) {
    const SfNjpDecodedReference *item =
      &resource->references[pattern->first_reference + reference];
    SfIndexedImage image;
    if (item->part >= resource->part_count) continue;
    image = resource->parts[item->part].image;
    image.palette = resource->palettes[pattern->palette];
    sf_renderer_draw_indexed(
      renderer, &image, item->x, item->y,
      1000u, 1000u, SF_BLEND_MASKED, NULL);
  }
}

static void sf_inventory_draw_text(
    SfRenderer *renderer, const SfIndexedImage *font,
    const char *text, int x, int y, uint16_t color) {
  sf_renderer_draw_text(renderer, font, text, x + 1, y + 1, 0u, 1000u);
  sf_renderer_draw_text(renderer, font, text, x, y, color, 1000u);
}

static void sf_inventory_draw_number(
    SfRenderer *renderer, const SfIndexedImage *font,
    int32_t value, int right, int y, uint16_t color) {
  char text[16];
  int length;
  if (value < 0) value = 0;
  length = snprintf(text, sizeof(text), "%d", (int) value);
  if (length <= 0 || (size_t) length >= sizeof(text)) return;
  sf_inventory_draw_text(
    renderer, font, text, right - length * 8, y, color);
}

static const SfItemGroundDefinition *sf_inventory_definition(
    const SfGameplayAssets *assets, const SfInventoryItem *item) {
  uint8_t index;
  for (index = 0u; index < assets->ground_items.definition_count; ++index) {
    const SfItemGroundDefinition *definition =
      &assets->ground_items.definitions[index];
    if (definition->category == item->category &&
        definition->definition_id == item->definition_id) return definition;
  }
  return NULL;
}

static void sf_inventory_draw_item_at(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfInventoryItem *item, int x, int y) {
  const SfItemGroundDefinition *definition =
    sf_inventory_definition(assets, item);
  const SfNjpSparseResource *resource;
  const SfNjpSparsePattern *pattern;
  SfIndexedImage image;
  if (!definition || definition->inventory_pattern_group < 0 ||
      definition->inventory_pattern < 0) return;
  resource = sf_inventory_item_artwork(
    &assets->inventory_items, definition->inventory_pattern_group);
  pattern = sf_njp_sparse_pattern(
    resource, definition->inventory_pattern);
  if (!pattern) return;
  image = pattern->image.image;
  if (definition->inventory_palette >= 0) {
    const uint16_t *palette = sf_njp_sparse_palette(
      resource, definition->inventory_palette);
    if (!palette) return;
    image.palette = palette;
  }
  sf_renderer_draw_indexed(
    renderer, &image,
    x + pattern->image.x, y + pattern->image.y,
    1000u, 1000u, SF_BLEND_MASKED, NULL);
}

static void sf_inventory_draw_item(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfInventoryItem *item) {
  sf_inventory_draw_item_at(
    renderer, assets, item,
    SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT +
      item->grid_x * SF_GAMEPLAY_INVENTORY_CELL_SIZE,
    SF_GAMEPLAY_INVENTORY_BACKPACK_TOP +
      item->grid_y * SF_GAMEPLAY_INVENTORY_CELL_SIZE);
}

void sf_gameplay_inventory_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfPlayerState *player, const SfGameplayInventoryUi *inventory,
    const SfRect *clip) {
  const uint16_t value_color = sf_rgb555(28u, 24u, 16u);
  const uint16_t normal_color = sf_rgb555(28u, 28u, 28u);
  const SfIndexedImage *font;
  uint8_t item;
  if (!renderer || !assets || !player || !inventory || !inventory->open ||
      assets->font.image_count == 0u ||
      !sf_inventory_intersects_panel(clip)) return;
  font = &assets->font.images[0].image;
  sf_renderer_fill_rect(
    renderer, (SfRect) {320, 0, 320, 412}, 0u);
  sf_inventory_draw_panel_pattern(renderer, &assets->inventory_panel, 2u);
  sf_inventory_draw_panel_pattern(renderer, &assets->inventory_panel, 3u);
  sf_inventory_draw_panel_pattern(
    renderer, &assets->inventory_panel, player->gender == 1u ? 0u : 1u);
  sf_inventory_draw_text(
    renderer, font, "Total Gold", 342, 30, value_color);
  sf_inventory_draw_number(
    renderer, font, sf_inventory_gold(&player->inventory),
    471, 28, value_color);
  sf_inventory_draw_number(renderer, font, 0, 446, 118, normal_color);
  sf_inventory_draw_text(renderer, font, "/", 445, 117, normal_color);
  sf_inventory_draw_number(renderer, font, 10, 471, 118, normal_color);
  sf_inventory_draw_number(
    renderer, font,
    sf_equipment_total_weight(
      &player->equipment, assets->ground_items.definitions,
      assets->ground_items.definition_count),
    471, 224, value_color);
  for (item = 0u; item < SF_EQUIPMENT_SLOT_COUNT; ++item) {
    const SfInventoryItem *equipped = sf_equipment_item(
      &player->equipment, (SfEquipmentSlot) item);
    int x;
    int y;
    if (!equipped) continue;
    sf_gameplay_equipment_item_origin(
      (SfEquipmentSlot) item, equipped, &x, &y);
    sf_inventory_draw_item_at(renderer, assets, equipped, x, y);
  }
  for (item = 0u; item < player->inventory.count; ++item)
    sf_inventory_draw_item(
      renderer, assets, &player->inventory.items[item]);
  sf_inventory_draw_panel_pattern(
    renderer, &assets->inventory_panel,
    inventory->close_hovered ? 117u : 116u);
}

void sf_gameplay_inventory_draw_held(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfPlayerState *player, const SfGameplayInventoryUi *inventory) {
  const SfInventoryItem *item;
  if (!renderer || !assets || !player || !inventory ||
      !player->inventory_transfer.holding_item) return;
  item = &player->inventory_transfer.held_item;
  sf_inventory_draw_item_at(
    renderer, assets, item,
    inventory->pointer_x -
      item->width * SF_GAMEPLAY_INVENTORY_CELL_SIZE / 2,
    inventory->pointer_y -
      item->height * SF_GAMEPLAY_INVENTORY_CELL_SIZE / 2);
}
