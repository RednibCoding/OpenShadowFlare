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

#include "ui/actor_nameplate.h"

#include "core/coordinates.h"

#include <string.h>

static const SfScenarioActor *sf_actor_nameplate_actor(
    const SfWorldState *world) {
  uint8_t index;
  if (!world || world->pointer.hovered_actor_id < 0) return NULL;
  for (index = 0u; index < world->actors.count; ++index) {
    if (world->actors.actors[index].id == world->pointer.hovered_actor_id)
      return &world->actors.actors[index];
  }
  return NULL;
}

static const SfMctPerson *sf_actor_nameplate_person(
    const SfGameplayAssets *assets, int32_t actor_id) {
  uint8_t index;
  if (!assets) return NULL;
  for (index = 0u; index < assets->scenario.people_count; ++index) {
    if (assets->scenario.people[index].id == actor_id)
      return &assets->scenario.people[index];
  }
  return NULL;
}

bool sf_actor_nameplate_bounds(
    const SfGameplayAssets *assets, const SfWorldState *world,
    const SfWorldRenderView *view, uint16_t interpolation, SfRect *bounds) {
  const SfScenarioActor *actor = sf_actor_nameplate_actor(world);
  const SfMctPerson *person;
  const SfIndexedImage *font;
  SfScreenPoint anchor;
  int cell_width;
  int half_width;
  if (!assets || !view || !bounds || !actor) return false;
  person = sf_actor_nameplate_person(assets, actor->id);
  if (!person || !person->name[0] || assets->font.image_count == 0u)
    return false;
  font = &assets->font.images[0].image;
  cell_width = font->width / 16u;
  if (cell_width <= 0) return false;
  anchor = sf_world_to_screen(
    sf_scenario_actor_render_position(actor, interpolation));
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  half_width = (int) strlen(person->name) * cell_width / 2;
  bounds->x = (int16_t) (anchor.x - half_width - 4);
  bounds->y = (int16_t) (anchor.y - person->label_height - 2);
  bounds->width = (int16_t) (half_width * 2 + 5);
  bounds->height = 15;
  return true;
}

void sf_actor_nameplate_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldState *world, const SfWorldRenderView *view,
    uint16_t interpolation) {
  const SfScenarioActor *actor = sf_actor_nameplate_actor(world);
  const SfMctPerson *person;
  const SfIndexedImage *font;
  SfRect background;
  uint16_t color;
  if (!renderer || !assets || !view || !actor ||
      !sf_actor_nameplate_bounds(
        assets, world, view, interpolation, &background)) return;
  person = sf_actor_nameplate_person(assets, actor->id);
  if (!person) return;
  font = &assets->font.images[0].image;
  color = sf_rgb555(
    (uint8_t) ((person->name_color & 0xffu) >> 3u),
    (uint8_t) (((person->name_color >> 8u) & 0xffu) >> 3u),
    (uint8_t) (((person->name_color >> 16u) & 0xffu) >> 3u));
  sf_renderer_fill_rect_blended(renderer, background, 0u, 500u);
  sf_renderer_draw_text(
    renderer, font, person->name,
    background.x + 5, background.y + 3, 0u, 1000u);
  sf_renderer_draw_text(
    renderer, font, person->name,
    background.x + 4, background.y + 2, color, 1000u);
}
