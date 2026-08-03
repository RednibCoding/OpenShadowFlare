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

#include "screens/gameplay_screen.h"

#include "screens/gameplay_player.h"

#include <string.h>

bool sf_gameplay_screen_init(
    SfGameplayScreen *screen, const SfGameplayAssets *assets,
    const SfWorldState *world) {
  SfWorldRenderView view;
  if (!screen || !assets || !world) return false;
  memset(screen, 0, sizeof(*screen));
  sf_world_render_view(world, 1000u, &view);
  if (!sf_gameplay_scene_update(&screen->scene, assets, world, &view))
    return false;
  screen->player_damage = sf_gameplay_player_bounds(
    &assets->player, world, &view);
  return true;
}

static bool sf_gameplay_actor_frames_changed(
    const SfGameplayScreen *screen, const SfWorldState *world) {
  uint8_t index;
  for (index = 0u; index < world->actors.count; ++index) {
    const SfScenarioActor *actor = &world->actors.actors[index];
    const bool visible = sf_scenario_actor_state(
      actor, SF_SCENARIO_VISIBLE);
    if (screen->rendered_actor_visible[index] != visible ||
        (visible &&
         (screen->rendered_actor_frames[index] != actor->animation_frame ||
          screen->rendered_actor_x[index] != actor->position.x ||
          screen->rendered_actor_y[index] != actor->position.y))) return true;
  }
  return false;
}

static void sf_gameplay_remember_actor_frames(
    SfGameplayScreen *screen, const SfWorldState *world) {
  uint8_t index;
  for (index = 0u; index < world->actors.count; ++index) {
    screen->rendered_actor_frames[index] =
      world->actors.actors[index].animation_frame;
    screen->rendered_actor_x[index] = world->actors.actors[index].position.x;
    screen->rendered_actor_y[index] = world->actors.actors[index].position.y;
    screen->rendered_actor_visible[index] = sf_scenario_actor_state(
      &world->actors.actors[index], SF_SCENARIO_VISIBLE);
  }
}

void sf_gameplay_screen_draw(
    SfGameplayScreen *screen, SfRenderer *renderer,
    const SfGameplayAssets *assets, const SfGame *game,
    uint16_t interpolation) {
  const SfRect *clip = NULL;
  const SfPlayerState *player;
  SfWorldRenderView view;
  bool scene_moved;
  if (!screen || !renderer || !assets || !game ||
      !game->world.entered) return;
  player = &game->world.player;
  sf_world_render_view(&game->world, interpolation, &view);
  scene_moved = !screen->drawn ||
    screen->rendered_player_x != view.player_position.x ||
    screen->rendered_player_y != view.player_position.y ||
    screen->rendered_camera_x != view.camera_x ||
    screen->rendered_camera_y != view.camera_y ||
    screen->rendered_motion != (uint8_t) player->motion ||
    screen->rendered_direction != player->direction ||
    sf_gameplay_actor_frames_changed(screen, &game->world);
  if (screen->drawn && !scene_moved) {
    if (screen->rendered_animation_frame == player->animation_frame) return;
    clip = &screen->player_damage;
    sf_renderer_fill_rect(renderer, *clip, 0u);
  } else {
    if (!sf_gameplay_scene_update(
          &screen->scene, assets, &game->world, &view)) return;
    screen->player_damage = sf_gameplay_player_bounds(
      &assets->player, &game->world, &view);
    sf_renderer_clear(renderer, 0u);
  }
  sf_gameplay_scene_draw(
    &screen->scene, renderer, assets, &game->world, &view, clip);
  screen->rendered_animation_frame = player->animation_frame;
  sf_gameplay_remember_actor_frames(screen, &game->world);
  screen->rendered_player_x = view.player_position.x;
  screen->rendered_player_y = view.player_position.y;
  screen->rendered_camera_x = view.camera_x;
  screen->rendered_camera_y = view.camera_y;
  screen->rendered_motion = (uint8_t) player->motion;
  screen->rendered_direction = player->direction;
  screen->drawn = true;
}
