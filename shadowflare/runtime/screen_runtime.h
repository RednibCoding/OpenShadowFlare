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

#ifndef SHADOWFLARE_RUNTIME_SCREEN_RUNTIME_H
#define SHADOWFLARE_RUNTIME_SCREEN_RUNTIME_H

#include "assets/character_create_assets.h"
#include "assets/title_assets.h"
#include "core/arena.h"
#include "game/game.h"
#include "render/renderer.h"
#include "screens/character_create_screen.h"
#include "screens/title_screen.h"

#include <stdbool.h>
#include <stddef.h>

typedef union SfRuntimeScreenAssets {
  SfTitleAssets title;
  SfCharacterCreateAssets character_create;
} SfRuntimeScreenAssets;

typedef union SfRuntimeScreenState {
  SfTitleScreen title;
  SfCharacterCreateScreen character_create;
} SfRuntimeScreenState;

typedef struct SfScreenRuntime {
  SfArena *arena;
  const char *data_root;
  void *decode_scratch;
  size_t decode_scratch_size;
  size_t arena_mark;
  SfRuntimeScreenAssets assets;
  SfRuntimeScreenState screen;
  SfGameMode loaded_mode;
  bool loaded;
  bool blank_drawn;
} SfScreenRuntime;

bool sf_screen_runtime_init(
  SfScreenRuntime *runtime, SfArena *arena, const char *data_root,
  void *decode_scratch, size_t decode_scratch_size);
bool sf_screen_runtime_load(SfScreenRuntime *runtime, SfGameMode mode);
const SfTitleAssets *sf_screen_runtime_title_assets(
  const SfScreenRuntime *runtime);
void sf_screen_runtime_draw(
  SfScreenRuntime *runtime, SfRenderer *renderer, const SfGame *game);

#endif
