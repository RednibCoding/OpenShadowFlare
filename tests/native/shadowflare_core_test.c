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
#include "game/game.h"
#include "render/framebuffer.h"
#include "render/title.h"

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

static int test_game(void) {
  uint16_t pixels[64u * 48u];
  SfFramebuffer framebuffer;
  SfGame game;
  SfGameInput input = {false, true, false, false};
  sf_game_init(&game);
  sf_game_update(&game, &input);
  if (check(game.title_selection == 1u,
            "title input did not move the selection")) return 1;
  input.down_pressed = false;
  input.cancel_pressed = true;
  sf_game_update(&game, &input);
  if (check(game.quit_requested, "cancel did not request a clean exit"))
    return 1;
  if (check(sf_framebuffer_init(
        &framebuffer, pixels, sizeof(pixels), 64u, 48u),
        "small test framebuffer failed to initialize")) return 1;
  sf_title_render(&game, &framebuffer);
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
      test_arena() || test_framebuffer() || test_game()) return 1;
  return 0;
}
