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

#include "assets/menu_assets.h"
#include "assets/retail_paths.h"
#include "core/arena.h"
#include "core/memory_budget.h"
#include "game/game.h"
#include "runtime/screen_runtime.h"

#include "tal.h"
#include "twl.h"

#include <stdint.h>
#include <stdio.h>

typedef union SfMeasureMemory {
  long double alignment;
  void *pointer;
  uint64_t integer;
  uint8_t bytes[SF_MAIN_ARENA_BYTES];
} SfMeasureMemory;

static SfMeasureMemory sf_measure_memory;

static size_t sf_measure_screen_bytes(
    const SfScreenRuntime *runtime, SfGameMode mode) {
  if (mode == SF_GAME_MODE_TITLE)
    return runtime->assets.title.memory_bytes;
  if (mode == SF_GAME_MODE_CHARACTER_SELECT)
    return runtime->assets.character_create.memory_bytes;
  if (mode == SF_GAME_MODE_LOAD_GAME)
    return runtime->assets.load_game.memory_bytes;
  if (mode == SF_GAME_MODE_GAMEPLAY)
    return runtime->assets.gameplay.memory_bytes;
  return 0u;
}

static bool sf_measure_screen(
    SfScreenRuntime *runtime, SfArena *arena, SfGame *game,
    SfGameMode mode, const char *name) {
  size_t screen_bytes;
  game->mode = mode;
  if (mode == SF_GAME_MODE_GAMEPLAY)
    sf_world_state_init(&game->world, 0, 0, 1u);
  if (!sf_screen_runtime_load(runtime, game)) return false;
  screen_bytes = sf_measure_screen_bytes(runtime, mode);
  printf("%-18s %10zu %13zu %10zu\n",
    name, arena->used, screen_bytes, arena->capacity - arena->used);
  return true;
}

int main(int argument_count, char **arguments) {
  SfArena arena;
  SfMenuAssets *menu_assets;
  SfScreenRuntime *screen_runtime;
  SfGame *game;
  TwlConfig window_config;
  TalConfig audio_config;
  void *scratch;
  char data_root[SF_RETAIL_PATH_CAPACITY];
  if (!sf_retail_root_find(
        data_root, sizeof(data_root),
        argument_count > 0 ? arguments[0] : NULL,
        argument_count > 1 ? arguments[1] : NULL)) {
    fprintf(stderr, "Could not find the retail ShadowFlare data.\n");
    return 1;
  }
  sf_arena_init(
    &arena, sf_measure_memory.bytes, sizeof(sf_measure_memory.bytes));
  window_config = twl_config_default();
  window_config.title = "OpenShadowFlare";
  window_config.width = SF_FRAME_WIDTH;
  window_config.height = SF_FRAME_HEIGHT;
  window_config.event_capacity = 64u;
  window_config.controller_capacity = 2u;
  window_config.resizable = true;
  audio_config = tal_config_default();
  audio_config.sample_rate = 11025u;
  audio_config.mix_block_frames = 256u;
  audio_config.max_voices = 8u;
  audio_config.channels = 1u;
  if (!sf_arena_push(
        &arena, twl_memory_required(&window_config),
        twl_memory_alignment()) ||
      !sf_arena_push(
        &arena, tal_memory_required(&audio_config),
        tal_memory_alignment()) ||
      !(game = (SfGame *) sf_arena_push_zero(
          &arena, sizeof(SfGame), sizeof(void *)))) return 2;
  menu_assets = (SfMenuAssets *) sf_arena_push_zero(
    &arena, sizeof(*menu_assets), sizeof(void *));
  screen_runtime = (SfScreenRuntime *) sf_arena_push_zero(
    &arena, sizeof(*screen_runtime), sizeof(void *));
  scratch = sf_arena_push(
    &arena, SF_TITLE_DECODE_SCRATCH_BYTES, 4u);
  if (!menu_assets || !screen_runtime || !scratch ||
      !sf_menu_assets_load(menu_assets, data_root, &arena) ||
      !sf_screen_runtime_init(
        screen_runtime, &arena, data_root,
        scratch, SF_TITLE_DECODE_SCRATCH_BYTES)) return 3;
  sf_game_init(game, NULL);

  puts("screen             total bytes  screen bytes free bytes");
  if (!sf_measure_screen(
        screen_runtime, &arena, game, SF_GAME_MODE_TITLE, "title") ||
      !sf_measure_screen(
        screen_runtime, &arena, game,
        SF_GAME_MODE_CHARACTER_SELECT, "character create") ||
      !sf_measure_screen(
        screen_runtime, &arena, game,
        SF_GAME_MODE_LOAD_GAME, "load game") ||
      !sf_measure_screen(
        screen_runtime, &arena, game,
        SF_GAME_MODE_GAMEPLAY, "Remote Town map")) {
    fprintf(stderr, "Could not load one of the measured screens.\n");
    return 4;
  }
  return 0;
}
