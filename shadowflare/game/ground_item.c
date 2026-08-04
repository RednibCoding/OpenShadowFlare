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

#include "game/ground_item.h"

#include "core/retail_random.h"

#include <limits.h>
#include <string.h>

#define SF_GROUND_ITEM_GOLD_CATEGORY 4
#define SF_GROUND_ITEM_GOLD_DEFINITION 0
#define SF_GROUND_ITEM_GOLD_STACK 10000

static const SfWorldPoint sf_ground_item_gold_offsets[20] = {
  {200, 0}, {190, -61}, {161, -117}, {117, -161}, {61, -190},
  {0, -200}, {-61, -190}, {-117, -161}, {-161, -117}, {-190, -61},
  {-200, 0}, {-190, 61}, {-161, 117}, {-117, 161}, {-61, 190},
  {0, 200}, {61, 190}, {117, 161}, {161, 117}, {190, 61}
};

void sf_ground_items_init(SfGroundItemSet *items) {
  if (!items) return;
  memset(items, 0, sizeof(*items));
  items->random_state = 1u;
}

void sf_ground_items_bind_definitions(
    SfGroundItemSet *items, const SfItemGroundDefinition *definitions,
    uint8_t definition_count) {
  if (!items) return;
  items->definitions = definitions;
  items->definition_count = definition_count;
}

const SfItemGroundDefinition *sf_ground_items_definition(
    const SfGroundItemSet *items, uint8_t category,
    int32_t definition_id) {
  uint8_t index;
  if (!items) return NULL;
  for (index = 0u; index < items->definition_count; ++index) {
    const SfItemGroundDefinition *definition = &items->definitions[index];
    if (definition->category == category &&
        definition->definition_id == definition_id) return definition;
  }
  return NULL;
}

static bool sf_ground_item_add(
    SfGroundItemSet *items, const SfItemGroundDefinition *definition,
    SfWorldPoint position, int32_t quantity) {
  SfGroundItem *item;
  if (items->count >= SF_GROUND_ITEM_LIMIT || quantity <= 0) return false;
  item = &items->items[items->count++];
  memset(item, 0, sizeof(*item));
  item->position = position;
  item->judgement = (SfObjectBounds) {-20, -20, 19, 19};
  item->category = definition->category;
  item->definition_id = definition->definition_id;
  item->resource_id = definition->resource_id;
  item->animation_chart = definition->animation_chart;
  item->quantity = quantity;
  item->vertical_velocity = 1600;
  item->vertical_gravity = 280;
  item->red_strength = definition->red_strength;
  item->green_strength = definition->green_strength;
  item->blue_strength = definition->blue_strength;
  item->id = items->next_id++;
  item->visible = true;
  ++items->presentation_revision;
  return true;
}

bool sf_ground_items_create(
    SfGroundItemSet *items, int32_t category, int32_t definition_id,
    SfWorldPoint position, int32_t minimum_quantity,
    int32_t maximum_quantity) {
  const SfItemGroundDefinition *definition;
  if (category < 0 || category > UINT8_MAX) return false;
  definition = sf_ground_items_definition(
    items, (uint8_t) category, definition_id);
  if (!definition) return false;
  if (category != SF_GROUND_ITEM_GOLD_CATEGORY ||
      definition_id != SF_GROUND_ITEM_GOLD_DEFINITION) {
    return sf_ground_item_add(items, definition, position, 1);
  }
  if (minimum_quantity < 0 || maximum_quantity < minimum_quantity ||
      (uint32_t) maximum_quantity - (uint32_t) minimum_quantity >=
        (uint32_t) INT32_MAX) return false;
  {
    const uint32_t range =
      (uint32_t) maximum_quantity - (uint32_t) minimum_quantity + 1u;
    int32_t remaining = minimum_quantity + (int32_t) (
      sf_retail_random_next(&items->random_state) % range);
    const uint32_t stack_count = remaining > 0
      ? ((uint32_t) remaining + SF_GROUND_ITEM_GOLD_STACK - 1u) /
        SF_GROUND_ITEM_GOLD_STACK : 0u;
    uint8_t offset = 0u;
    if (stack_count > SF_GROUND_ITEM_LIMIT - items->count) return false;
    while (remaining > 0) {
      const int32_t quantity = remaining > SF_GROUND_ITEM_GOLD_STACK
        ? SF_GROUND_ITEM_GOLD_STACK : remaining;
      const SfWorldPoint radial = sf_ground_item_gold_offsets[
        offset % (sizeof(sf_ground_item_gold_offsets) /
          sizeof(sf_ground_item_gold_offsets[0]))];
      if (!sf_ground_item_add(
            items, definition,
            (SfWorldPoint) {position.x + radial.x, position.y + radial.y},
            quantity)) return false;
      remaining -= quantity;
      ++offset;
    }
  }
  return true;
}

SfGroundItem *sf_ground_items_find(SfGroundItemSet *items, int32_t id) {
  uint8_t index;
  if (!items) return NULL;
  for (index = 0u; index < items->count; ++index)
    if (items->items[index].id == id) return &items->items[index];
  return NULL;
}

void sf_ground_item_restart_drop(SfGroundItem *item) {
  if (!item) return;
  item->height = 0;
  item->vertical_velocity = 1600;
  item->vertical_gravity = 280;
  item->bounce_state = 0u;
}

bool sf_ground_items_remove(SfGroundItemSet *items, int32_t id) {
  uint8_t index;
  if (!items) return false;
  for (index = 0u; index < items->count; ++index) {
    if (items->items[index].id != id) continue;
    if (index + 1u < items->count)
      memmove(
        &items->items[index], &items->items[index + 1u],
        (size_t) (items->count - index - 1u) * sizeof(items->items[0]));
    --items->count;
    ++items->presentation_revision;
    return true;
  }
  return false;
}

void sf_ground_items_update(SfGroundItemSet *items) {
  uint8_t index;
  if (!items) return;
  items->sound_count = 0u;
  for (index = 0u; index < items->count; ++index) {
    SfGroundItem *item = &items->items[index];
    if (!item->visible || item->bounce_state >= 2u) continue;
    item->height += item->vertical_velocity / 10;
    item->vertical_velocity -= item->vertical_gravity;
    ++items->presentation_revision;
    if (item->height > 0) continue;
    item->height = 0;
    if (item->bounce_state == 0u) {
      item->bounce_state = 1u;
      item->vertical_velocity = 700;
      if (item->category == SF_GROUND_ITEM_GOLD_CATEGORY &&
          item->definition_id == SF_GROUND_ITEM_GOLD_DEFINITION)
        sf_ground_items_emit_sound(items, 85u);
      else if (item->category == 2u)
        sf_ground_items_emit_sound(items, 93u);
      else
        sf_ground_items_emit_sound(items, 15u);
    } else {
      item->bounce_state = 2u;
    }
  }
}

void sf_ground_items_emit_sound(SfGroundItemSet *items, uint16_t sample) {
  if (!items || items->sound_count >= 8u) return;
  items->sound_samples[items->sound_count++] = sample;
}
