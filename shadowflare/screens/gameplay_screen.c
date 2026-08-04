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
#include "ui/actor_nameplate.h"
#include "ui/conversation_bubble.h"
#include "ui/gameplay_belt.h"
#include "ui/gameplay_hud.h"
#include "ui/gameplay_inventory.h"
#include "ui/gameplay_item_condition.h"
#include "ui/gameplay_item_information.h"
#include "ui/ground_item_nameplate.h"
#include "ui/world_pointer_overlay.h"

#include <string.h>

static SfRect sf_gameplay_damage_union(SfRect first, SfRect second) {
  const int left = first.x < second.x ? first.x : second.x;
  const int top = first.y < second.y ? first.y : second.y;
  const int first_right = first.x + first.width;
  const int second_right = second.x + second.width;
  const int first_bottom = first.y + first.height;
  const int second_bottom = second.y + second.height;
  const int right = first_right > second_right ? first_right : second_right;
  const int bottom = first_bottom > second_bottom ? first_bottom : second_bottom;
  return (SfRect) {
    (int16_t) left, (int16_t) top,
    (int16_t) (right - left), (int16_t) (bottom - top)};
}

bool sf_gameplay_screen_init(
    SfGameplayScreen *screen, const SfGameplayAssets *assets,
    const SfWorldState *world) {
  SfWorldRenderView view;
  if (!screen || !assets || !world) return false;
  memset(screen, 0, sizeof(*screen));
  sf_gameplay_inventory_init(&screen->inventory);
  sf_world_render_view(world, 1000u, &view);
  if (!sf_gameplay_scene_update(
        &screen->scene, assets, world, &view, 1000u))
    return false;
  screen->player_damage = sf_gameplay_player_bounds(
    &assets->player, world, &view);
  return true;
}

static bool sf_gameplay_actor_frames_changed(
    const SfGameplayScreen *screen, const SfWorldState *world,
    uint16_t interpolation) {
  uint8_t index;
  for (index = 0u; index < world->actors.count; ++index) {
    const SfScenarioActor *actor = &world->actors.actors[index];
    const bool visible = sf_scenario_actor_state(
      actor, SF_SCENARIO_VISIBLE);
    const SfWorldPoint position = sf_scenario_actor_render_position(
      actor, interpolation);
    if (screen->rendered_actor_visible[index] != visible ||
        (visible &&
         (screen->rendered_actor_frames[index] != actor->animation_frame ||
          screen->rendered_actor_chart[index] != actor->animation_chart ||
          screen->rendered_actor_x[index] != position.x ||
          screen->rendered_actor_y[index] != position.y))) return true;
  }
  return false;
}

static void sf_gameplay_remember_actor_frames(
    SfGameplayScreen *screen, const SfWorldState *world,
    uint16_t interpolation) {
  uint8_t index;
  for (index = 0u; index < world->actors.count; ++index) {
    const SfScenarioActor *actor = &world->actors.actors[index];
    const SfWorldPoint position = sf_scenario_actor_render_position(
      actor, interpolation);
    screen->rendered_actor_frames[index] =
      actor->animation_frame;
    screen->rendered_actor_x[index] = position.x;
    screen->rendered_actor_y[index] = position.y;
    screen->rendered_actor_chart[index] = actor->animation_chart;
    screen->rendered_actor_visible[index] = sf_scenario_actor_state(
      actor, SF_SCENARIO_VISIBLE);
  }
}

