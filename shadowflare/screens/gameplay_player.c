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

#include "screens/gameplay_player.h"

#include "core/coordinates.h"
#include "core/memory_budget.h"

static const SfCafSelectedAnimation *sf_gameplay_player_animation(
    const SfPlayerAssets *assets, const SfPlayerState *player) {
  return sf_player_assets_animation(
    assets, (uint8_t) player->motion, player->direction);
}

static bool sf_gameplay_player_part_style(
    const SfWorldState *world, uint8_t source_part,
    const SfItemGroundDefinition **definition) {
  uint8_t index;
  *definition = sf_equipment_part_definition(
    &world->player.equipment, world->ground_items.definitions,
    world->ground_items.definition_count, source_part);
  if (*definition) return true;
  for (index = 0u; index < world->player.appearance_part_count; ++index)
    if (world->player.appearance_parts[index] == source_part) return true;
  return false;
}

static uint16_t sf_gameplay_player_strength(int32_t strength) {
  if (strength <= 0) return 0u;
  return strength > UINT16_MAX ? UINT16_MAX : (uint16_t) strength;
}

SfRect sf_gameplay_player_bounds(
    const SfPlayerAssets *assets, const SfWorldState *world,
    const SfWorldRenderView *view) {
  const SfCafSelectedAnimation *animation;
  SfScreenPoint anchor;
  SfRect result = {0, 0, 0, 0};
  int left = SF_FRAME_WIDTH;
  int top = SF_FRAME_HEIGHT;
  int right = 0;
  int bottom = 0;
  uint8_t part;
  if (!assets || !world || !view) return result;
  animation = sf_gameplay_player_animation(assets, &world->player);
  if (!animation) return result;
  anchor = sf_world_to_screen(view->player_position);
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  for (part = 0u; part < animation->part_count; ++part) {
    const SfItemGroundDefinition *definition;
    uint8_t frame;
    if (!sf_gameplay_player_part_style(
          world, animation->parts[part].source_index, &definition)) continue;
    for (frame = 0u; frame < animation->frame_count; ++frame) {
      const SfCafCell *cell = &animation->parts[part].cells[frame];
      const SfNjpSparseResource *resource = (cell->status & 8) != 0
        ? &assets->shadows : &assets->artwork;
      const SfNjpSparsePattern *pattern =
        sf_njp_sparse_pattern(resource, cell->pattern);
      int x;
      int y;
      if (!pattern) continue;
      x = anchor.x + pattern->image.x;
      y = anchor.y + pattern->image.y;
      if (x < left) left = x;
      if (y < top) top = y;
      if (x + pattern->image.image.width > right)
        right = x + pattern->image.image.width;
      if (y + pattern->image.image.height > bottom)
        bottom = y + pattern->image.image.height;
    }
  }
  if (right > left && bottom > top) {
    result.x = (int16_t) left;
    result.y = (int16_t) top;
    result.width = (int16_t) (right - left);
    result.height = (int16_t) (bottom - top);
  }
  return result;
}

void sf_gameplay_player_draw(
    SfRenderer *renderer, const SfPlayerAssets *assets,
    const SfWorldState *world, const SfWorldRenderView *view,
    bool shadow, const SfRect *clip) {
  const SfCafSelectedAnimation *animation;
  const SfNjpSparseResource *resource;
  SfScreenPoint anchor;
  uint8_t priority;
  uint8_t frame;
  if (!renderer || !assets || !world || !view) return;
  animation = sf_gameplay_player_animation(assets, &world->player);
  resource = shadow ? &assets->shadows : &assets->artwork;
  if (!animation || animation->frame_count == 0u) return;
  frame = (uint8_t) (
    world->player.animation_frame % animation->frame_count);
  anchor = sf_world_to_screen(view->player_position);
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  for (priority = animation->priority_count; priority > 0u; --priority) {
    uint8_t part;
    for (part = 0u; part < animation->part_count; ++part) {
      const SfCafCell *cell = &animation->parts[part].cells[frame];
      const SfItemGroundDefinition *definition;
      const SfNjpSparsePattern *pattern;
      uint16_t opacity;
      SfBlendMode blend;
      if (!sf_gameplay_player_part_style(
            world, animation->parts[part].source_index, &definition) ||
          cell->priority != (int16_t) (priority - 1u) || cell->pattern < 0 ||
          (shadow != ((cell->status & 8) != 0))) continue;
      pattern = sf_njp_sparse_pattern(resource, cell->pattern);
      if (!pattern) continue;
      if (shadow) {
        opacity = 500u;
        blend = SF_BLEND_TRANSLUCENT;
      } else {
        opacity = cell->transparency < 0 ? 0u :
          cell->transparency > 1000 ? 1000u :
          (uint16_t) cell->transparency;
        blend = (cell->status & 0x10) != 0
          ? SF_BLEND_ADDITIVE : SF_BLEND_MASKED;
      }
      if (!shadow && definition) {
        sf_renderer_draw_indexed_tinted(
          renderer, &pattern->image.image,
          anchor.x + pattern->image.x, anchor.y + pattern->image.y,
          sf_gameplay_player_strength(definition->appearance_red_strength),
          sf_gameplay_player_strength(definition->appearance_green_strength),
          sf_gameplay_player_strength(definition->appearance_blue_strength),
          opacity, blend, clip);
      } else {
        sf_renderer_draw_indexed(
          renderer, &pattern->image.image,
          anchor.x + pattern->image.x, anchor.y + pattern->image.y,
          1000u, opacity, blend, clip);
      }
    }
  }
}
