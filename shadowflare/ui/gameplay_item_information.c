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

#include "ui/gameplay_item_information.h"

#include "ui/gameplay_item_icon.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SF_ITEM_INFORMATION_TEXT_CAPACITY 768u

static bool sf_item_information_append(
    char *text, size_t capacity, size_t *length,
    const char *format, ...) {
  va_list arguments;
  int written;
  if (*length >= capacity) return false;
  va_start(arguments, format);
  written = vsnprintf(
    text + *length, capacity - *length, format, arguments);
  va_end(arguments);
  if (written < 0 || (size_t) written >= capacity - *length) return false;
  *length += (size_t) written;
  return true;
}

static int32_t sf_item_information_sale_price(
    const SfInventoryItem *item,
    const SfItemGroundDefinition *definition) {
  int64_t price = definition->base_price > 0 ? definition->base_price : 0;
  if (definition->category <= 1u && definition->maximum_durability > 0) {
    int32_t durability = item->durability;
    if (durability < 0) durability = 0;
    if (durability > definition->maximum_durability)
      durability = definition->maximum_durability;
    price = price * durability / definition->maximum_durability;
  }
  price /= 4;
  if (price < 1) price = 1;
  if (price > INT32_MAX) price = INT32_MAX;
  return (int32_t) price;
}

static bool sf_item_information_value(
    char *text, size_t capacity, size_t *length,
    const char *label, int32_t value) {
  return sf_item_information_append(
    text, capacity, length, "%-26s:%9d\n", label, (int) value);
}

bool sf_gameplay_item_information_text(
    char *text, size_t capacity, const SfInventoryItem *item,
    const SfItemGroundDefinition *definition) {
  static const char *parameter_labels[10] = {
    "Attack", "Hit Rate", "Defense", "Evasion Rate", "Magical Attack",
    "Magical Hit Rate", "Magical Defense", "Magical Evasion Rate",
    "Speed of Attack", "Walking Speed"
  };
  size_t length = 0u;
  uint8_t index;
  if (!text || capacity == 0u || !item || !definition) return false;
  text[0] = '\0';
  if (!sf_item_information_append(
        text, capacity, &length, "[%s]\n\n",
        item->identified ? definition->name : definition->description))
    return false;
  if (!item->identified) return true;
  if (definition->category <= 1u) {
    for (index = 0u; index < 10u; ++index)
      if (definition->parameter_bonuses[index] != 0 &&
          !sf_item_information_value(
            text, capacity, &length, parameter_labels[index],
            definition->parameter_bonuses[index])) return false;
    if (!sf_item_information_value(
          text, capacity, &length, "Durability", item->durability) ||
        !sf_item_information_value(
          text, capacity, &length, "Weight", definition->weight) ||
        !sf_item_information_value(
          text, capacity, &length, "Required Level",
          definition->required_level) ||
        !sf_item_information_value(
          text, capacity, &length, "Sale Price",
          sf_item_information_sale_price(item, definition)) ||
        !sf_item_information_append(
          text, capacity, &length,
          "\nFire   :%3d Water  :%3d Earth  :%3d Thunder:%3d\n"
          "Holy   :%3d Dark   :%3d Gel    :%3d Metal  :%3d\n",
          (int) definition->element_strengths[0],
          (int) definition->element_strengths[1],
          (int) definition->element_strengths[2],
          (int) definition->element_strengths[3],
          (int) definition->element_strengths[4],
          (int) definition->element_strengths[5],
          (int) definition->element_strengths[6],
          (int) definition->element_strengths[7])) return false;
  } else if (definition->category == 2u) {
    if (!sf_item_information_value(
          text, capacity, &length, "Weight", definition->weight) ||
        !sf_item_information_value(
          text, capacity, &length, "Required Level",
          definition->required_level) ||
        !sf_item_information_value(
          text, capacity, &length, "Sale Price",
          sf_item_information_sale_price(item, definition))) return false;
  } else if (definition->category == 3u) {
    if (!sf_item_information_value(
          text, capacity, &length, "Sale Price",
          sf_item_information_sale_price(item, definition))) return false;
  } else if (definition->category == 4u) {
    if (!sf_item_information_value(
          text, capacity, &length,
          definition->definition_id == 0 ? "Price" : "Sale Price",
          definition->definition_id == 0
            ? item->quantity
            : sf_item_information_sale_price(item, definition))) return false;
  }
  return true;
}

