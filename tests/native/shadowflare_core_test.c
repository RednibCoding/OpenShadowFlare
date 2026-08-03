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

#include "core/arena.h"
#include "core/coordinates.h"
#include "core/memory_budget.h"
#include "data/gnd.h"
#include "data/rclib.h"
#include "game/character_create.h"
#include "game/collision.h"
#include "game/game.h"
#include "game/load_game.h"
#include "game/movement.h"
#include "game/route.h"
#include "render/dirty.h"
#include "render/depth.h"
#include "render/framebuffer.h"
#include "render/renderer.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message) {
  if (condition) return 0;
  fprintf(stderr, "%s\n", message);
  return 1;
}

static int test_arena(void) {
  union {
    long double alignment;
    uint8_t bytes[128];
  } storage;
  SfArena arena;
  void *first;
  void *second;
  size_t mark;
  sf_arena_init(&arena, storage.bytes, sizeof(storage.bytes));
  first = sf_arena_push_zero(&arena, 17u, 8u);
  mark = sf_arena_mark(&arena);
  second = sf_arena_push(&arena, 16u, 16u);
  if (check(first != NULL, "arena rejected its first allocation") ||
      check(second != NULL, "arena rejected an aligned allocation") ||
      check((uintptr_t) second % 16u == 0u,
            "arena returned a misaligned allocation") ||
      check(sf_arena_rewind(&arena, mark), "arena rejected a valid mark") ||
      check(arena.used == mark, "arena did not rewind to its mark") ||
      check(sf_arena_push(&arena, 256u, 1u) == NULL,
            "arena exceeded its fixed capacity")) return 1;
  return 0;
}

static int test_framebuffer(void) {
  uint16_t pixels[34];
  SfFramebuffer framebuffer;
  size_t index;
  pixels[0] = 0x55aau;
  pixels[33] = 0xaa55u;
  if (check(sf_framebuffer_init(
        &framebuffer, &pixels[1], 32u * sizeof(uint16_t), 8u, 4u),
        "framebuffer rejected valid RGB555 memory")) return 1;
  sf_framebuffer_clear(&framebuffer, sf_rgb555(1u, 2u, 3u));
  sf_framebuffer_fill_rect(&framebuffer, -2, -2, 5, 5, 0x1234u);
  if (check(pixels[0] == 0x55aau && pixels[33] == 0xaa55u,
            "framebuffer drawing escaped its memory")) return 1;
  for (index = 0u; index < 32u; ++index) {
    if (check(pixels[index + 1u] != 0u,
              "framebuffer did not initialize every visible pixel"))
      return 1;
  }
  return 0;
}

static int test_world_coordinates(void) {
  const SfScreenPoint screen = sf_world_to_screen(
    (SfWorldPoint) {89898, 2811});
  const SfWorldPoint world = sf_screen_to_world((SfScreenPoint) {45, 30});
  if (check(screen.x == 13063 && screen.y == 9270,
            "world projection did not match the retail entry point") ||
      check(world.x == 300 && world.y == 0,
            "screen projection did not recover its world position") ||
      check(sf_floor_divide(-1, 64) == -1 &&
            sf_floor_divide(64, 64) == 1,
            "floor division broke map-cell culling") ||
      check(sizeof(SfGroundCell) == 2u,
            "render ground cells no longer fit their two-byte budget"))
    return 1;
  return 0;
}

