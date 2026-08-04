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

#include "ui/enemy_nameplate.h"

#include "core/coordinates.h"

#include <stdio.h>
#include <string.h>

#define SF_ENEMY_NATIVE_ELEMENT_INDEX 6u

static const SfScenarioEnemy *sf_enemy_nameplate_enemy(
    const SfWorldState *world) {
  uint16_t index;
  if (!world || world->pointer.hovered_enemy_id < 0) return NULL;
  for (index = 0u; index < world->enemies.count; ++index) {
    const SfScenarioEnemy *enemy = &world->enemies.enemies[index];
    if (enemy->definition &&
        enemy->definition->id == world->pointer.hovered_enemy_id)
      return enemy;
  }
  return NULL;
}

static bool sf_enemy_nameplate_text(
    const SfScenarioEnemy *enemy, char *text, size_t capacity) {
  int length;
  if (!enemy || !enemy->definition || !enemy->definition->name[0] ||
      !text || capacity == 0u) return false;
  length = snprintf(text, capacity, "  %s", enemy->definition->name);
  return length > 0 && (size_t) length < capacity;
}

bool sf_enemy_nameplate_bounds(
    const SfGameplayAssets *assets, const SfWorldState *world,
    const SfWorldRenderView *view, uint16_t interpolation, SfRect *bounds) {
  const SfScenarioEnemy *enemy = sf_enemy_nameplate_enemy(world);
  const SfIndexedImage *font;
  SfScreenPoint anchor;
  char text[SF_MCT_PERSON_NAME_CAPACITY + 3u];
  int cell_width;
  int half_width;
  if (!assets || !view || !bounds || !enemy || enemy->current_life <= 0 ||
      assets->font.image_count == 0u ||
      !sf_enemy_nameplate_text(enemy, text, sizeof(text))) return false;
  font = &assets->font.images[0].image;
  cell_width = font->width / 16u;
  if (cell_width <= 0) return false;
  anchor = sf_world_to_screen(
    sf_scenario_enemy_render_position(enemy, interpolation));
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  half_width = (int) strlen(text) * cell_width / 2;
  bounds->x = (int16_t) (anchor.x - half_width - 5);
  bounds->y = (int16_t) (
    anchor.y - enemy->definition->label_height - 3);
  bounds->width = (int16_t) (half_width * 2 + 8);
  bounds->height = 18;
  return true;
}

static void sf_enemy_nameplate_draw_icon(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfScenarioEnemy *enemy, int x, int y) {
  const int32_t element = enemy->definition->pre_ai_values[
    SF_ENEMY_NATIVE_ELEMENT_INDEX];
  const SfNjpPatternImage *icon;
  if (element < 0 || element >= assets->status_icons.image_count) return;
  icon = &assets->status_icons.images[element];
  sf_renderer_draw_indexed(
    renderer, &icon->image, x + icon->x, y + icon->y,
    1000u, 1000u, SF_BLEND_MASKED, NULL);
}

void sf_enemy_nameplate_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldState *world, const SfWorldRenderView *view,
    uint16_t interpolation) {
  const SfScenarioEnemy *enemy = sf_enemy_nameplate_enemy(world);
  const SfMctEnemy *definition;
  SfRect frame;
  SfRect inner;
  SfRect life;
  char text[SF_MCT_PERSON_NAME_CAPACITY + 3u];
  int maximum_life;
  int current_life;
  int life_width;
  uint16_t name_color;
  if (!renderer || !assets || !enemy ||
      !sf_enemy_nameplate_bounds(
        assets, world, view, interpolation, &frame) ||
      !sf_enemy_nameplate_text(enemy, text, sizeof(text))) return;
  definition = enemy->definition;
  inner = (SfRect) {
    (int16_t) (frame.x + 1), (int16_t) (frame.y + 1),
    (int16_t) (frame.width - 2), 16};
  maximum_life = enemy->maximum_life > 0 ? enemy->maximum_life : 0;
  current_life = enemy->current_life < 0 ? 0 : enemy->current_life;
  if (current_life > maximum_life) current_life = maximum_life;
  life_width = maximum_life > 0
    ? inner.width * current_life / maximum_life : 0;
  sf_renderer_fill_rect_blended(renderer, frame, 0u, 800u);
  if (life_width > 0) {
    life = inner;
    life.width = (int16_t) life_width;
    sf_renderer_fill_rect_blended(
      renderer, life, sf_rgb555(16u, 4u, 4u), 500u);
  }
  if (life_width < inner.width) {
    life = inner;
    life.x = (int16_t) (life.x + life_width);
    life.width = (int16_t) (life.width - life_width);
    sf_renderer_fill_rect_blended(renderer, life, 0u, 500u);
  }
  name_color = sf_rgb555(
    (uint8_t) ((definition->name_color & 0xffu) >> 3u),
    (uint8_t) (((definition->name_color >> 8u) & 0xffu) >> 3u),
    (uint8_t) (((definition->name_color >> 16u) & 0xffu) >> 3u));
  sf_renderer_draw_text(
    renderer, &assets->font.images[0].image, text,
    frame.x + 6, frame.y + 4, 0u, 1000u);
  sf_renderer_draw_text(
    renderer, &assets->font.images[0].image, text,
    frame.x + 5, frame.y + 3, name_color, 1000u);
  sf_enemy_nameplate_draw_icon(
    renderer, assets, enemy, frame.x + 3, frame.y + 4);
}