void sf_gameplay_screen_draw(
    SfGameplayScreen *screen, SfRenderer *renderer,
    const SfGameplayAssets *assets, const SfGame *game,
    uint16_t interpolation) {
  const SfRect *clip = NULL;
  const SfPlayerState *player;
  SfWorldRenderView view;
  SfRect damage;
  SfRect ui_damage;
  bool scene_moved;
  uint8_t condition_phase;
  if (!screen || !renderer || !assets || !game ||
      !game->world.entered) return;
  player = &game->world.player;
  condition_phase = (uint8_t) ((game->ticks >> 3u) & 1u);
  sf_world_render_view(&game->world, interpolation, &view);
  if (screen->inventory.open)
    view.camera_x += SF_GAMEPLAY_INVENTORY_VIEW_OFFSET;
  scene_moved = !screen->drawn ||
    screen->rendered_player_x != view.player_position.x ||
    screen->rendered_player_y != view.player_position.y ||
    screen->rendered_camera_x != view.camera_x ||
    screen->rendered_camera_y != view.camera_y ||
    screen->rendered_hovered_actor_id !=
      game->world.pointer.hovered_actor_id ||
    screen->rendered_hovered_ground_item_id !=
      game->world.pointer.hovered_ground_item_id ||
    screen->rendered_message_id !=
      game->world.actor_script_state.message_id ||
    screen->rendered_selected_option !=
      game->world.actor_script_state.selected_option ||
    screen->rendered_message_active !=
      game->world.actor_script_state.message_active ||
    screen->rendered_pointer_x != game->world.pointer.screen_x ||
    screen->rendered_pointer_y != game->world.pointer.screen_y ||
    screen->rendered_pointer_active != game->world.pointer.active ||
    screen->rendered_motion != (uint8_t) player->motion ||
    screen->rendered_direction != player->direction ||
    screen->rendered_ground_item_revision !=
      game->world.ground_items.presentation_revision ||
    (screen->rendered_condition_phase != condition_phase &&
     sf_gameplay_item_condition_animation_active(
       assets, player, &screen->inventory)) ||
    sf_gameplay_actor_frames_changed(screen, &game->world, interpolation);
  if (screen->drawn && !scene_moved) {
    if (screen->rendered_animation_frame == player->animation_frame) return;
    damage = screen->player_damage;
    if (sf_world_pointer_overlay_bounds(&game->world, &ui_damage))
      damage = sf_gameplay_damage_union(damage, ui_damage);
    if (sf_actor_nameplate_bounds(
          assets, &game->world, &view, interpolation, &ui_damage))
      damage = sf_gameplay_damage_union(damage, ui_damage);
    if (sf_ground_item_nameplate_bounds(
          assets, &game->world, &view, &ui_damage))
      damage = sf_gameplay_damage_union(damage, ui_damage);
    clip = &damage;
    sf_renderer_fill_rect(renderer, *clip, 0u);
  } else {
    if (!sf_gameplay_scene_update(
          &screen->scene, assets, &game->world, &view, interpolation))
      return;
    screen->player_damage = sf_gameplay_player_bounds(
      &assets->player, &game->world, &view);
    sf_renderer_clear(renderer, 0u);
  }
  sf_gameplay_scene_draw(
    &screen->scene, renderer, assets, &game->world, &view,
    interpolation, clip);
  sf_actor_nameplate_draw(
    renderer, assets, &game->world, &view, interpolation);
  sf_ground_item_nameplate_draw(
    renderer, assets, &game->world, &view);
  sf_conversation_bubble_draw(
    renderer, assets, &game->world, &view, interpolation);
  sf_world_pointer_overlay_draw(renderer, &game->world);
  sf_gameplay_inventory_draw(
    renderer, assets, player, &screen->inventory, game->ticks, clip);
  sf_gameplay_hud_draw(renderer, assets, player, clip);
  sf_gameplay_belt_draw(renderer, assets, player, clip);
  sf_gameplay_inventory_draw_held(
    renderer, assets, player, &screen->inventory, game->ticks);
  sf_gameplay_item_information_draw(
    renderer, assets, player, &screen->inventory);
  screen->rendered_animation_frame = player->animation_frame;
  sf_gameplay_remember_actor_frames(screen, &game->world, interpolation);
  screen->rendered_player_x = view.player_position.x;
  screen->rendered_player_y = view.player_position.y;
  screen->rendered_camera_x = view.camera_x;
  screen->rendered_camera_y = view.camera_y;
  screen->rendered_hovered_actor_id =
    game->world.pointer.hovered_actor_id;
  screen->rendered_hovered_ground_item_id =
    game->world.pointer.hovered_ground_item_id;
  screen->rendered_message_id =
    game->world.actor_script_state.message_id;
  screen->rendered_selected_option =
    game->world.actor_script_state.selected_option;
  screen->rendered_message_active =
    game->world.actor_script_state.message_active;
  screen->rendered_pointer_x = game->world.pointer.screen_x;
  screen->rendered_pointer_y = game->world.pointer.screen_y;
  screen->rendered_pointer_active = game->world.pointer.active;
  screen->rendered_motion = (uint8_t) player->motion;
  screen->rendered_direction = player->direction;
  screen->rendered_ground_item_revision =
    game->world.ground_items.presentation_revision;
  screen->rendered_condition_phase = condition_phase;
  screen->drawn = true;
}