static int test_player_movement(void) {
  SfPlayerState player;
  SfMovementStep step;
  SfWorldPoint rendered;
  sf_player_init(&player, 1u);
  sf_player_enter(&player, (SfWorldPoint) {1000, 1000}, 1u);
  sf_player_move_to(&player, (SfWorldPoint) {1100, 1000});
  sf_player_update(&player, NULL);
  rendered = sf_player_render_position(&player, 500u);
  if (check(player.position.x == 1020 && player.position.y == 1000,
            "retail walking speed was not twenty world units") ||
      check(rendered.x == 1010 && rendered.y == 1000,
            "integer interpolation did not produce the half-step position") ||
      check(player.motion == SF_PLAYER_WALKING &&
            player.direction == 1u && player.animation_frame == 0u,
            "walking did not select chart one and its first frame")) return 1;
  sf_player_toggle_pace(&player);
  sf_player_update(&player, NULL);
  rendered = sf_player_render_position(&player, 500u);
  if (check(player.position.x == 1060 &&
            player.motion == SF_PLAYER_RUNNING &&
            player.animation_frame == 0u,
            "running did not use twice the walk speed and chart two") ||
      check(rendered.x == 1040 && rendered.y == 1000,
            "running interpolation did not preserve the 30 Hz endpoints"))
    return 1;
  step = sf_movement_step_toward(
    (SfWorldPoint) {0, 0}, (SfWorldPoint) {3, 4}, 20u);
  if (check(step.arrived && step.position.x == 3 && step.position.y == 4,
            "a short movement step overshot its destination") ||
      check(sf_movement_direction(
              (SfWorldPoint) {0, 0}, (SfWorldPoint) {1, 1}) == 0u &&
            sf_movement_direction(
              (SfWorldPoint) {0, 0}, (SfWorldPoint) {1, 0}) == 1u &&
            sf_movement_direction(
              (SfWorldPoint) {0, 0}, (SfWorldPoint) {1, -1}) == 2u &&
            sf_movement_direction(
              (SfWorldPoint) {0, 0}, (SfWorldPoint) {0, -1}) == 3u &&
            sf_movement_direction(
              (SfWorldPoint) {0, 0}, (SfWorldPoint) {-1, -1}) == 4u &&
            sf_movement_direction(
              (SfWorldPoint) {0, 0}, (SfWorldPoint) {-1, 0}) == 5u &&
            sf_movement_direction(
              (SfWorldPoint) {0, 0}, (SfWorldPoint) {-1, 1}) == 6u &&
            sf_movement_direction(
              (SfWorldPoint) {0, 0}, (SfWorldPoint) {0, 1}) == 7u,
            "movement did not preserve the eight retail directions"))
    return 1;
  return 0;
}

static int test_world_pointer_movement(void) {
  SfWorldState world;
  SfGameInput input;
  int32_t released_x;
  unsigned update;
  sf_world_state_init(&world, 0, 0, 1u);
  sf_world_state_enter(&world, 1000, 1000, 1u);
  memset(&input, 0, sizeof(input));
  input.pointer_x = 420;
  input.pointer_y = 240;
  input.pointer_primary_pressed = true;
  sf_world_state_update(&world, &input);
  released_x = world.player.position.x;
  memset(&input, 0, sizeof(input));
  sf_world_state_update(&world, &input);
  if (check(world.player.position.x > released_x,
            "a single click did not retain its destination")) return 1;

  sf_world_state_enter(&world, 1000, 1000, 1u);
  memset(&input, 0, sizeof(input));
  input.pointer_x = 420;
  input.pointer_y = 240;
  input.pointer_primary_pressed = true;
  input.pointer_primary_down = true;
  sf_world_state_update(&world, &input);
  input.pointer_primary_pressed = false;
  for (update = 1u; update < 10u; ++update)
    sf_world_state_update(&world, &input);
  released_x = world.player.position.x;
  input.pointer_primary_down = false;
  sf_world_state_update(&world, &input);
  if (check(world.player.position.x == released_x &&
            world.player.motion == SF_PLAYER_IDLE,
            "releasing a held pointer did not stop movement immediately"))
    return 1;
  return 0;
}

static int test_collision_route(void) {
  SfMapObject object;
  SfObjectMap objects;
  SfCollisionWorld collision;
  SfRouteController route;
  SfObjectBounds player_bounds = {-80, -80, 79, 79};
  SfWorldPoint position = {0, 0};
  const SfWorldPoint destination = {250, 0};
  int32_t greatest_detour = 0;
  unsigned update;
  memset(&object, 0, sizeof(object));
  object.world_x = 90;
  object.status = 1;
  objects.objects = &object;
  objects.count = 1u;
  objects.version = 1u;
  collision.ground = NULL;
  collision.objects = &objects;
  if (check(sf_collision_position_walkable(
        &collision, (SfWorldPoint) {10, 0}, player_bounds, false),
        "collision rejected the last free point before an object") ||
      check(!sf_collision_position_walkable(
        &collision, (SfWorldPoint) {11, 0}, player_bounds, false),
        "collision ignored inclusive OBL judgement bounds")) return 1;
  sf_route_reset(&route);
  for (update = 0u; update < 100u &&
       (position.x != destination.x || position.y != destination.y);
       ++update) {
    const SfRouteStep step = sf_route_advance(
      &route, &collision, player_bounds, position, destination, 20u);
    position = step.position;
    if (position.y < 0 && -position.y > greatest_detour)
      greatest_detour = -position.y;
    if (position.y > greatest_detour) greatest_detour = position.y;
  }
  if (check(position.x == destination.x && position.y == destination.y &&
            greatest_detour > 79,
            "the retail edge controller did not route around an object"))
    return 1;
  return 0;
}

