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
#include "core/memory_budget.h"
#include "data/rclib.h"
#include "game/game.h"
#include "render/dirty.h"
#include "render/framebuffer.h"
#include "render/renderer.h"

#include <stdint.h>
#include <stdio.h>

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

static int test_renderer(void) {
  uint16_t pixels[8u * 4u];
  static const uint8_t indices[4] = {0u, 1u, 1u, 0u};
  static const uint16_t palette[2] = {0u, 0x7fffu};
  SfIndexedImage image;
  SfRenderer renderer;
  SfDirtyRects dirty;
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
  return 0;
}

int main(void) {
  if (check(SF_MAIN_MEMORY_LIMIT_BYTES == 2u * 1024u * 1024u,
            "main-memory limit changed") ||
      check(SF_VIDEO_MEMORY_LIMIT_BYTES == 1024u * 1024u,
            "video-memory limit changed") ||
      check(SF_FRAMEBUFFER_BYTES == 640u * 480u * 2u,
            "RGB555 framebuffer size is wrong") ||
      check(SF_VIDEO_ASSET_BUDGET_BYTES == 434176u,
            "video asset budget is wrong") ||
      test_arena() || test_framebuffer() || test_renderer() ||
      test_rclib() || test_game())
    return 1;
  return 0;
}