static const SfInventoryItem *sf_item_information_hovered(
    const SfPlayerState *player,
    const SfGameplayInventoryUi *inventory) {
  if (!inventory->open || inventory->item_hover_updates < 3u ||
      player->inventory_transfer.holding_item) return NULL;
  if (inventory->hovered_equipment_slot >= 0)
    return sf_equipment_item(
      &player->equipment,
      (SfEquipmentSlot) inventory->hovered_equipment_slot);
  if (inventory->hovered_item_index < 0 ||
      inventory->hovered_item_index >= player->inventory.count) return NULL;
  return &player->inventory.items[
    (uint8_t) inventory->hovered_item_index];
}

static void sf_item_information_dimensions(
    const char *text, int *columns, int *lines) {
  int column = 0;
  *columns = 0;
  *lines = 0;
  while (*text) {
    if (*text++ == '\n') {
      if (column > *columns) *columns = column;
      column = 0;
      ++*lines;
    } else {
      ++column;
    }
  }
  if (column > *columns) *columns = column;
  if (column > 0 || *lines == 0) ++*lines;
}

static uint16_t sf_item_information_color(int32_t variant) {
  if (variant == 1) return sf_rgb555(24u, 16u, 16u);
  if (variant == 2) return sf_rgb555(16u, 24u, 28u);
  if (variant == 3) return sf_rgb555(12u, 16u, 28u);
  return sf_rgb555(28u, 28u, 28u);
}

void sf_gameplay_item_information_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfPlayerState *player, const SfGameplayInventoryUi *inventory) {
  char text[SF_ITEM_INFORMATION_TEXT_CAPACITY];
  const SfInventoryItem *item;
  const SfItemGroundDefinition *definition;
  const SfIndexedImage *font;
  int columns;
  int lines;
  int width;
  int height;
  int x;
  int y;
  if (!renderer || !assets || !player || !inventory ||
      assets->font.image_count == 0u) return;
  item = sf_item_information_hovered(player, inventory);
  definition = sf_gameplay_item_definition(assets, item);
  if (!item || !definition || !sf_gameplay_item_information_text(
        text, sizeof(text), item, definition)) return;
  font = &assets->font.images[0].image;
  if (font->width < 16u || font->height < 16u) return;
  sf_item_information_dimensions(text, &columns, &lines);
  width = columns * (font->width / 16u) + 8;
  height = lines * (font->height / 16u) + 8;
  x = inventory->pointer_x - width / 2;
  y = inventory->pointer_y + 8;
  if (x < 1) x = 1;
  if (x > 640 - width) x = 640 - width;
  if (y < 1) y = 1;
  if (y > 480 - height) y = 480 - height;
  sf_renderer_fill_rect_blended(
    renderer, (SfRect) {x, y, width, height}, 0u, 600u);
  sf_renderer_fill_rect_blended(
    renderer, (SfRect) {x - 1, y - 1, width + 1, 1},
    0x7fffu, 500u);
  sf_renderer_fill_rect_blended(
    renderer, (SfRect) {x - 1, y - 1, 1, height + 1},
    0x7fffu, 500u);
  sf_renderer_fill_rect_blended(
    renderer, (SfRect) {x + width - 1, y, 1, height},
    0x7fffu, 500u);
  sf_renderer_fill_rect_blended(
    renderer, (SfRect) {x, y + height - 1, width, 1},
    0x7fffu, 500u);
  sf_renderer_draw_text(renderer, font, text, x + 5, y + 5, 0u, 1000u);
  sf_renderer_draw_text(
    renderer, font, text, x + 4, y + 4,
    sf_item_information_color(definition->variant), 1000u);
}