static int test_remote_town_collision(void) {
#if defined(OPENSHADOWFLARE_SOURCE_DIR)
  static uint8_t memory[512u * 1024u];
  SfArena arena;
  SfGroundMap ground;
  SfObjectMap objects;
  SfCollisionWorld collision;
  SfRouteController route;
  SfObjectBounds player_bounds = {-80, -80, 79, 79};
  SfWorldPoint position = {89800, 1450};
  const SfWorldPoint destination = {91800, 1450};
  char path[1024];
  int32_t greatest_detour = 0;
  unsigned update;
  FILE *probe;
  (void) snprintf(
    path, sizeof(path), "%s/tmp/ShadowFlare/Map/Ground/f00_01.Gnd",
    OPENSHADOWFLARE_SOURCE_DIR);
  probe = fopen(path, "rb");
  if (!probe) return 0;
  fclose(probe);
  sf_arena_init(&arena, memory, sizeof(memory));
  if (check(sf_gnd_load(path, &arena, &ground),
            "the C99 runtime could not load Remote Town collision")) return 1;
  (void) snprintf(
    path, sizeof(path), "%s/tmp/ShadowFlare/Map/Object/f00_01.Obl",
    OPENSHADOWFLARE_SOURCE_DIR);
  if (check(sf_obl_load(path, &arena, &objects),
            "the C99 runtime could not load Remote Town objects")) return 1;
  collision.ground = &ground;
  collision.objects = &objects;
  if (check(ground.judge_width == 852 && ground.judge_height == 852 &&
            ground.judge_offset_x == -1 && ground.judge_offset_y == -401,
            "Remote Town judgement dimensions differ from retail") ||
      check(sf_collision_position_walkable(
        &collision, (SfWorldPoint) {89898, 2811}, player_bounds, false),
        "the authored Remote Town spawn is no longer walkable") ||
      check(!sf_collision_position_walkable(
        &collision, (SfWorldPoint) {90700, 1450}, player_bounds, false),
        "the Remote Town sacks lost their blocking footprint")) return 1;
  sf_route_reset(&route);
  for (update = 0u; update < 500u &&
       (position.x != destination.x || position.y != destination.y);
       ++update) {
    const SfRouteStep step = sf_route_advance(
      &route, &collision, player_bounds, position, destination, 20u);
    position = step.position;
    if (position.y - 1450 > greatest_detour)
      greatest_detour = position.y - 1450;
    if (1450 - position.y > greatest_detour)
      greatest_detour = 1450 - position.y;
  }
  if (check(position.x == destination.x && position.y == destination.y &&
            greatest_detour > 100,
            "the player did not follow the full Remote Town sacks edge"))
    return 1;
  {
    static const SfWorldPoint destinations[] = {
      {92500, 500}, {91200, 500}, {93000, 3000}, {88700, 500}};
    unsigned route_index;
    for (route_index = 0u;
         route_index < sizeof(destinations) / sizeof(destinations[0]);
         ++route_index) {
      position = (SfWorldPoint) {89898, 2811};
      sf_route_reset(&route);
      for (update = 0u; update < 1000u &&
           (position.x != destinations[route_index].x ||
            position.y != destinations[route_index].y); ++update)
        position = sf_route_advance(
          &route, &collision, player_bounds, position,
          destinations[route_index], 20u).position;
      if (check(position.x == destinations[route_index].x &&
                position.y == destinations[route_index].y,
                "the edge controller did not retry between town obstacles"))
        return 1;
    }
  }
#endif
  return 0;
}

static int test_depth_order(void) {
  SfDepthEntry entries[3];
  memset(entries, 0, sizeof(entries));
  entries[0].source_index = 0u;
  entries[0].position.x = 300;
  entries[0].position.y = 300;
  entries[0].judgement.right = 100;
  entries[0].judgement.bottom = 100;
  entries[1].source_index = 1u;
  entries[1].position.x = 100;
  entries[1].position.y = 100;
  entries[1].judgement.right = 100;
  entries[1].judgement.bottom = 100;
  entries[2].source_index = 2u;
  entries[2].status = 0x20;
  sf_depth_sort(entries, 3u);
  if (check(entries[0].source_index == 2u &&
            entries[1].source_index == 1u &&
            entries[2].source_index == 0u &&
            sf_depth_class(entries[0].status) == 3,
            "display entries did not retain the retail class/depth order"))
    return 1;
  return 0;
}

static int test_renderer(void) {
  uint16_t pixels[8u * 4u];
  static const uint16_t direct_pixels[2] = {0x001fu, 0x7c00u};
  uint8_t font_pixels[16u * 16u] = {0};
  static const uint8_t indices[4] = {0u, 1u, 1u, 0u};
  static const uint16_t palette[2] = {0u, 0x7fffu};
  SfIndexedImage image;
  SfRenderer renderer;
  SfDirtyRects dirty;
  SfIndexedImage font;
  SfRgb555Image direct;
  SfRect rectangle = {1, 1, 2, 2};
  image.pixels = indices;
  image.palette = palette;
  image.width = 2u;
  image.height = 2u;
  image.stride = 2u;
  image.palette_size = 2u;
  image.bits_per_pixel = 8u;
  image.bottom_up = false;
  if (check(sf_renderer_init(
        &renderer, pixels, sizeof(pixels), 8u, 4u),
        "renderer rejected a valid target")) return 1;
  sf_renderer_clear(&renderer, 0u);
  sf_renderer_draw_indexed(
    &renderer, &image, 1, 1, 1000u, 1000u,
    SF_BLEND_MASKED, NULL);
  if (check(pixels[1u * 8u + 2u] == 0x7fffu &&
            pixels[2u * 8u + 1u] == 0x7fffu,
            "renderer did not draw a masked indexed image")) return 1;
  sf_dirty_clear(&dirty);
  sf_dirty_add(&dirty, rectangle, 8u, 4u);
  rectangle.x = 2;
  sf_dirty_add(&dirty, rectangle, 8u, 4u);
  if (check(dirty.count == 1u && dirty.rectangles[0].width == 3,
            "dirty rectangles did not merge touching damage")) return 1;
  font_pixels[4u * 16u + 1u] = 1u;
  font.pixels = font_pixels;
  font.palette = palette;
  font.width = 16u;
  font.height = 16u;
  font.stride = 16u;
  font.palette_size = 2u;
  font.bits_per_pixel = 8u;
  font.bottom_up = false;
  sf_renderer_clear(&renderer, 0u);
  sf_renderer_draw_text(
    &renderer, &font, "A", 0, 0, sf_rgb555(31u, 0u, 0u), 1000u);
  if (check(pixels[0] == sf_rgb555(31u, 0u, 0u),
            "renderer did not draw a bitmap-font glyph")) return 1;
  direct.pixels = direct_pixels;
  direct.width = 2u;
  direct.height = 1u;
  direct.stride = 2u;
  sf_renderer_draw_rgb555(&renderer, &direct, 3, 2, 1000u);
  if (check(pixels[2u * 8u + 3u] == 0x001fu &&
            pixels[2u * 8u + 4u] == 0x7c00u,
            "renderer did not draw a packed RGB555 image")) return 1;
  return 0;
}

static int test_load_game(void) {
  SfGame game;
  SfGameInput input;
  int frame;
  memset(&game, 0, sizeof(game));
  memset(&input, 0, sizeof(input));
  game.mode = SF_GAME_MODE_LOAD_GAME;
  game.config.saved_game_count = 4u;
  game.config.saved_game_file_slots[0] = 0u;
  game.config.saved_game_file_slots[1] = 2u;
  game.config.saved_game_file_slots[2] = 4u;
  game.config.saved_game_file_slots[3] = 5u;
  sf_load_game_state_init(&game);
  game.load_game.fade_steps_remaining = 0u;
  game.load_game.fade_value = 1000;
  game.load_game.fade_target = 1000;
  game.load_game.input_latch = false;
  input.right_pressed = true;
  sf_load_game_state_update(&game, &input);
  if (check(game.load_game.selection == 1u,
            "load-game navigation did not select the next save")) return 1;
  memset(&input, 0, sizeof(input));
  input.confirm_pressed = true;
  sf_load_game_state_update(&game, &input);
  if (check(game.load_game.screen == 10u &&
            game.load_game.selected_save == 1u &&
            game.load_game.selected_file_slot == 2,
            "load-game confirmation did not open the mode menu")) return 1;
  memset(&input, 0, sizeof(input));
  input.cancel_pressed = true;
  sf_load_game_state_update(&game, &input);
  if (check(game.load_game.screen == 0u &&
            game.load_game.brightness_increasing,
            "closing the mode menu did not restore the load screen"))
    return 1;
  memset(&input, 0, sizeof(input));
  input.delete_pressed = true;
  sf_load_game_state_update(&game, &input);
  if (check(game.load_game.screen == 1u &&
            game.load_game.dialog_selection == 1u,
            "Delete did not open the retail confirmation dialog")) return 1;
  memset(&input, 0, sizeof(input));
  input.left_pressed = true;
  sf_load_game_state_update(&game, &input);
  memset(&input, 0, sizeof(input));
  input.confirm_pressed = true;
  sf_load_game_state_update(&game, &input);
  if (check(game.load_game.delete_request == 1 &&
            game.load_game.screen == 0u,
            "confirming save deletion did not emit the selected request"))
    return 1;
  game.load_game.delete_request = -1;
  memset(&input, 0, sizeof(input));
  input.confirm_pressed = true;
  sf_load_game_state_update(&game, &input);
  memset(&input, 0, sizeof(input));
  input.down_pressed = true;
  sf_load_game_state_update(&game, &input);
  memset(&input, 0, sizeof(input));
  input.confirm_pressed = true;
  sf_load_game_state_update(&game, &input);
  for (frame = 0; frame < 16 && game.mode != SF_GAME_MODE_LOADING; ++frame) {
    memset(&input, 0, sizeof(input));
    sf_load_game_state_update(&game, &input);
  }
  if (check(game.mode == SF_GAME_MODE_LOADING,
            "Single Mode did not start the saved-game loading hand-off"))
    return 1;
  return 0;
}

static int test_rclib(void) {
  static const uint8_t encoded[] = {
    'R', 'C', 'L', 'I', 'B', '-', 'L', 0,
    4, 0, 0, 0, 5, 0, 0, 0,
    0, 1, 2, 3, 4
  };
  uint8_t decoded[4];
  if (check(sf_rclib_decode_memory(
        encoded, sizeof(encoded), decoded, sizeof(decoded)),
        "RCLIB rejected a valid literal block") ||
      check(decoded[0] == 1u && decoded[1] == 2u &&
            decoded[2] == 3u && decoded[3] == 4u,
            "RCLIB decoded the wrong literal bytes")) return 1;
  return 0;
}

static uint8_t test_rclib_invert(void *user, uint8_t value) {
  (void) user;
  return (uint8_t) ~value;
}

static bool test_rclib_sink(void *user, size_t offset, uint8_t value) {
  ((uint8_t *) user)[offset] = value;
  return true;
}

static int test_transformed_rclib(void) {
  static const uint8_t header[] = {
    'R', 'C', 'L', 'I', 'B', '-', 'L', 0,
    4, 0, 0, 0, 5, 0, 0, 0
  };
  static const uint8_t encoded[] = {
    (uint8_t) ~0u, (uint8_t) ~1u, (uint8_t) ~2u,
    (uint8_t) ~3u, (uint8_t) ~4u
  };
  uint8_t decoded[4] = {0};
  FILE *file = tmpfile();
  if (check(file != NULL, "could not create transformed RCLIB fixture"))
    return 1;
  if (fwrite(header, 1u, sizeof(header), file) != sizeof(header) ||
      fwrite(encoded, 1u, sizeof(encoded), file) != sizeof(encoded) ||
      fseek(file, 0L, SEEK_SET) != 0 ||
      !sf_rclib_decode_stream_to_transformed(
        file, sizeof(decoded), test_rclib_invert, NULL,
        test_rclib_sink, decoded)) {
    fclose(file);
    return check(0, "transformed RCLIB stream was not decoded");
  }
  fclose(file);
  if (check(decoded[0] == 1u && decoded[1] == 2u &&
            decoded[2] == 3u && decoded[3] == 4u,
            "transformed RCLIB stream decoded the wrong bytes")) return 1;
  return 0;
}

static int test_game(void) {
  SfGameConfig config;
  SfGame game;
  SfGameInput input;
  size_t index;
  for (index = 0u; index < sizeof(config); ++index)
    ((uint8_t *) &config)[index] = 0u;
  for (index = 0u; index < SF_GAME_TITLE_SMOKE_COUNT; ++index)
    config.title_smoke_frame_count[index] = 46u;
  config.next_save_available = true;
  for (index = 0u; index < sizeof(input); ++index)
    ((uint8_t *) &input)[index] = 0u;
  input.down_pressed = true;
  sf_game_init(&game, &config);
  sf_game_update(&game, &input);
  if (check(game.title.selection == 2u,
            "title input did not move the selection")) return 1;
  input.down_pressed = false;
  input.cancel_pressed = true;
  sf_game_update(&game, &input);
  if (check(game.title.transition_started,
            "cancel did not start the retail exit transition"))
    return 1;
  memset(&input, 0, sizeof(input));
  game.player_gender = 0u;
  game.mode = SF_GAME_MODE_LOADING;
  sf_game_update(&game, &input);
  if (check(game.mode == SF_GAME_MODE_GAMEPLAY &&
            game.world.scenario_id == 0 && game.world.entry_key == 0 &&
            game.world.player.gender == 0u &&
            game.world.player.appearance_part_count == 2u &&
            game.world.player.visible_item_count == 1u &&
            game.world.player.visible_items[0].category == 1u &&
            game.world.player.visible_items[0].definition_id == 0,
            "loading did not hand off to the first retail scenario"))
    return 1;
  return 0;
}

static int test_character_creation(void) {
  SfGame game;
  SfGameInput input;
  memset(&game, 0, sizeof(game));
  memset(&input, 0, sizeof(input));
  game.mode = SF_GAME_MODE_CHARACTER_SELECT;
  sf_character_create_state_init(&game);
  game.character_create.fade_steps_remaining = 0u;
  game.character_create.fade_value = 1000;
  game.character_create.fade_target = 1000;
  game.character_create.input_latch = false;
  input.right_pressed = true;
  sf_character_create_state_update(&game, &input);
  if (check(game.character_create.selection == 1u,
            "character creation did not select the female hero")) return 1;
  input.right_pressed = false;
  input.confirm_pressed = true;
  sf_character_create_state_update(&game, &input);
  if (check(game.character_create.screen == 1u &&
            game.character_create.gender == 0u &&
            game.character_create.rendered_transition_counter == 1000,
            "character creation did not begin retail name entry")) return 1;
  input.confirm_pressed = true;
  memcpy(input.text, "Mina", 5u);
  input.text_length = 4u;
  sf_character_create_state_update(&game, &input);
  if (check(game.character_create.screen == 10u &&
            strcmp(game.character_create.name, "Mina") == 0,
            "a valid character name did not open the mode menu")) return 1;
  memset(&input, 0, sizeof(input));
  input.down_pressed = true;
  sf_character_create_state_update(&game, &input);
  memset(&input, 0, sizeof(input));
  input.confirm_pressed = true;
  sf_character_create_state_update(&game, &input);
  if (check(game.character_create.screen == 20u,
            "Single Mode did not start the gameplay hand-off")) return 1;
  game.character_create.launch_counter = 5023;
  game.character_create.gender = 0u;
  memset(&input, 0, sizeof(input));
  sf_character_create_state_update(&game, &input);
  if (check(game.mode == SF_GAME_MODE_LOADING && game.player_gender == 0u,
            "character creation lost the selected retail gender")) return 1;
  return 0;
}

int main(void) {
  if (check(SF_MAIN_MEMORY_LIMIT_BYTES == 8u * 1024u * 1024u,
            "main-memory limit changed") ||
      check(SF_MAIN_SYSTEM_RESERVE_BYTES == 1024u * 1024u,
            "main-memory system reserve changed") ||
      check(SF_MAIN_ARENA_BYTES == 7u * 1024u * 1024u,
            "caller-owned game arena changed") ||
      check(SF_VIDEO_MEMORY_LIMIT_BYTES == 4u * 1024u * 1024u,
            "video-memory limit changed") ||
      check(SF_FRAMEBUFFER_BYTES == 640u * 480u * 2u,
            "RGB555 framebuffer size is wrong") ||
      check(SF_VIDEO_ASSET_BUDGET_BYTES == 3579904u,
            "video asset budget is wrong") ||
      test_arena() || test_world_coordinates() || test_player_movement() ||
      test_world_pointer_movement() || test_collision_route() ||
      test_remote_town_collision() || test_depth_order() ||
      test_framebuffer() || test_renderer() ||
      test_rclib() || test_transformed_rclib() ||
      test_game() || test_character_creation() ||
      test_load_game())
    return 1;
  return 0;
}
